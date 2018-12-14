#ifndef	_RX_DBC_PGSQL_STATEMENT_H_
#define	_RX_DBC_PGSQL_STATEMENT_H_


namespace pgsql
{
    //-----------------------------------------------------
    //执行sql语句的功能类
    class stmt_t
    {
        stmt_t (const stmt_t&);
        stmt_t& operator = (const stmt_t&);
        friend class raw_stmt_t;

        typedef rx::alias_array_t<param_t, FIELD_NAME_LENGTH> param_array_t;
        typedef rx::tiny_string_t<char, MAX_SQL_LENGTH> sql_string_t;
        typedef rx::array_t<int>    mi_oid_array_t;
        typedef rx::array_t<char*>  mi_val_array_t;

        //-------------------------------------------------
        //对真正的pg语句执行动作进行封装,便于进行结果管理.
        class raw_stmt_t
        {
            PGresult               *m_res;                  //执行结果
            stmt_t                 &parent;                 //便于访问父类对象
            char                    m_pre_name[36];         //预解析sql语句的别名
            //----------------------------------------------
            void m_check_error()
            {
                if (!m_res)                                 //出现错误了
                    throw (error_info_t(DBEC_DB_CONNLOST, __FILE__, __LINE__,"SQL:{ %s }",parent.m_SQL.c_str()));

                //获取执行结果
                ::ExecStatusType ec = ::PQresultStatus(m_res);
                if (ec != PGRES_TUPLES_OK && ec != PGRES_COMMAND_OK)
                {//不成功,就进行错误信息记录
                    rx::tiny_string_t<char, 1024> tmp;
                    tmp = ::PQresultErrorMessage(m_res);
                    reset();                                //必须清理执行结果对象后,再抛出错误异常
                    throw (error_info_t(tmp.c_str(), __FILE__, __LINE__));
                }
            }
        public:
            //----------------------------------------------
            raw_stmt_t(stmt_t *p) :parent(*p) 
            {
                m_res = NULL; 
                m_pre_name[0] = 0;
            }
            ~raw_stmt_t() { reset(); }
            //----------------------------------------------
            //访问执行结果指针
            PGresult* res() { return m_res; }
            //获取预解析时生成的语句名字
            const char* pre_name() const { return m_pre_name; }
            //----------------------------------------------
            //进行真正的预解析,将sql与参数信息发送给服务器,不得到执行结果
            void prepare()
            {
                reset();
                m_pre_name[0] = 0;
                if (!parent.m_SQL.size())
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is empty!"));

                rx::string_alias4x8(parent.m_SQL.c_str(), m_pre_name);  //自动生成sql语句对应的别名

                //发送解析请求
                m_res = ::PQprepare(parent.m_conn.m_handle, m_pre_name, parent.m_SQL.c_str(), parent.m_params.size(), (Oid*)parent.m_mi_oids.array());
                m_check_error();
            }
            //----------------------------------------------
            //预解析后执行动作,将参数对应的数据发送给服务器,得到执行结果
            void prepare_exec(bool auto_commit)
            {
                reset();
                if (is_empty(m_pre_name))
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not prepared!"));
                if (auto_commit)                            //如果要求自动提交,就不要尝试进行自动事务
                    parent.m_conn.try_auto_trans(parent.m_SQL.c_str());
                m_res = ::PQexecPrepared(parent.m_conn.m_handle, m_pre_name, parent.m_params.size(), parent.m_mi_vals.array(), NULL, NULL, 0);
                m_check_error();
            }
            //----------------------------------------------
            //不进行预解析,直接将参数数据和sql语句发送给服务器,得到执行结果
            void params_exec(bool auto_commit)
            {
                reset(true);
                if (!parent.m_SQL.size())
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is empty!"));
                if (auto_commit)                            //如果要求自动提交,就不要尝试进行自动事务
                    parent.m_conn.try_auto_trans(parent.m_SQL.c_str());
                m_res = ::PQexecParams(parent.m_conn.m_handle, parent.m_SQL.c_str(), parent.m_params.size(), (Oid*)parent.m_mi_oids.array(), parent.m_mi_vals.array(), NULL, NULL, 0);
                m_check_error();
            }
            //----------------------------------------------
            //尝试释放之前执行的结果
            void reset(bool clear_pre_name=false)
            {
                if (m_res)
                {
                    PQclear(m_res);
                    m_res = NULL;
                }

                if (clear_pre_name)
                    m_pre_name[0] = 0;
            }
        };

