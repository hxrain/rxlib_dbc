#ifndef	_RX_DBC_ORA_STATEMENT_H_
#define	_RX_DBC_ORA_STATEMENT_H_


namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //ִ��SQL���Ĺ�����
    class stmt_t
    {
        stmt_t (const stmt_t&);
        stmt_t& operator = (const stmt_t&);
        friend class field_t;
        typedef rx::alias_array_t<sql_param_t, FIELD_NAME_LENGTH> param_array_t;
    protected:
        conn_t		                        &m_conn;		//����������������ݿ����Ӷ���
        param_array_t		                m_params;	    //�������ư󶨵Ĳ�������
        OCIStmt			                    *m_stmt_handle; //���������OCI���
        sql_stmt_t	                        m_sql_type;     //��������ǰSQL��������
        rx::tiny_string_t<char,MAX_SQL_LENGTH>  m_SQL;      //Ԥ����ʱ��¼�Ĵ�ִ�е�SQL���
        ub2                                 m_max_bulk_count; //�������������ύ�������
        bool			                    m_executed;     //��ǵ�ǰ����Ƿ��Ѿ�����ȷִ�й���
        //-------------------------------------------------
        //�ͷ�ȫ���Ĳ���
        void m_bind_reset()
        {
            m_params.clear();
            m_max_bulk_count=1;
        }
        //Ԥ����һ��SQL���,�õ���Ҫ����Ϣ,֮����Խ��в�������
        void m_prepare()
        {
            rx_assert(m_SQL.size()!=0);
            sword result;
            m_executed = false;
            m_bind_reset();                                 //�����ܶ�����,�󶨵Ĳ���Ҳ������

            if (m_stmt_handle == NULL)
            {//����SQL���ִ�о��,��ʼִ�л���close֮��ִ��
                result = OCIHandleAlloc(m_conn.m_EnvHandle, (void **)&m_stmt_handle, OCI_HTYPE_STMT, 0, NULL);
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn.m_ErrHandle, __FILE__, __LINE__));
            }

            result = OCIStmtPrepare(m_stmt_handle, m_conn.m_ErrHandle, (text *)m_SQL.c_str(),m_SQL.size(), OCI_NTV_SYNTAX, OCI_DEFAULT);

            if (result == OCI_SUCCESS)
            {
                ub2	stmt_type = 0;                          //�õ�SQL��������
                result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &stmt_type, NULL, OCI_ATTR_STMT_TYPE, m_conn.m_ErrHandle);
                m_sql_type = (sql_stmt_t)stmt_type;        //stmt_typeΪ0˵������Ǵ����
            }

            if (result != OCI_SUCCESS)
            {
                close();
                throw (rx_dbc_ora::error_info_t(result, m_conn.m_ErrHandle, __FILE__, __LINE__));
            }
        }
    public:
        //-------------------------------------------------
        stmt_t(conn_t &conn):m_conn(conn), m_params(conn.m_MemPool)
        {
            m_stmt_handle = NULL;
            m_executed = false;
            m_sql_type = ST_UNKNOWN;
            m_max_bulk_count=1;
        }
        //-------------------------------------------------
        conn_t& conn()const { return m_conn; }
        //-------------------------------------------------
        virtual ~stmt_t() { close(); }
        //-------------------------------------------------
        //�õ���������SQL���
        const char* SQL(){return m_SQL.c_str();}
        //-------------------------------------------------
        //Ԥ����һ��SQL���,�õ���Ҫ����Ϣ,֮����Խ��в�������
        void prepare(const char *SQL,int Len = -1)
        {
            rx_assert (!is_empty(SQL));
            if (Len == -1) Len = rx::st::strlen(SQL);
            if (m_SQL.set(SQL,Len)!=Len)
                throw (rx_dbc_ora::error_info_t(EC_NO_BUFFER, __FILE__, __LINE__, "SQL buffer is not enough!"));
            m_prepare();
        }
        void prepare(const char *SQL,va_list arg)
        {

        }
        //-------------------------------------------------
        //ִ�е�ǰԤ�����������,�����з��ؼ�¼���Ĵ���
        //���:��ǰʵ�ʰ󶨲�����������.
        void exec (ub2 BulkCount=0)
        {
            if (m_sql_type == ST_UNKNOWN) 
                throw (rx_dbc_ora::error_info_t(EC_METHOD_ORDER, __FILE__, __LINE__, "SQL Is Not Prepared!"));

            rx_assert(BulkCount<=m_max_bulk_count);
            if (BulkCount==0) BulkCount=m_max_bulk_count;

            if (m_sql_type == ST_SELECT)
                BulkCount = 0;

            sword result = OCIStmtExecute (
                         m_conn.m_SvcHandle,
                         m_stmt_handle,
                         m_conn.m_ErrHandle,
                         BulkCount,	// number of iterations
                         0,		// starting index from which the data in an array bind is relevant
                         NULL,	// input snapshot descriptor
                         NULL,	// output snapshot descriptor
                         OCI_DEFAULT);

            if (result == OCI_SUCCESS)
                m_executed = true;
            else
                throw (rx_dbc_ora::error_info_t (result, m_conn.m_ErrHandle, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //Ԥ������ִ��ͬʱ����,�м�û�а󶨲����Ļ�����,�ʺϲ��󶨲��������
        void exec (const char *SQL,int Len = -1)
        {
            prepare(SQL,Len);
            exec ();
        }
        //-------------------------------------------------
        //�õ���һ�����ִ�к�Ӱ�������(select��Ч)
        ub4 rows()
        {
            if (!m_executed)
                throw (rx_dbc_ora::error_info_t(EC_METHOD_ORDER, __FILE__, __LINE__, "SQL Is Not Executed!"));
            ub4 RC=0;
            sword result=OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT,&RC, 0, OCI_ATTR_ROW_COUNT, m_conn.m_ErrHandle);
            if (result != OCI_SUCCESS)
                throw (rx_dbc_ora::error_info_t(result, m_conn.m_ErrHandle, __FILE__, __LINE__));
            return RC;
        }
        //-------------------------------------------------
        //�󶨲�����ʼ��:�������������������(��exec��ʱ����Ը�֪ʵ������);����������(�������֪,���Զ�����sql��������ȡ);
        //���Ҫʹ��Bulkģʽ��������,��ô�����ȵ��ô˺���,��֪ÿ��Bulk�����Ԫ������
        void begin(ub2 MaxBulkCount,ub2 ParamCount=0)
        {
            m_bind_reset();

            if (ParamCount == 0)
            {//���Ը���SQL�еĲ����������г�ʼ��.�жϲ��������ͼ򵥵�����':'������,����ֻ�಻��,�ǿ��Ե�
                ParamCount = rx::st::count(m_SQL.c_str(), ':');
                if (ParamCount&&!m_params.make_ex(ParamCount))
                    throw (rx_dbc_ora::error_info_t(EC_NO_MEMORY, __FILE__, __LINE__));
            }

            m_max_bulk_count=MaxBulkCount;
        }
        //-------------------------------------------------
        //���������Ϊ1(Ĭ�����),�����ֱ�ӵ���bind������Ҫbegin
        //��һ��������������ǰ�����,�������������DT_UNKNOWN,����ݱ�����ǰ׺�����Զ��ֱ�.�����ַ�������,�������ò�������ĳߴ�
        //�������������׸�������ʱ���Ը���SQL����е�':'��������ȷ��
        sql_param_t& bind(const char *name,data_type_t type = DT_UNKNOWN,int MaxStringSize=MAX_TEXT_BYTES)
        {
            rx_assert (!is_empty(name));
            char Tmp[200];
            rx::st::strlwr(name,Tmp);

            if (!m_params.capacity())
            {//֮ǰû�г�ʼ����,��ô���ھ͸���SQL�еĲ����������г�ʼ��.�жϲ��������ͼ򵥵�����':'������,����ֻ�಻��,�ǿ��Ե�
                ub4 PC=rx::st::count(m_SQL.c_str(),':');
                if (PC==0)
                    throw (rx_dbc_ora::error_info_t (EC_SQL_NOT_PARAM, __FILE__, __LINE__));
                    
                if (!m_params.make_ex(PC))
                    throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
            }

            ub4 ParamIdx = m_params.index(Tmp);
            if (ParamIdx != m_params.capacity())
                return m_params[ParamIdx];                  //���������ظ���ʱ��,ֱ�ӷ���

            //���ڽ��������ֵİ�
            ParamIdx=m_params.size();
            m_params.bind(ParamIdx, Tmp);                   //�������������������ֽ��й���
            sql_param_t &Ret = m_params[ParamIdx];          //�õ���������
            Ret.bind(m_conn,m_stmt_handle,name,type,MaxStringSize,m_max_bulk_count);  //�Բ���������б�Ҫ�ĳ�ʼ��
            return Ret;
        }
        //��ȡ�󶨵Ĳ�������
        sql_param_t& param(const char* name) 
        { 
            char Tmp[200];
            rx::st::strlwr(name, Tmp);
            ub4 Idx = m_params.index(Tmp);
            if (Idx == m_params.capacity())
                throw (rx_dbc_ora::error_info_t(EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__, name));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //�󶨹��Ĳ�������
        ub4 params() { return m_params.size(); }
        //-------------------------------------------------
        //�������ֵõ���������
        sql_param_t& operator [] (const char *name) { return param(name); }
        //-------------------------------------------------
        //�����������ʲ�������,������0��ʼ
        sql_param_t& operator [] (ub4 Idx)
        {
            if (Idx>=m_params.size())
                throw (rx_dbc_ora::error_info_t (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //�ͷ������,����󶨵Ĳ���
        void close (void)
        {
            m_bind_reset();
            if (m_stmt_handle)
            {
                OCIHandleFree(m_stmt_handle,OCI_HTYPE_STMT); //�ͷ�SQL�����
                m_stmt_handle = NULL;
            }
        }
    };

    //-----------------------------------------------------
    //�����ݿ����Ӷ������ֱ��ִ��SQL���ķ���,�õ���HOStmt����,������Ҫ����HOStmt����ĺ���
    inline void conn_t::exec (const char *sql_block,int sql_len)
    {
        rx_assert (!is_empty(sql_block));
        stmt_t st (*this);
        st.prepare(sql_block,sql_len);
        st.exec ();
    }

}


#endif
