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
        typedef rx::array_t<MYSQL_BIND> mi_array_t;
        typedef rx::tiny_string_t<char, MAX_SQL_LENGTH> sql_string_t;
    protected:
        conn_t		           &m_conn;		                //����������������ݿ����Ӷ���
        mi_array_t              m_metainfos;                //mysql��Ҫ�İ���Ϣ�ṹ����
        param_array_t	        m_params;	                //�������ư󶨵Ĳ�������
        sql_stmt_t	            m_sql_type;                 //��������ǰsql��������
        sql_string_t            m_SQL;                      //Ԥ����ʱ��¼�Ĵ�ִ�е�sql���
        sql_string_t            m_SQL_BAK;                  //Ԥ����ʱ��¼��ԭʼ��sql���
        MYSQL_STMT	           *m_stmt_handle;              //���������mysql���
        uint32_t                m_cur_param_idx;            //�����ݵ�ʱ������˳����������
        bool			        m_executed;                 //��ǵ�ǰ����Ƿ��Ѿ�����ȷִ�й���
        //-------------------------------------------------
        //Ԥ����һ��sql���,�õ���Ҫ����Ϣ,֮����Խ��в�������
        void m_prepare()
        {
            rx_assert(!is_empty(m_SQL));
            
            close(true);                                    //�����ܶ�����,��λ������
            m_stmt_handle = mysql_stmt_init(&m_conn.m_handle);

            if (!m_stmt_handle)
                throw (error_info_t(&m_conn.m_handle, __FILE__, __LINE__,m_SQL));

            if (mysql_stmt_prepare(m_stmt_handle,m_SQL,m_SQL.size()))
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));

            m_sql_type = get_sql_type(m_SQL);
        }
        //-------------------------------------------------
        //�󶨲�����ʼ��:����������(�������֪,���Զ�����sql��������ȡ);
        void m_param_make(uint16_t ParamCount = 0)
        {
            if (ParamCount == 0)
                ParamCount = rx::st::count(m_SQL.c_str(), '?');

            if (!ParamCount) return;

            if (!m_params.make(ParamCount,true))            //���ɰ󶨲������������
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            if (!m_metainfos.make(ParamCount, true))        //���ɰ󶨲���Ԫ��Ϣ����
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));
        }
        //-------------------------------------------------
        //��һ��������������ǰ�����
        sql_param_t& m_param_bind(const char *name)
        {
            rx_assert(!is_empty(name));

            char Tmp[200];
            rx::st::strlwr(name, Tmp);

            if (m_params.capacity() == 0)
                m_param_make();                             //���Է����������Դ

            if (m_params.capacity() == 0)
                throw (error_info_t(DBEC_NOT_PARAM, __FILE__, __LINE__, m_SQL));

            uint32_t ParamIdx = m_params.index(Tmp);
            if (ParamIdx != m_params.capacity())
                return m_params[ParamIdx];                  //���������ظ���ʱ��,ֱ�ӷ���

            //���ڽ��������ֵİ�
            ParamIdx = m_params.size();                     //���ð󶨹���������Ϊ��������
            if (ParamIdx>= m_params.capacity())
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__, name));

            m_params.bind(ParamIdx, Tmp);                   //�������������������ֽ��й���
            sql_param_t &Ret = m_params[ParamIdx];          //�õ���������
            Ret.make(name, &m_metainfos.at(ParamIdx));      //�Բ���������б�Ҫ�ĳ�ʼ��
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
            if (!m_SQL_BAK.fmt(sql, arg))                   //�Ƚ���ִ�������뱸�ݻ�����
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));
            
            //�ٳ��Խ���ora�󶨱�������ת��
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

            //�����ִ��Ԥ����
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
        //��Ԥ������ɺ�,����ֱ�ӽ��в������Զ���
        //ʡȥ���ٴε���(name,data)��ʱ��������ֵ��鷳,����ֱ��ʹ��<<data�������ݵİ�
        stmt_t& auto_bind()
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            //�ȳ��Խ���oraģʽ����������
            sql_param_parse_t<> sp;
            const char* err = sp.ora_sql(m_SQL_BAK);
            if (err)
                throw (error_info_t(DBEC_PARSE_PARAM, __FILE__, __LINE__, "sql param parse error!| %s |",err));

            bool unnamed = false;
            if (sp.count == 0)
            {//�ٳ��Խ���mysqlģʽ��δ��������
                sp.count = rx::st::count(m_SQL.c_str(), '?');
                if (sp.count)
                    unnamed = true;
            }
            if (!sp.count)                                  //���ȷʵû�в����󶨵�����,�򷵻�
                return *this;

            m_param_make(sp.count);

            char name[FIELD_NAME_LENGTH];
            for (uint16_t i = 0; i < sp.count; ++i)
            {//ѭ�����в���������Զ���
                if (unnamed)
                    rx::st::itoa(i + 1, name);              //δ������ʱ��,ʹ����ŵ�������,��Ŵ�1��ʼ
                else
                    rx::st::strcpy(name, sizeof(name), sp.segs[i].name, sp.segs[i].name_len);
                m_param_bind(name);
            }
            return *this;
        }
        //-------------------------------------------------
        //��Ԥ������ɺ�,����������Զ�������Գ��Խ����ֶ�������,��֪�����ֵ.
        //ʡȥ���ٴε���(name,data)��ʱ��������ֵ��鷳,����ֱ��ʹ��<<data�������ݵİ�
        stmt_t& manual_bind(uint16_t params = 0)
        {
            if (m_sql_type == ST_UNKNOWN || m_stmt_handle == NULL)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Prepared!"));

            m_param_make(params);                           //���Խ��в������������

            return *this;
        }
        //-------------------------------------------------
        //��ָ�������İ��뵱ǰ��ȵ����ݸ�ֵͬʱ����,����Ӧ�ò����
        template<class DT>
        stmt_t& operator()(const char* name, const DT& data)
        {
            sql_param_t &param = m_param_bind(name);
            param = data;
            return *this;
        }
        //-------------------------------------------------
        //�ֶ����в����İ�
        stmt_t& operator()(const char* name)
        {
            m_param_bind(name);
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
        const char* sql_string() { return m_SQL; }
        //-------------------------------------------------
        //ִ�е�ǰԤ�����������,�����з��ؼ�¼���Ĵ���
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
            if (!m_executed||!m_stmt_handle)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql Is Not Executed!"));
            return (uint32_t)mysql_stmt_affected_rows(m_stmt_handle);
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
            if (!reset_only) 
                m_metainfos.clear();
            m_executed = false;
            m_cur_param_idx = 0;
            m_sql_type = ST_UNKNOWN;

            if (m_stmt_handle)
            {//�ͷ�sql�����
                mysql_stmt_close(m_stmt_handle);
                m_stmt_handle = NULL;
            }
        }
    };
}


#endif
