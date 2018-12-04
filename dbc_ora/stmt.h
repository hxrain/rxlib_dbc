#ifndef	_RX_DBC_ORA_STATEMENT_H_
#define	_RX_DBC_ORA_STATEMENT_H_


namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //ִ��sql���Ĺ�����
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
        sql_stmt_t	                        m_sql_type;     //��������ǰsql��������
        rx::tiny_string_t<char,MAX_SQL_LENGTH>  m_SQL;      //Ԥ����ʱ��¼�Ĵ�ִ�е�sql���
        ub2                                 m_max_bulk_deep; //�������������ύ�������
        ub2                                 m_cur_bulk_idx; //��ǰ�����Ŀ��������
        bool			                    m_executed;     //��ǵ�ǰ����Ƿ��Ѿ�����ȷִ�й���
        ub2                                 m_cur_param_idx;//��ǰ���ڰ󶨴���Ĳ���˳��
        ub2                                 m_last_bulk_deep;//��¼���һ��execʱ����Ŀ����,�������������������
        //-------------------------------------------------
        //Ԥ����һ��sql���,�õ���Ҫ����Ϣ,֮����Խ��в�������
        void m_prepare()
        {
            rx_assert(m_SQL.size()!=0);
            sword result;
            close(true);                                    //�����ܶ�����,��λ������
#if RX_DBC_ORA_USE_OLD_STMT
            if (m_stmt_handle == NULL)
            {//����sql���ִ�о��,��ʼִ�л���close֮��ִ��
                result = OCIHandleAlloc(m_conn.m_handle_env, (void **)&m_stmt_handle, OCI_HTYPE_STMT, 0, NULL);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
            }

            result = OCIStmtPrepare(m_stmt_handle, m_conn.m_handle_err, (text *)m_SQL.c_str(),m_SQL.size(), OCI_NTV_SYNTAX, OCI_DEFAULT);
#else
            //Ԥ����/������/���ݸ�ֵ��,����������������½���
            rx_assert(m_stmt_handle==NULL);
            result = OCIStmtPrepare2(m_conn.m_handle_svc,&m_stmt_handle, m_conn.m_handle_err, (text *)m_SQL.c_str(), m_SQL.size(),NULL,0,OCI_NTV_SYNTAX, OCI_DEFAULT);
#endif
            if (result == OCI_SUCCESS)
            {
                ub2	stmt_type = 0;                          //�õ�sql��������
                result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &stmt_type, NULL, OCI_ATTR_STMT_TYPE, m_conn.m_handle_err);
                m_sql_type = (sql_stmt_t)stmt_type;         //stmt_typeΪ0˵������Ǵ����
            }

            if (result != OCI_SUCCESS)
            {
                close(true);
                throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__,m_SQL.c_str()));
            }
        }
        //-------------------------------------------------
        //�󶨲�����ʼ��:�������������������(��exec��ʱ����Ը�֪ʵ������);����������(�������֪,���Զ�����sql��������ȡ);
        //���Ҫʹ��Bulkģʽ��������,��ô�����ȵ��ô˺���,��֪ÿ��Bulk�����Ԫ������
        void m_param_make(ub2 max_bulk_deep, ub2 ParamCount = 0)
        {
            rx_assert(max_bulk_deep != 0);

            if (ParamCount == 0)    //���Ը���sql�еĲ����������г�ʼ��.�жϲ��������ͼ򵥵�����':'������,����ֻ�಻��,�ǿ��Ե�
                ParamCount = rx::st::count(m_SQL.c_str(), ':');

            if (ParamCount)         //���ɰ󶨲������������
            {
                if (ParamCount <= m_params.capacity())
                    m_params.clear(true);                       //��������,ֻ�踴λ����
                else if (!m_params.make_ex(ParamCount))         //�����������·���
                    throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL.c_str()));
            }
            m_cur_bulk_idx = 0;
            m_max_bulk_deep = max_bulk_deep;
        }
        //-------------------------------------------------
        //���������Ϊ1(Ĭ�����),�����ֱ�ӵ���bind
        //��һ��������������ǰ�����,�������������DT_UNKNOWN,����ݱ�����ǰ׺�����Զ��ֱ�.�����ַ�������,�������ò�������ĳߴ�
        //�������������׸�������ʱ���Ը���sql����е�':'��������ȷ��
        sql_param_t& m_param_bind(const char *name, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES)
        {
            rx_assert(!is_empty(name));
            rx_assert(rx::st::strstr(m_SQL.c_str(), name) != NULL);

            char Tmp[200];
            rx::st::strlwr(name, Tmp);

            if (m_params.capacity() == 0)
                m_param_make(m_max_bulk_deep);              //���Է����������Դ

            if (m_params.capacity() == 0)
                throw (error_info_t(DBEC_NOT_PARAM, __FILE__, __LINE__, m_SQL.c_str()));

            ub4 ParamIdx = m_params.index(Tmp);
            if (ParamIdx != m_params.capacity())
                return m_params[ParamIdx];                  //���������ظ���ʱ��,ֱ�ӷ���

            //���ڽ��������ֵİ�
            ParamIdx = m_params.size();                     //���ð󶨹���������Ϊ��������
            if (ParamIdx>= m_params.capacity())
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__, name));

            m_params.bind(ParamIdx, Tmp);                   //�������������������ֽ��й���
            sql_param_t &Ret = m_params[ParamIdx];          //�õ���������
            Ret.bind_param(m_conn, m_stmt_handle, name, type, MaxStringSize, m_max_bulk_deep);  //�Բ���������б�Ҫ�ĳ�ʼ��
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
        //Ԥ����һ��sql���,�õ���Ҫ����Ϣ,֮����Խ��в�����(����auto_bind�Զ��󶨻��ߵ���(name,data)�ֶ���)
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
        //��Ԥ������ɺ�,����ֱ�ӽ��в������Զ���,Ĭ��max_bulk_deepΪ0��ʹ��Ԥ������ʼ��ʱ��Ĭ��ֵ1.
        //ʡȥ���ٴε���(name,data)��ʱ��������ֵ��鷳,����ֱ��ʹ��<<data�������ݵİ�
        stmt_t& auto_bind(ub2 max_bulk_deep = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            sql_param_parse_t<> sp;
            const char* err = sp.ora_sql(m_SQL.c_str());
            if (err)
                throw (error_info_t(DBEC_PARSE_PARAM, __FILE__, __LINE__, "sql param parse error!| %s |",err));

            if (max_bulk_deep == 0) max_bulk_deep = m_max_bulk_deep;
            m_param_make(max_bulk_deep, sp.count);           //���ܽ����õ��˼�������,���ɳ��Խ��в������������

            char name[FIELD_NAME_LENGTH];
            for (ub2 i = 0; i < sp.count; ++i)
            {//ѭ�����в���������Զ���
                rx::st::strcpy(name, sizeof(name), sp.segs[i].name, sp.segs[i].name_len);
                m_param_bind(name);
            }
            return *this;
        }
        //-------------------------------------------------
        //��Ԥ������ɺ�,����������Զ�������Գ��Խ����ֶ�������,��֪�����ֵ.
        //ʡȥ���ٴε���(name,data)��ʱ��������ֵ��鷳,����ֱ��ʹ��<<data�������ݵİ�
        stmt_t& manual_bind(ub2 max_bulk_deep, ub2 params = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            if (max_bulk_deep == 0) max_bulk_deep = m_max_bulk_deep;
            m_param_make(max_bulk_deep, params);            //���Խ��в������������

            return *this;
        }
        //-------------------------------------------------
        //��ȡ�����������Ȼ������������
        ub2 bulks(bool is_max = true) { return is_max ? m_max_bulk_deep : m_last_bulk_deep;; }
        //-------------------------------------------------
        //�������в����ĵ�ǰ��������
        stmt_t& bulk(ub2 idx)
        {
            rx_assert_if(m_cur_param_idx, m_cur_param_idx == m_params.size());//Ҫ���Զ�������������ŵ�ʱ��,���������������ͬ,����<<��ʱ��©������
            if (idx >= m_max_bulk_deep)
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__, "bulkdeep(%d/%d)",idx, m_max_bulk_deep));

            m_cur_param_idx = 0;
            m_cur_bulk_idx = idx;
            for (ub4 i = 0; i < m_params.size(); ++i)
                m_params[i].bulk(m_cur_bulk_idx);

            return *this;
        }
        //-------------------------------------------------
        //��ָ�������İ��뵱ǰ��ȵ����ݸ�ֵͬʱ����,����Ӧ�ò����
        template<class DT>
        stmt_t& operator()(const char* name, const DT& data, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES)
        {
            sql_param_t &param = m_param_bind(name, type, MaxStringSize);
            param = data;
            return *this;
        }
        //-------------------------------------------------
        //�ֶ����в����İ�
        stmt_t& operator()(const char* name, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES)
        {
            m_param_bind(name, type, MaxStringSize);
            return *this;
        }
        //-------------------------------------------------
        //�ֶ����Զ�������֮��,���Խ��в������ݵ�����
        template<class DT>
        stmt_t& operator<<(const DT& data)
        {
            param(m_cur_param_idx++) = data;
            return *this;
        }
        //-------------------------------------------------
        //��ȡ�������sql�������
        sql_stmt_t	sql_type() { return m_sql_type; }
        //-------------------------------------------------
        //�õ���������sql���
        const char* sql_string() { return m_SQL.c_str(); }
        //-------------------------------------------------
        //ִ�е�ǰԤ�����������,�����з��ؼ�¼���Ĵ���
        //���:��ǰʵ�ʰ󶨲������������.
        stmt_t& exec (ub2 BulkCount=0,bool auto_commit=false)
        {
            m_executed = false;
            rx_assert_if(m_cur_param_idx, m_cur_param_idx == m_params.size());//Ҫ���Զ�������������ŵ�ʱ��,���������������ͬ,����<<��ʱ��©������
            m_cur_param_idx = 0;

            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            rx_assert(BulkCount<=m_max_bulk_deep);
            
            m_last_bulk_deep = BulkCount;                  //��¼���ִ��ʱ������

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
                         BulkCount,	            //����������ʵ�����,selectҪ��Ϊ0,����Ϊ1��ʵ�ʰ����
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
        //Ԥ������ִ��ͬʱ����,�м�û�а󶨲����Ļ�����,�ʺϲ��󶨲��������
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
        //�õ���һ�����ִ�к�Ӱ�������(select��Ч)
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
        //�󶨹��Ĳ�������
        ub4 params() { return m_params.size(); }
        //��ȡ�󶨵Ĳ�������
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
        //�����������ʲ�������,������0��ʼ
        sql_param_t& param(ub4 Idx)
        {
            if (Idx>=m_params.size())
                throw (error_info_t (DBEC_IDX_OVERSTEP, __FILE__, __LINE__));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //�ͷ������,����󶨵Ĳ���
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
            {//�ͷ�sql�����
                OCIHandleFree(m_stmt_handle,OCI_HTYPE_STMT);
                m_stmt_handle = NULL;
            }
#else
            if (m_stmt_handle)
            {//�ͷ�sql�����
                OCIStmtRelease(m_stmt_handle, m_conn.m_handle_err,NULL,0, OCI_DEFAULT);
                m_stmt_handle = NULL;
            }
#endif
        }
    };

    //-----------------------------------------------------
    //�����ݿ����Ӷ������ֱ��ִ��sql���ķ���,�õ���HOStmt����,������Ҫ����HOStmt����ĺ���
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
