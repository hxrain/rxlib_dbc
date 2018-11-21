#ifndef	_RX_DBC_ORA_STATEMENT_H_
#define	_RX_DBC_ORA_STATEMENT_H_


namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //执行SQL语句的功能类
    //-----------------------------------------------------
    class command_t
    {
        command_t (const command_t&);
        command_t& operator = (const command_t&);
        friend class field_t;
    protected:
        typedef rx::alias_array_t<sql_param_t, FIELD_NAME_LENGTH> param_array_t;
        param_array_t		                m_params;	    //参数数组

        conn_t		                        &m_Conn;		//该语句对象关联的数据库连接对象
        OCIStmt			                    *m_StmtHandle;  //该语句对象的OCI句柄
        sql_stmt_t	                        m_SqlType;      //该语句对象当前SQL语句的类型
        String                              m_SQL;          //预解析时记录的待执行的SQL语句
        ub2                                 m_max_bulk_count; //参数批量数据提交的最大数量

        bool			                    m_IsPrepared;   //标记当前语句是否已经被预解析成功了
        bool			                    m_IsExecuted;   //标记当前语句是否已经被正确执行过了
        //-------------------------------------------------
        //释放全部的参数
        void m_bind_reset()
        {
            m_params.clear();
            m_max_bulk_count=1;
        }


    public:
        //-------------------------------------------------
        command_t (conn_t &use):m_Conn(use)
        {
            m_StmtHandle = NULL;
            m_IsPrepared = false;
            m_IsExecuted = false;
            m_SqlType = ST_UNKNOWN;
            m_max_bulk_count=1;
        }
        //-------------------------------------------------
        conn_t& conn()const{return m_Conn;}
        //-------------------------------------------------
        virtual ~command_t (){close ();}
        //-------------------------------------------------
        //得到解析过的SQL语句
        const char* SQL(){return m_SQL.c_str();}
        //-------------------------------------------------
        //预解析一个SQL语句,得到必要的信息,之后可以进行参数绑定了
        void prepare (const char *SQL,int Len = -1)
        {
            rx_assert (!is_empty(SQL));
            sword result;
            m_IsPrepared = false;
            m_IsExecuted = false;
            m_bind_reset();                                    //语句可能都变了,绑定的参数也放弃吧
            if (m_StmtHandle==NULL)
            {//分配SQL语句执行句柄,初始执行或在close之后执行
                result=OCIHandleAlloc (m_Conn.m_EnvHandle,(void **) &m_StmtHandle,OCI_HTYPE_STMT,0,NULL);
                if (result!=OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
            }

            if (Len == -1) Len = (int)strlen (SQL);              //解析SQL语句
            result = OCIStmtPrepare (m_StmtHandle,m_Conn.m_ErrHandle,(text *) SQL,Len,OCI_NTV_SYNTAX,OCI_DEFAULT);

            if (result == OCI_SUCCESS)
            {
                ub2	stmt_type = 0;                          //得到SQL语句的类型
                result = OCIAttrGet (m_StmtHandle,OCI_HTYPE_STMT,&stmt_type,NULL,OCI_ATTR_STMT_TYPE,m_Conn.m_ErrHandle);
                m_SqlType = (sql_stmt_t) stmt_type;           //stmt_type为0说明语句是错误的
            }

            if (result == OCI_SUCCESS)
            {
                m_IsPrepared = true;
                m_IsExecuted = false;
                m_SQL=SQL;
            }
            else
            {
                close();
                throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
            }
        }


        //-------------------------------------------------
        //执行当前预解析过的语句,不进行返回记录集的处理
        //入口:当前实际绑定参数的批量数.
        void exec (ub2 BulkCount=0)
        {
            if (!m_IsPrepared) throw (rx_dbc_ora::error_info_t (EC_METHOD_ORDER, __FILE__, __LINE__,"SQL Is Not Prepared!"));

            rx_assert(BulkCount<=m_max_bulk_count);
            if (BulkCount==0) BulkCount=m_max_bulk_count;

            ub4 iters = (m_SqlType == ST_SELECT) ? 0 : BulkCount;
            sword result = OCIStmtExecute (
                         m_Conn.m_SvcHandle,
                         m_StmtHandle,
                         m_Conn.m_ErrHandle,
                         iters,	// number of iterations
                         0,		// starting index from which the data in an array bind is relevant
                         NULL,	// input snapshot descriptor
                         NULL,	// output snapshot descriptor
                         OCI_DEFAULT);

            if (result == OCI_SUCCESS)
                m_IsExecuted = true;
            else
                throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //预解析与执行同时进行,中间没有绑定参数的机会了,适合不绑定参数的语句
        void exec (const char *SQL,int Len = -1)
        {
            prepare(SQL,Len);
            exec ();
        }
        //-------------------------------------------------
        //得到上一条语句执行后被影响的行数(select无效)
        ub4 row_count()
        {
            if (!m_IsExecuted) throw (rx_dbc_ora::error_info_t (EC_METHOD_ORDER, __FILE__, __LINE__,"SQL Is Not Executed!"));
            ub4 RC=0;
            sword result=OCIAttrGet(m_StmtHandle, OCI_HTYPE_STMT,&RC, 0, OCI_ATTR_ROW_COUNT, m_Conn.m_ErrHandle);
            if (result != OCI_SUCCESS)
                throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
            return RC;
        }
        //-------------------------------------------------
        //绑定参数初始化:批量数据数组的最大长度;参数的数量;
        //如果要使用Bulk模式批量插入,那么必须先调用此函数,告知每个Bulk的元素数量
        void bulk_bind_begin(ub2 MaxBulkCount,ub2 ParamCount=0)
        {
            m_bind_reset();
            if (ParamCount&&!m_params.make(ParamCount))
                throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
            m_max_bulk_count=MaxBulkCount;
            if (m_max_bulk_count>MAX_BULK_COUNT)
                throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //绑定一个命名变量给当前的语句,如果变量类型是DT_UNKNOWN,则根据变量名前缀进行自动分辨.对于字符串类型,可以设置参数缓存的尺寸
        //参数的数量在首个参数绑定时可以根据SQL语句中的':'的数量来确定
        sql_param_t &bind (const char *name,data_type_t type = DT_UNKNOWN,int MaxStringSize=MAX_OUTPUT_TEXT_BYTES)
        {
            rx_assert (!is_empty(name));
            char Tmp[200];
            rx::st::strlwr(name,Tmp);

            if (!m_params.capacity())
            {//之前没有初始化过,那么现在就根据SQL中的参数数量进行初始化.判断参数数量就简单的依据':'的数量,这样只多不少,是可以的
                ub4 PC=rx::st::count(m_SQL.c_str(),':');
                if (PC==0)
                    throw (rx_dbc_ora::error_info_t (EC_SQL_NOT_PARAM, __FILE__, __LINE__));
                    
                if (!m_params.make(PC))
                    throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
            }

            if (m_params.index(Tmp)!= m_params.capacity())
                throw (rx_dbc_ora::error_info_t (EC_PARAMNAME_DUP, __FILE__, __LINE__));

            ub4 ParamIdx=m_params.size();
            m_params.bind(ParamIdx, Tmp);                   //将参数的索引号与名字进行关联
            sql_param_t &Ret = m_params[ParamIdx];          //得到参数对象
            Ret.bind(m_Conn,m_StmtHandle,name,type,MaxStringSize,m_max_bulk_count);  //对参数对象进行必要的初始化
            return Ret;
        }
        //-------------------------------------------------
        //绑定过的参数数量
        ub4 params() { return m_params.size(); }
        //-------------------------------------------------
        //根据名字得到参数对象
        sql_param_t& operator [] (const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 Idx=m_params.index(Tmp);
            if (Idx==m_params.capacity())
                throw (rx_dbc_ora::error_info_t (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__, name));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //根据索引访问参数对象,索引从0开始
        sql_param_t& operator [] (ub4 Idx)
        {
            if (Idx>=m_params.size())
                throw (rx_dbc_ora::error_info_t (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //释放语句句柄,清理绑定的参数
        void close (void)
        {
            m_bind_reset();
            if (m_StmtHandle)
            {
                OCIHandleFree (m_StmtHandle,OCI_HTYPE_STMT);//释放SQL语句句柄
                m_StmtHandle = NULL;
            }
        }
    };

    //-----------------------------------------------------
    //让数据库连接对象可以直接执行SQL语句的方法,用到了HOStmt对象,所以需要放在HOStmt定义的后面
    inline void conn_t::exec (const char *sql_block,int sql_len)
    {
        rx_assert (!is_empty(sql_block));
        command_t st (*this);
        st.prepare(sql_block,sql_len);
        st.exec ();
    }

}


#endif
