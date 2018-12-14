#ifndef	_RX_DBC_PGSQL_STATEMENT_H_
#define	_RX_DBC_PGSQL_STATEMENT_H_


namespace pgsql
{
    //-----------------------------------------------------
    //ִ��sql���Ĺ�����
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
        //��������pg���ִ�ж������з�װ,���ڽ��н������.
        class raw_stmt_t
        {
            PGresult               *m_res;                  //ִ�н��
            stmt_t                 &parent;                 //���ڷ��ʸ������
            char                    m_pre_name[36];         //Ԥ����sql���ı���
            //----------------------------------------------
            void m_check_error()
            {
                if (!m_res)                                 //���ִ�����
                    throw (error_info_t(DBEC_DB_CONNLOST, __FILE__, __LINE__,"SQL:{ %s }",parent.m_SQL.c_str()));

                //��ȡִ�н��
                ::ExecStatusType ec = ::PQresultStatus(m_res);
                if (ec != PGRES_TUPLES_OK && ec != PGRES_COMMAND_OK)
                {//���ɹ�,�ͽ��д�����Ϣ��¼
                    rx::tiny_string_t<char, 1024> tmp;
                    tmp = ::PQresultErrorMessage(m_res);
                    reset();                                //��������ִ�н�������,���׳������쳣
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
            //����ִ�н��ָ��
            PGresult* res() { return m_res; }
            //��ȡԤ����ʱ���ɵ��������
            const char* pre_name() const { return m_pre_name; }
            //----------------------------------------------
            //����������Ԥ����,��sql�������Ϣ���͸�������,���õ�ִ�н��
            void prepare()
            {
                reset();
                m_pre_name[0] = 0;
                if (!parent.m_SQL.size())
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is empty!"));

                rx::string_alias4x8(parent.m_SQL.c_str(), m_pre_name);  //�Զ�����sql����Ӧ�ı���

                //���ͽ�������
                m_res = ::PQprepare(parent.m_conn.m_handle, m_pre_name, parent.m_SQL.c_str(), parent.m_params.size(), (Oid*)parent.m_mi_oids.array());
                m_check_error();
            }
            //----------------------------------------------
            //Ԥ������ִ�ж���,��������Ӧ�����ݷ��͸�������,�õ�ִ�н��
            void prepare_exec(bool auto_commit)
            {
                reset();
                if (is_empty(m_pre_name))
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not prepared!"));
                if (auto_commit)                            //���Ҫ���Զ��ύ,�Ͳ�Ҫ���Խ����Զ�����
                    parent.m_conn.try_auto_trans(parent.m_SQL.c_str());
                m_res = ::PQexecPrepared(parent.m_conn.m_handle, m_pre_name, parent.m_params.size(), parent.m_mi_vals.array(), NULL, NULL, 0);
                m_check_error();
            }
            //----------------------------------------------
            //������Ԥ����,ֱ�ӽ��������ݺ�sql��䷢�͸�������,�õ�ִ�н��
            void params_exec(bool auto_commit)
            {
                reset(true);
                if (!parent.m_SQL.size())
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is empty!"));
                if (auto_commit)                            //���Ҫ���Զ��ύ,�Ͳ�Ҫ���Խ����Զ�����
                    parent.m_conn.try_auto_trans(parent.m_SQL.c_str());
                m_res = ::PQexecParams(parent.m_conn.m_handle, parent.m_SQL.c_str(), parent.m_params.size(), (Oid*)parent.m_mi_oids.array(), parent.m_mi_vals.array(), NULL, NULL, 0);
                m_check_error();
            }
            //----------------------------------------------
            //�����ͷ�֮ǰִ�еĽ��
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
        conn_t		           &m_conn;		                //����������������ݿ����Ӷ���
        mi_oid_array_t          m_mi_oids;                  //pgsql��Ҫ�İ���Ϣ��������
        mi_val_array_t          m_mi_vals;                  //pgsql��Ҫ�İ���Ϣ��ֵ����
        param_array_t	        m_params;	                //�������ư󶨵Ĳ�������
        sql_type_t	            m_sql_type;                 //��������ǰsql��������
        sql_string_t            m_SQL;                      //Ԥ����ʱ��¼�Ĵ�ִ�е�sql���
        sql_string_t            m_SQL_BAK;                  //Ԥ����ʱ��¼��ԭʼ��sql���
        uint32_t                m_cur_param_idx;            //�����ݵ�ʱ������˳����������
        bool			        m_executed;                 //��ǵ�ǰ����Ƿ��Ѿ�����ȷִ�й���
        raw_stmt_t              m_raw_stmt;                 //�ײ����ִ�ж���
        sql_param_parse_t<>     m_sp;                       //SQL����������
        //-------------------------------------------------
        //�󶨲�����ʼ��:����������(�������֪,���Զ�����sql��������ȡ);
        void m_param_make(uint16_t ParamCount)
        {
            if (!ParamCount) return;

            if (!m_params.make(ParamCount,true))            //���ɰ󶨲������������
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            if (!m_mi_oids.make(ParamCount, true))          //���ɰ󶨲���Ԫ��Ϣ��������
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            if (!m_mi_vals.make(ParamCount, true))          //���ɰ󶨲���Ԫ��Ϣ��ֵ����
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            m_mi_oids.set(0);
            m_mi_vals.set(0);
        }
        //-------------------------------------------------
        //��һ��������������ǰ�����
        param_t& m_param_bind(const char *name,int pg_data_type)
        {
            rx_assert(!is_empty(name));

            char Tmp[200];
            rx::st::strlwr(name, Tmp);

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
            param_t &p = m_params[ParamIdx];                //�õ���������

            //�Բ���������б�Ҫ�ĳ�ʼ��
            p.bind(m_mi_vals.array(),ParamIdx,&m_mi_oids.at(ParamIdx),name, pg_data_type);

            if (m_params.size() == m_sp.count)
                m_raw_stmt.prepare();                       //�󶨲�������,˳��ִ��������Ԥ����

            return p;
        }
        //-------------------------------------------------
        //�ֶ�ģʽ,�������ֻ�ȡ����
        int m_data_type_by_name(const char* name)
        {
            if (name == NULL)
                return PG_DATA_TYPE_UNKNOW;
            if (name[0] == ':')                             //ora��ʽ������
                return  pg_data_type_by_dbc(get_data_type_by_name(name));
            else
            {//pg��ʽ������
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
        //Ԥ����һ��sql���,�õ���Ҫ����Ϣ,֮����Խ��в�����(����auto_bind�Զ��󶨻��ߵ���(name,data)�ֶ���)
        stmt_t& prepare(const char *sql,va_list arg)
        {
            rx_assert(!is_empty(sql));

            //����֮ǰ��״̬
            close(true);

            //����ִ�������뱸�ݻ�����
            if (!m_SQL_BAK.fmt(sql, arg))
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));
            
            //���Խ���ora�󶨱�����ʽ�Ľ���,ʵ���ⲿsql�󶨸�ʽ��ͳһ
            m_sp.ora_sql(m_SQL_BAK.c_str());

            if (m_sp.count)
            {//���ȷʵ��oraģʽ��sql���,����Ҫת��Ϊpg��ʽ
                rx::tiny_string_t<> dst(m_SQL.capacity(), m_SQL.ptr());
                m_sp.ora2pgsql(m_SQL_BAK.c_str(), dst);
                m_SQL.end(dst.size());
            }
            else
            {//����oraģʽ,���Խ���pg��ʽ��sql�󶨱���������
                m_sp.pg_sql(m_SQL_BAK.c_str());
                //��¼���տ�ִ�����
                m_SQL = m_SQL_BAK;
            }

            m_sql_type = get_sql_type(m_SQL);

            if (m_sp.count)
                m_param_make(m_sp.count);                   //�д��󶨲���,���в�����������ķ���
            else
                m_raw_stmt.prepare();                       //û�д��󶨲���,ֱ�ӽ���Ԥ����

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
        stmt_t& auto_bind(int dummy=0)
        {
            if (m_sql_type == ST_UNKNOWN)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not prepared!"));

            //�Զ�����Ҫ����֮ǰԤ����ʱ�Ľ��,���û�в����󶨵�����,�򷵻�
            if (m_sp.count==0)
                return *this;

            rx_assert(is_empty(m_raw_stmt.pre_name()));

            char name[FIELD_NAME_LENGTH];
            bool is_ora_name = m_sp.segs[0].name[0] == ':';
            int pg_data_type;

            for (uint16_t i = 0; i < m_sp.count; ++i)
            {//ѭ�����в���������Զ���
                sql_param_parse_t<>::param_seg_t &s = m_sp.segs[i];
                
                //��ȡ��������,pg��ʽ����и�����������,��Ҳ��Ϊ�������Ƶ�һ����
                rx::st::strcpy(name, sizeof(name), s.name, s.length);

                if (is_ora_name)                            //����ora��������ʽ,ת��Ϊpg��������
                    pg_data_type = pg_data_type_by_dbc(get_data_type_by_name(name));
                else if (s.offset == 0)                     //���pg������ģʽ,��û�и��ӵ�����,��ֱ�Ӹ���δ֪����
                    pg_data_type = PG_DATA_TYPE_UNKNOW; 
                else
                {//���ھ���Ҫ���Ը���pg������������������ȡ����
                    rx_assert(s.name[0]=='$');
                    pg_data_type = pg_data_type_by_name(s.name+s.offset);
                }
                    
                m_param_bind(name, pg_data_type);           //�󶨲����ķ����ڲ��᳢�Խ����Զ�Ԥ��������
            }

            return *this;
        }
        //-------------------------------------------------
        //��Ԥ������ɺ�,���Գ��Խ����ֶ�������,������������.
        stmt_t& manual_bind(uint16_t dummy=0, uint16_t params = 0)
        {
            if (m_sql_type == ST_UNKNOWN)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not prepared!"));

            if (m_sp.count == 0 && params == 0)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql manual bind error!"));
            
            if (params > m_sp.count)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql manual bind params error!"));

            m_param_make(params);                           //���Խ��в������������
            m_sp.count = params;                            //��¼�����ֶ���֪�Ĳ�������

            return *this;
        }
        //-------------------------------------------------
        //��ָ�������İ��뵱ǰ��ȵ����ݸ�ֵͬʱ����,����Ӧ�ò����
        template<class DT>
        stmt_t& operator()(const char* name, const DT& data)
        {
            param_t &param = m_param_bind(name, m_data_type_by_name(name));
            param = data;
            return *this;
        }
        //-------------------------------------------------
        //�ֶ����в����İ�
        stmt_t& operator()(const char* name)
        {
            m_param_bind(name, m_data_type_by_name(name));
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
        sql_type_t	sql_type() { return m_sql_type; }
        //-------------------------------------------------
        //�õ���������sql���
        const char* sql_string() { return m_SQL; }
        //-------------------------------------------------
        //ִ�е�ǰԤ�����������,�����з��ؼ�¼���Ĵ���
        stmt_t& exec (bool auto_commit=false)
        {
            m_executed = false;
            m_cur_param_idx = 0;
            m_raw_stmt.prepare_exec(auto_commit);
            m_executed = true;
            return *this;
        }
        //-------------------------------------------------
        //Ԥ������ִ��ͬʱ����,�м�û�а󶨲����Ļ�����,�ʺϲ��󶨲��������
        stmt_t& exec (const char *sql,...)
        {
            rx_assert(!is_empty(sql));
            //����֮ǰ��״̬
            close(true);

            va_list	arg;
            va_start(arg, sql);
            //����ִ�������뱸�ݻ�����
            bool rc = m_SQL_BAK.fmt(sql, arg);
            va_end(arg);
            if (!rc)
                throw (error_info_t(DBEC_NO_BUFFER, __FILE__, __LINE__, sql));

            m_SQL = m_SQL_BAK;

            m_raw_stmt.params_exec(false);
            
            return *this;
        }
        //-------------------------------------------------
        //�õ���һ�����ִ�к�Ӱ�������(select��Ч)
        uint32_t rows()
        {
            if (!m_executed)
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__, "sql is not executed!"));
            const char *msg = ::PQcmdTuples(m_raw_stmt.res());
            return (uint32_t)rx::st::atoi(msg);
        }
        //-------------------------------------------------
        //�󶨹��Ĳ�������
        uint32_t params() { return m_params.size(); }
        //��ȡ�󶨵Ĳ�������
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
        //�����������ʲ�������,������0��ʼ
        param_t& param(uint32_t Idx)
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
            {//���Ǹ�λ,��ǿ��Ҫ���ͷ��ڴ�
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