    protected:
        conn_t		           &m_conn;		                //该语句对象关联的数据库连接对象
        mi_oid_array_t          m_mi_oids;                  //pgsql需要的绑定信息类型数组
        mi_val_array_t          m_mi_vals;                  //pgsql需要的绑定信息数值数组
        param_array_t	        m_params;	                //带有名称绑定的参数数组
        sql_type_t	            m_sql_type;                 //该语句对象当前sql语句的类型
        sql_string_t            m_SQL;                      //预解析时记录的待执行的sql语句
        sql_string_t            m_SQL_BAK;                  //预解析时记录的原始的sql语句
        uint32_t                m_cur_param_idx;            //绑定数据的时候用于顺序处理参数序号
        bool			        m_executed;                 //标记当前语句是否已经被正确执行过了
        raw_stmt_t              m_raw_stmt;                 //底层语句执行对象
        sql_param_parse_t<>     m_sp;                       //SQL参数解析器
        //-------------------------------------------------
        //绑定参数初始化:参数的数量(如果不告知,则自动根据sql语句分析获取);
        void m_param_make(uint16_t ParamCount)
        {
            if (!ParamCount) return;

            if (!m_params.make(ParamCount,true))            //生成绑定参数对象的数组
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            if (!m_mi_oids.make(ParamCount, true))          //生成绑定参数元信息类型数组
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            if (!m_mi_vals.make(ParamCount, true))          //生成绑定参数元信息数值数组
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            m_mi_oids.set(0);
            m_mi_vals.set(0);
        }
        //-------------------------------------------------
        //绑定一个命名变量给当前的语句
        param_t& m_param_bind(const char *name,int pg_data_type)
        {
            rx_assert(!is_empty(name));

            char Tmp[200];
            rx::st::strlwr(name, Tmp);

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
            param_t &p = m_params[ParamIdx];                //得到参数对象

            //对参数对象进行必要的初始化
            p.bind(m_mi_vals.array(),ParamIdx,&m_mi_oids.at(ParamIdx),name, pg_data_type);

            if (m_params.size() == m_sp.count)
                m_raw_stmt.prepare();                       //绑定参数够了,顺带执行真正的预解析

            return p;
        }
        //-------------------------------------------------
        //手动模式,根据名字获取类型
        int m_data_type_by_name(const char* name)
        {
            if (name == NULL)
                return PG_DATA_TYPE_UNKNOW;
            if (name[0] == ':')                             //ora格式参数名
                return  pg_data_type_by_dbc(get_data_type_by_name(name));
            else
            {//pg格式参数名
                rx_assert(name[0] == '$');
                const char *ts = rx::st::strstr(name, "::");
                if (ts)
                    return pg_data_type_by_name(ts + 2);
                return PG_DATA_TYPE_UNKNOW;
            }
        }
    public:
        //-------------------------------------------------
        stmt_t(conn_t &conn):m_conn(conn), m_params(conn.m_mem), m_raw_stmt(this)
        {
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

            //清理之前的状态
            close(true);

            //将待执行语句放入备份缓冲区
            if (!m_SQL_BAK.fmt(sql, arg))
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));
            
            //尝试进行ora绑定变量格式的解析,实现外部sql绑定格式的统一
            m_sp.ora_sql(m_SQL_BAK.c_str());

            if (m_sp.count)
            {//如果确实是ora模式的sql语句,则需要转换为pg格式
                rx::tiny_string_t<> dst(m_SQL.capacity(), m_SQL.ptr());
                m_sp.ora2pgsql(m_SQL_BAK.c_str(), dst);
                m_SQL.end(dst.size());
            }
            else
            {//不是ora模式,则尝试进行pg格式的sql绑定变量语句分析
                m_sp.pg_sql(m_SQL_BAK.c_str());
                //记录最终可执行语句
                m_SQL = m_SQL_BAK;
            }

            m_sql_type = get_sql_type(m_SQL);

