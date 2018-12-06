#ifndef	_RX_DBC_MYSQL_STATEMENT_H_
#define	_RX_DBC_MYSQL_STATEMENT_H_


namespace rx_dbc_mysql
{
    //-----------------------------------------------------
    //执行sql语句的功能类
    class stmt_t
    {
        stmt_t (const stmt_t&);
        stmt_t& operator = (const stmt_t&);
        friend class field_t;
        typedef rx::alias_array_t<sql_param_t, FIELD_NAME_LENGTH> param_array_t;
        typedef rx::array_t<MYSQL_BIND> mi_array_t;
        typedef rx::tiny_string_t<char, MAX_SQL_LENGTH> sql_string_t;
    protected:
        conn_t		           &m_conn;		                //该语句对象关联的数据库连接对象
        mi_array_t              m_metainfos;                //mysql需要的绑定信息结构数组
        param_array_t	        m_params;	                //带有名称绑定的参数数组
        sql_stmt_t	            m_sql_type;                 //该语句对象当前sql语句的类型
        sql_string_t            m_SQL;                      //预解析时记录的待执行的sql语句
        sql_string_t            m_SQL_BAK;                  //预解析时记录的原始的sql语句
        MYSQL_STMT	           *m_stmt_handle;              //该语句对象的mysql句柄
        uint32_t                m_cur_param_idx;            //绑定数据的时候用于顺序处理参数序号
        bool			        m_executed;                 //标记当前语句是否已经被正确执行过了
        //-------------------------------------------------
        //预解析一个sql语句,得到必要的信息,之后可以进行参数绑定了
        void m_prepare()
        {
            rx_assert(!is_empty(m_SQL));
            
            close(true);                                    //语句可能都变了,复位后重来
            m_stmt_handle = mysql_stmt_init(&m_conn.m_handle);

            if (!m_stmt_handle)
                throw (error_info_t(&m_conn.m_handle, __FILE__, __LINE__,m_SQL));

            if (mysql_stmt_prepare(m_stmt_handle,m_SQL,m_SQL.size()))
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));

