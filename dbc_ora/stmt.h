#ifndef	_RX_DBC_ORA_STATEMENT_H_
#define	_RX_DBC_ORA_STATEMENT_H_


namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //执行sql语句的功能类
    class stmt_t
    {
        stmt_t (const stmt_t&);
        stmt_t& operator = (const stmt_t&);
        friend class field_t;
        typedef rx::alias_array_t<sql_param_t, FIELD_NAME_LENGTH> param_array_t;
    protected:
        conn_t		                        &m_conn;		//该语句对象关联的数据库连接对象
        param_array_t		                m_params;	    //带有名称绑定的参数数组
        OCIStmt			                    *m_stmt_handle; //该语句对象的OCI句柄
        sql_stmt_t	                        m_sql_type;     //该语句对象当前sql语句的类型
        rx::tiny_string_t<char,MAX_SQL_LENGTH>  m_SQL;      //预解析时记录的待执行的sql语句
        ub2                                 m_max_bulk_deep; //参数批量数据提交的最大数
        ub2                                 m_cur_bulk_idx; //当前操作的块深度索引
        bool			                    m_executed;     //标记当前语句是否已经被正确执行过了
        ub2                                 m_cur_param_idx;//当前正在绑定处理的参数顺序
        ub2                                 m_last_bulk_deep;//记录最后一次exec时处理的块深度,便于掌握最后的数据深度
        //-------------------------------------------------
        //预解析一个sql语句,得到必要的信息,之后可以进行参数绑定了
        void m_prepare()
        {
            rx_assert(m_SQL.size()!=0);
            sword result;
            close(true);                                    //语句可能都变了,复位后重来
#if RX_DBC_ORA_USE_OLD_STMT
            if (m_stmt_handle == NULL)
            {//分配sql语句执行句柄,初始执行或在close之后执行
                result = OCIHandleAlloc(m_conn.m_handle_env, (void **)&m_stmt_handle, OCI_HTYPE_STMT, 0, NULL);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
            }

            result = OCIStmtPrepare(m_stmt_handle, m_conn.m_handle_err, (text *)m_SQL.c_str(),m_SQL.size(), OCI_NTV_SYNTAX, OCI_DEFAULT);
#else
            //预解析/参数绑定/数据赋值等,可以在脱网的情况下进行
            rx_assert(m_stmt_handle==NULL);
            result = OCIStmtPrepare2(m_conn.m_handle_svc,&m_stmt_handle, m_conn.m_handle_err, (text *)m_SQL.c_str(), m_SQL.size(),NULL,0,OCI_NTV_SYNTAX, OCI_DEFAULT);
#endif
            if (result == OCI_SUCCESS)
            {
                ub2	stmt_type = 0;                          //得到sql语句的类型
                result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &stmt_type, NULL, OCI_ATTR_STMT_TYPE, m_conn.m_handle_err);
                m_sql_type = (sql_stmt_t)stmt_type;         //stmt_type为0说明语句是错误的
            }

            if (result != OCI_SUCCESS)
            {
                close(true);
                throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__,m_SQL.c_str()));
            }
        }
        //-------------------------------------------------
        //绑定参数初始化:批量数据数的最大数量(在exec的时候可以告知实际数量);参数的数量(如果不告知,则自动根据sql语句分析获取);
        //如果要使用Bulk模式批量插入,那么必须先调用此函数,告知每个Bulk的最大元素数量
        void m_param_make(ub2 max_bulk_deep, ub2 ParamCount = 0)
        {
            rx_assert(max_bulk_deep != 0);

            if (ParamCount == 0)    //尝试根据sql中的参数数量进行初始化.判断参数数量就简单的依据':'的数量,这样只多不少,是可以的
                ParamCount = rx::st::count(m_SQL.c_str(), ':');

            if (ParamCount)         //生成绑定参数对象的数组
            {
                if (ParamCount <= m_params.capacity())
                    m_params.clear(true);                       //容量还够,只需复位即可
                else if (!m_params.make_ex(ParamCount))         //容量不够重新分配
                    throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL.c_str()));
            }
            m_cur_bulk_idx = 0;
            m_max_bulk_deep = max_bulk_deep;
        }
        //-------------------------------------------------
        //如果批量数为1(默认情况),则可以直接调用bind
        //绑定一个命名变量给当前的语句,如果变量类型是DT_UNKNOWN,则根据变量名前缀进行自动分辨.对于字符串类型,可以设置参数缓存的尺寸
        //参数的数量在首个参数绑定时可以根据sql语句中的':'的数量来确定
        sql_param_t& m_param_bind(const char *name, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES)
        {
            rx_assert(!is_empty(name));
            rx_assert(rx::st::strstr(m_SQL.c_str(), name) != NULL);

            char Tmp[200];
            rx::st::strlwr(name, Tmp);

            if (m_params.capacity() == 0)
                m_param_make(m_max_bulk_deep);              //尝试分配参数块资源

            if (m_params.capacity() == 0)
                throw (error_info_t(DBEC_NOT_PARAM, __FILE__, __LINE__, m_SQL.c_str()));

            ub4 ParamIdx = m_params.index(Tmp);
            if (ParamIdx != m_params.capacity())
                return m_params[ParamIdx];                  //参数名字重复的时候,直接返回

            //现在进行新名字的绑定
            ParamIdx = m_params.size();                     //利用绑定过的数量作为增量序数
            if (ParamIdx>= m_params.capacity())
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__, name));

            m_params.bind(ParamIdx, Tmp);                   //将参数的索引号与名字进行关联
            sql_param_t &Ret = m_params[ParamIdx];          //得到参数对象
            Ret.bind_param(m_conn, m_stmt_handle, name, type, MaxStringSize, m_max_bulk_deep);  //对参数对象进行必要的初始化
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
            if (!m_SQL.fmt(sql, arg))
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));
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
        //在预解析完成后,可以直接进行参数的自动绑定,默认max_bulk_deep为0则使用预解析初始化时的默认值1.
        //省去了再次调用(name,data)的时候带有名字的麻烦,可以直接使用<<data进行数据的绑定
        stmt_t& auto_bind(ub2 max_bulk_deep = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            sql_param_parse_t<> sp;
            const char* err = sp.ora_sql(m_SQL.c_str());
            if (err)
                throw (error_info_t(DBEC_PARSE_PARAM, __FILE__, __LINE__, "sql param parse error!| %s |",err));

            if (max_bulk_deep == 0) max_bulk_deep = m_max_bulk_deep;
            m_param_make(max_bulk_deep, sp.count);           //不管解析得到了几个参数,都可尝试进行参数数组的生成

            char name[FIELD_NAME_LENGTH];
            for (ub2 i = 0; i < sp.count; ++i)
            {//循环进行参数数组的自动绑定
                rx::st::strcpy(name, sizeof(name), sp.segs[i].name, sp.segs[i].name_len);
                m_param_bind(name);
            }
            return *this;
        }
        //-------------------------------------------------
        //在预解析完成后,如果不进行自动绑定则可以尝试进行手动参数绑定,告知的深度值.
        //省去了再次调用(name,data)的时候带有名字的麻烦,可以直接使用<<data进行数据的绑定
        stmt_t& manual_bind(ub2 max_bulk_deep, ub2 params = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            if (max_bulk_deep == 0) max_bulk_deep = m_max_bulk_deep;
            m_param_make(max_bulk_deep, params);            //尝试进行参数数组的生成

            return *this;
        }
        //-------------------------------------------------
        //获取批量的最大深度或最后操作的深度
        ub2 bulks(bool is_max = true) { return is_max ? m_max_bulk_deep : m_last_bulk_deep;; }
        //-------------------------------------------------
        //设置所有参数的当前块访问深度
        stmt_t& bulk(ub2 idx)
        {
            rx_assert_if(m_cur_param_idx, m_cur_param_idx == m_params.size());//要求自动调整参数列序号的时候,必须与参数数量相同,避免<<的时候漏掉数据
            if (idx >= m_max_bulk_deep)
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__, "bulkdeep(%d/%d)",idx, m_max_bulk_deep));

            m_cur_param_idx = 0;
            m_cur_bulk_idx = idx;
            for (ub4 i = 0; i < m_params.size(); ++i)
                m_params[i].bulk(m_cur_bulk_idx);

            return *this;
        }
        //-------------------------------------------------
        //对指定参数的绑定与当前深度的数据赋值同时进行,便于应用层操作
        template<class DT>
        stmt_t& operator()(const char* name, const DT& data, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES)
        {
            sql_param_t &param = m_param_bind(name, type, MaxStringSize);
            param = data;
            return *this;
        }
        //-------------------------------------------------
        //手动进行参数的绑定
        stmt_t& operator()(const char* name, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES)
        {
            m_param_bind(name, type, MaxStringSize);
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
        const char* sql_string() { return m_SQL.c_str(); }
        //-------------------------------------------------
        //执行当前预解析过的语句,不进行返回记录集的处理
        //入口:当前实际绑定参数的批量深度.
        stmt_t& exec (ub2 BulkCount=0,bool auto_commit=false)
        {
            m_executed = false;
            rx_assert_if(m_cur_param_idx, m_cur_param_idx == m_params.size());//要求自动调整参数列序号的时候,必须与参数数量相同,避免<<的时候漏掉数据
            m_cur_param_idx = 0;

            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            rx_assert(BulkCount<=m_max_bulk_deep);
            
            m_last_bulk_deep = BulkCount;                  //记录最后执行时块的深度

            if (m_sql_type == ST_SELECT)
            {
                if (m_last_bulk_deep == 0)
                    m_last_bulk_deep = 1;
                BulkCount = 0;
            }
            else if (BulkCount == 0)
            {
                BulkCount = m_max_bulk_deep;
                m_last_bulk_deep = m_max_bulk_deep;
            }
                

            sword result = OCIStmtExecute (
                         m_conn.m_handle_svc,
                         m_stmt_handle,
                         m_conn.m_handle_err,
                         BulkCount,	            //批量操作的实际深度,select要求为0,其他为1或实际绑定深度
                         0,		// starting index from which the data in an array bind is relevant
                         NULL,	// input snapshot descriptor
                         NULL,	// output snapshot descriptor
                        auto_commit? OCI_COMMIT_ON_SUCCESS :OCI_DEFAULT);

            if (result == OCI_SUCCESS)
                m_executed = true;
            else
                throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__,m_SQL.c_str()));
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
        ub4 rows()
        {
            if (!m_executed)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Executed!"));
            ub4 RC=0;
            sword result=OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT,&RC, 0, OCI_ATTR_ROW_COUNT, m_conn.m_handle_err);
            if (result != OCI_SUCCESS)
                throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
            return RC;
        }
        //-------------------------------------------------
        //绑定过的参数数量
        ub4 params() { return m_params.size(); }
        //获取绑定的参数对象
        sql_param_t& param(const char* name)
        {
            char Tmp[200];
            rx::st::strlwr(name, Tmp);
            ub4 Idx = m_params.index(Tmp);
            if (Idx == m_params.capacity())
                throw (error_info_t(DBEC_PARAM_NOT_FOUND, __FILE__, __LINE__, name));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //根据索引访问参数对象,索引从0开始
        sql_param_t& param(ub4 Idx)
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
            m_last_bulk_deep = 0;
            m_max_bulk_deep = 1;
            m_cur_bulk_idx = 0;
            m_executed = false;
            m_cur_param_idx = 0;
            m_sql_type = ST_UNKNOWN;
#if RX_DBC_ORA_USE_OLD_STMT
            if (m_stmt_handle)
            {//释放sql语句句柄
                OCIHandleFree(m_stmt_handle,OCI_HTYPE_STMT);
                m_stmt_handle = NULL;
            }
#else
            if (m_stmt_handle)
            {//释放sql语句句柄
                OCIStmtRelease(m_stmt_handle, m_conn.m_handle_err,NULL,0, OCI_DEFAULT);
                m_stmt_handle = NULL;
            }
#endif
        }
    };

    //-----------------------------------------------------
    //让数据库连接对象可以直接执行sql语句的方法,用到了HOStmt对象,所以需要放在HOStmt定义的后面
    inline void conn_t::exec (const char *sql,...)
    {
        rx_assert(!is_empty(sql));
        va_list	arg;
        va_start(arg, sql);
        stmt_t st (*this);
        st.prepare(sql,arg);
        va_end(arg);
        st.exec ();
    }

}


#endif