            if (m_sp.count)
                m_param_make(m_sp.count);                   //有待绑定参数,进行参数对象数组的分配
            else
                m_raw_stmt.prepare();                       //没有待绑定参数,直接进行预解析

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
        stmt_t& auto_bind(int dummy=0)
        {
            if (m_sql_type == ST_UNKNOWN)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not prepared!"));

            //自动绑定需要依赖之前预解析时的结果,如果没有参数绑定的需求,则返回
            if (m_sp.count==0)
                return *this;

            rx_assert(is_empty(m_raw_stmt.pre_name()));

            char name[FIELD_NAME_LENGTH];
            bool is_ora_name = m_sp.segs[0].name[0] == ':';
            int pg_data_type;

            for (uint16_t i = 0; i < m_sp.count; ++i)
            {//循环进行参数数组的自动绑定
                sql_param_parse_t<>::param_seg_t &s = m_sp.segs[i];
                
                //获取参数名称,pg格式如果有附带参数类型,则也作为参数名称的一部分
                rx::st::strcpy(name, sizeof(name), s.name, s.length);

                if (is_ora_name)                            //根据ora参数名格式,转换为pg数据类型
                    pg_data_type = pg_data_type_by_dbc(get_data_type_by_name(name));
                else if (s.offset == 0)                     //如果pg参数名模式,但没有附加的类型,则直接给出未知类型
                    pg_data_type = PG_DATA_TYPE_UNKNOW; 
                else
                {//现在就需要尝试根据pg参数附带的类型名获取类型
                    rx_assert(s.name[0]=='$');
                    pg_data_type = pg_data_type_by_name(s.name+s.offset);
                }
                    
                m_param_bind(name, pg_data_type);           //绑定参数的方法内部会尝试进行自动预解析处理
            }

            return *this;
        }
        //-------------------------------------------------
        //在预解析完成后,可以尝试进行手动参数绑定,调整参数个数.
        stmt_t& manual_bind(uint16_t dummy=0, uint16_t params = 0)
        {
            if (m_sql_type == ST_UNKNOWN)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not prepared!"));

            if (m_sp.count == 0 && params == 0)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql manual bind error!"));
            
            if (params > m_sp.count)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql manual bind params error!"));

            m_param_make(params);                           //尝试进行参数数组的生成
            m_sp.count = params;                            //记录真正手动告知的参数数量

            return *this;
        }
        //-------------------------------------------------
        //对指定参数的绑定与当前深度的数据赋值同时进行,便于应用层操作
        template<class DT>
        stmt_t& operator()(const char* name, const DT& data)
        {
            param_t &param = m_param_bind(name, m_data_type_by_name(name));
            param = data;
            return *this;
        }
        //-------------------------------------------------
        //手动进行参数的绑定
        stmt_t& operator()(const char* name)
        {
            m_param_bind(name, m_data_type_by_name(name));
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
        sql_type_t	sql_type() { return m_sql_type; }
        //-------------------------------------------------
        //得到解析过的sql语句
        const char* sql_string() { return m_SQL; }
        //-------------------------------------------------
        //执行当前预解析过的语句,不进行返回记录集的处理
        stmt_t& exec (bool auto_commit=false)
        {
            m_executed = false;
            m_cur_param_idx = 0;
            m_raw_stmt.prepare_exec(auto_commit);
            m_executed = true;
            return *this;
        }
        //-------------------------------------------------
        //预解析与执行同时进行,中间没有绑定参数的机会了,适合不绑定参数的语句
        stmt_t& exec (const char *sql,...)
        {
            rx_assert(!is_empty(sql));
            //清理之前的状态
            close(true);

            va_list	arg;
            va_start(arg, sql);
            //将待执行语句放入备份缓冲区
            bool rc = m_SQL_BAK.fmt(sql, arg);
            va_end(arg);
            if (!rc)
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));

            m_SQL = m_SQL_BAK;

            m_raw_stmt.params_exec(false);
            
            return *this;
        }
        //-------------------------------------------------
        //得到上一条语句执行后被影响的行数(select无效)
        uint32_t rows()
        {
            if (!m_executed)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not executed!"));
            const char *msg = ::PQcmdTuples(m_raw_stmt.res());
            return (uint32_t)rx::st::atoi(msg);
        }
        //-------------------------------------------------
        //绑定过的参数数量
        uint32_t params() { return m_params.size(); }
        //获取绑定的参数对象
        param_t& param(const char* name)
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
        param_t& param(uint32_t Idx)
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
            {//不是复位,则强制要求释放内存
                m_mi_oids.clear();
                m_mi_vals.clear();
            }
                
            m_executed = false;
            m_cur_param_idx = 0;
            m_sql_type = ST_UNKNOWN;
            m_raw_stmt.reset(true);
            m_SQL.assign();
            m_SQL_BAK.assign();
            m_sp.reset();
        }
    };
}


#endif