            m_sql_type = get_sql_type(m_SQL);
        }
        //-------------------------------------------------
        //绑定参数初始化:参数的数量(如果不告知,则自动根据sql语句分析获取);
        void m_param_make(uint16_t ParamCount = 0)
        {
            if (ParamCount == 0)
                ParamCount = rx::st::count(m_SQL.c_str(), '?');

            if (!ParamCount) return;

            if (!m_params.make(ParamCount,true))            //生成绑定参数对象的数组
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            if (!m_metainfos.make(ParamCount, true))        //生成绑定参数元信息数组
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));
        }
        //-------------------------------------------------
        //绑定一个命名变量给当前的语句
        sql_param_t& m_param_bind(const char *name)
        {
            rx_assert(!is_empty(name));

            char Tmp[200];
            rx::st::strlwr(name, Tmp);

            if (m_params.capacity() == 0)
                m_param_make();                             //尝试分配参数块资源

            if (m_params.capacity() == 0)
                throw (error_info_t(DBEC_NOT_PARAM, __FILE__, __LINE__, m_SQL));

            uint32_t ParamIdx = m_params.index(Tmp);
            if (ParamIdx != m_params.capacity())
                return m_params[ParamIdx];                  //参数名字重复的时候,直接返回

            //现在进行新名字的绑定
            ParamIdx = m_params.size();                     //利用绑定过的数量作为增量序数
            if (ParamIdx>= m_params.capacity())
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__, name));

            m_params.bind(ParamIdx, Tmp);                   //将参数的索引号与名字进行关联
            sql_param_t &Ret = m_params[ParamIdx];          //得到参数对象
            Ret.make(name, &m_metainfos.at(ParamIdx));      //对参数对象进行必要的初始化
            return Ret;
        }
    public:
        //-------------------------------------------------
        stmt_t(conn_t &conn):m_conn(conn), m_params(conn.m_mem)
        {
            m_stmt_handle = NULL;
            close();
        }
        //-------------------------------------------------
        conn_t& conn()const { return m_conn; }
        //-------------------------------------------------
        virtual ~stmt_t() { close(); }
        //-------------------------------------------------
        //预解析一个sql语句,得到必要的信息,之后可以进行参数绑定(调用auto_bind自动绑定或者调用(name,data)手动绑定)
        stmt_t& prepare(const char *sql,va_list arg)
        {
            rx_assert(!is_empty(sql));
            if (!m_SQL_BAK.fmt(sql, arg))                   //先将待执行语句放入备份缓冲区
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));
            
            //再尝试进行ora绑定变量语句的转换
            sql_param_parse_t<> sp;
            sp.ora_sql(m_SQL_BAK.c_str());
            if (sp.count)
            {
                rx::tiny_string_t<> dst(m_SQL.capacity(), m_SQL.ptr());
                sp.ora2mysql(m_SQL_BAK.c_str(), dst);
                m_SQL.end(dst.size());
            }
            else
                m_SQL = m_SQL_BAK;

            //最后再执行预处理
            m_prepare();
            return *this;
        }
        stmt_t& prepare(const char *sql, ...)
        {
            rx_assert(!is_empty(sql));
            va_list	arg;
            va_start(arg, sql);
            prepare(sql,arg);
            va_end(arg);
            return *this;
        }
        //-------------------------------------------------
        //在预解析完成后,可以直接进行参数的自动绑定
        //省去了再次调用(name,data)的时候带有名字的麻烦,可以直接使用<<data进行数据的绑定
        stmt_t& auto_bind()
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            //先尝试解析ora模式的命名参数
            sql_param_parse_t<> sp;
            const char* err = sp.ora_sql(m_SQL_BAK);
            if (err)
                throw (error_info_t(DBEC_PARSE_PARAM, __FILE__, __LINE__, "sql param parse error!| %s |",err));

            bool unnamed = false;
            if (sp.count == 0)
            {//再尝试解析mysql模式的未命名参数
                sp.count = rx::st::count(m_SQL.c_str(), '?');
                if (sp.count)
                    unnamed = true;
            }
            if (!sp.count)                                  //如果确实没有参数绑定的需求,则返回
                return *this;

            m_param_make(sp.count);

            char name[FIELD_NAME_LENGTH];
            for (uint16_t i = 0; i < sp.count; ++i)
            {//循环进行参数数组的自动绑定
                if (unnamed)
                    rx::st::itoa(i + 1, name);              //未命名的时候,使用序号当作名字,序号从1开始
                else
                    rx::st::strcpy(name, sizeof(name), sp.segs[i].name, sp.segs[i].name_len);
                m_param_bind(name);
            }
            return *this;
        }
        //-------------------------------------------------
        //在预解析完成后,如果不进行自动绑定则可以尝试进行手动参数绑定,告知的深度值.
        //省去了再次调用(name,data)的时候带有名字的麻烦,可以直接使用<<data进行数据的绑定
        stmt_t& manual_bind(uint16_t params = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            m_param_make(params);                           //尝试进行参数数组的生成

            return *this;
        }
        //-------------------------------------------------
        //对指定参数的绑定与当前深度的数据赋值同时进行,便于应用层操作
        template<class DT>
        stmt_t& operator()(const char* name, const DT& data)
        {
            sql_param_t &param = m_param_bind(name);
            param = data;
            return *this;
        }
        //-------------------------------------------------
        //手动进行参数的绑定
        stmt_t& operator()(const char* name)
        {
            m_param_bind(name);
            return *this;
        }
        //-------------------------------------------------
        //手动或自动参数绑定之后,可以进行参数数据的设置
        template<class DT>
        stmt_t& operator<<(const DT& data)
        {
            param(m_cur_param_idx++) = data;
            return *this;
        }
        //-------------------------------------------------
        //获取解析后的sql语句类型
        sql_stmt_t	sql_type() { return m_sql_type; }
        //-------------------------------------------------
        //得到解析过的sql语句
        const char* sql_string() { return m_SQL; }
        //-------------------------------------------------
        //执行当前预解析过的语句,不进行返回记录集的处理
        stmt_t& exec (bool auto_commit=false)
        {
            rx_assert(m_stmt_handle != NULL);
            m_executed = false;
            m_cur_param_idx = 0;
            
            if (m_params.size() && mysql_stmt_bind_param(m_stmt_handle, m_metainfos.array()))
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));

            if (mysql_stmt_execute(m_stmt_handle))
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));
            
            if (auto_commit)
                m_conn.trans_commit();

            m_executed = true;
            return *this;
        }
        //-------------------------------------------------
        //预解析与执行同时进行,中间没有绑定参数的机会了,适合不绑定参数的语句
        stmt_t& exec (const char *sql,...)
        {
            rx_assert(!is_empty(sql));
            va_list	arg;
            va_start(arg, sql);
            prepare(sql, arg);
            va_end(arg);
            return exec ();
        }
        //-------------------------------------------------
        //得到上一条语句执行后被影响的行数(select无效)
        uint32_t rows()
        {
            if (!m_executed||!m_stmt_handle)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Executed!"));
            return (uint32_t)mysql_stmt_affected_rows(m_stmt_handle);
        }
        //-------------------------------------------------
        //绑定过的参数数量
        uint32_t params() { return m_params.size(); }
        //获取绑定的参数对象
        sql_param_t& param(const char* name)
        {
            char Tmp[200];
            rx::st::strlwr(name, Tmp);
            uint32_t Idx = m_params.index(Tmp);
            if (Idx == m_params.capacity())
                throw (error_info_t(DBEC_PARAM_NOT_FOUND, __FILE__, __LINE__, name));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //根据索引访问参数对象,索引从0开始
        sql_param_t& param(uint32_t Idx)
        {
            if (Idx>=m_params.size())
                throw (error_info_t (DBEC_IDX_OVERSTEP, __FILE__, __LINE__));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //释放语句句柄,清理绑定的参数
        void close (bool reset_only=false)
        {
            m_params.clear(reset_only);
            if (!reset_only) 
                m_metainfos.clear();
            m_executed = false;
            m_cur_param_idx = 0;
            m_sql_type = ST_UNKNOWN;

            if (m_stmt_handle)
            {//释放sql语句句柄
                mysql_stmt_close(m_stmt_handle);
                m_stmt_handle = NULL;
            }
        }
    };
}


#endif
