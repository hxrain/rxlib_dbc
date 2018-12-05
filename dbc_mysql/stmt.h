#ifndef	_RX_DBC_MYSQL_STATEMENT_H_
#define	_RX_DBC_MYSQL_STATEMENT_H_


namespace rx_dbc_mysql
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
        uint16_t                            m_max_bulk_deep;//�������������ύ�������
        uint16_t                            m_cur_bulk_idx; //��ǰ�����Ŀ��������
        bool			                    m_executed;     //��ǵ�ǰ����Ƿ��Ѿ�����ȷִ�й���
        uint16_t                            m_cur_param_idx;//��ǰ���ڰ󶨴���Ĳ���˳��
        uint16_t                            m_last_bulk_deep;//��¼���һ��execʱ����Ŀ����,�������������������
        //-------------------------------------------------
        //Ԥ����һ��sql���,�õ���Ҫ����Ϣ,֮����Խ��в�������
        void m_prepare()
        {
            rx_assert(m_SQL.size()!=0);
            int16_t result;
            close(true);                                    //�����ܶ�����,��λ������

            if (result != OCI_SUCCESS)
            {
                close(true);
                throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__,m_SQL.c_str()));
            }
        }
        //-------------------------------------------------
        sql_stmt_t m_get_sql_type()
        {
            char tmp[5];
            rx::st::strncpy(tmp, m_SQL.c_str(), 4);
            rx::st::strupr(tmp);
            switch (*(uint32_t*)tmp)
            {
                case 'SELE':return ST_SELECT;
                case 'UPDA':return ST_UPDATE;
                case 'DELE':return ST_DELETE;
                case 'INSE':return ST_INSERT;
                case 'CREA':return ST_CREATE;
                case 'DROP':return ST_DROP  ;
                case 'ALTE':return ST_ALTER ;
                case 'BEGI':return ST_BEGIN ;
                case 'DECL':return ST_DECLARE;
                case 'SET ':return ST_SET;
                default:return ST_UNKNOWN;
            }
        }
        //-------------------------------------------------
        //�󶨲�����ʼ��:�������������������(��exec��ʱ����Ը�֪ʵ������);����������(�������֪,���Զ�����sql��������ȡ);
        //���Ҫʹ��Bulkģʽ��������,��ô�����ȵ��ô˺���,��֪ÿ��Bulk�����Ԫ������
        void m_param_make(uint16_t max_bulk_deep, uint16_t ParamCount = 0)
        {
            rx_assert(max_bulk_deep != 0);

            if (ParamCount == 0)    //���Ը���sql�еĲ����������г�ʼ��.�жϲ��������ͼ򵥵�����':'������,����ֻ�಻��,�ǿ��Ե�
                ParamCount = rx::st::count(m_SQL.c_str(), ':');

            if (ParamCount && !m_params.make_ex(ParamCount,true))    //���ɰ󶨲������������
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL.c_str()));

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

            uint32_t ParamIdx = m_params.index(Tmp);
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
        stmt_t& auto_bind(uint16_t max_bulk_deep = 0)
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
            for (uint16_t i = 0; i < sp.count; ++i)
            {//ѭ�����в���������Զ���
                rx::st::strcpy(name, sizeof(name), sp.segs[i].name, sp.segs[i].name_len);
                m_param_bind(name);
            }
            return *this;
        }
        //-------------------------------------------------
        //��Ԥ������ɺ�,����������Զ�������Գ��Խ����ֶ�������,��֪�����ֵ.
        //ʡȥ���ٴε���(name,data)��ʱ��������ֵ��鷳,����ֱ��ʹ��<<data�������ݵİ�
        stmt_t& manual_bind(uint16_t max_bulk_deep, uint16_t params = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            if (max_bulk_deep == 0) max_bulk_deep = m_max_bulk_deep;
            m_param_make(max_bulk_deep, params);            //���Խ��в������������

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
        stmt_t& exec (bool auto_commit=false)
        {
            m_executed = false;
            rx_assert_if(m_cur_param_idx, m_cur_param_idx == m_params.size());//Ҫ���Զ�������������ŵ�ʱ��,���������������ͬ,����<<��ʱ��©������
            m_cur_param_idx = 0;

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
        uint32_t rows()
        {
            if (!m_executed)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Executed!"));
            uint32_t RC=0;

            return RC;
        }
        //-------------------------------------------------
        //�󶨹��Ĳ�������
        uint32_t params() { return m_params.size(); }
        //��ȡ�󶨵Ĳ�������
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
        //�����������ʲ�������,������0��ʼ
        sql_param_t& param(uint32_t Idx)
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

            if (m_stmt_handle)
            {//�ͷ�sql�����
                OCIStmtRelease(m_stmt_handle, m_conn.m_handle_err,NULL,0, OCI_DEFAULT);
                m_stmt_handle = NULL;
            }
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
