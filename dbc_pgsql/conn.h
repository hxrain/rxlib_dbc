
#ifndef	_RX_DBC_PGSQL_CONN_H_
#define	_RX_DBC_PGSQL_CONN_H_

namespace pgsql
{
    //-----------------------------------------------------
    //����db���ӵĹ��ܶ���
    class conn_t
    {
    private:
        friend class stmt_t;
        friend class param_t;
        friend class query_t;
        friend class field_t;

        rx::mem_allotter_i &m_mem;                          //�ڴ������
        bool		        m_is_valid;
        PGconn             *m_handle;
        bool                m_auto_trans;                   //�Ƿ���Ҫ�ֶ�����beginָ��,��ɷ��Զ��ύ(������ʽ�Զ�����)��ģʽ(pg9.5֮����Ҫ)
        conn_t(const conn_t&);
        conn_t& operator = (const conn_t&);

        //-------------------------------------------------
        //ִ��һ�����
        void m_exec(const char *sql)
        {
            if (!m_handle)                                  //ִ��˳�����,������δ����
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));
            //ִ�����
            PGresult *res = ::PQexec(m_handle, sql);
            if (!res)                                       //���ִ�����
                throw (error_info_t(DBEC_DB_CONNLOST, __FILE__, __LINE__));

            //��ȡִ�н��
            ::ExecStatusType ec = ::PQresultStatus(res);
            if (ec != PGRES_COMMAND_OK)
            {//��������޽��������ɹ�,�ͽ��д�����Ϣ��¼
                rx::tiny_string_t<char, 1024> tmp;
                tmp = ::PQresultErrorMessage(res);
                ::PQclear(res);                             //��������ִ�н�������,���׳������쳣
                throw (error_info_t(tmp.c_str(), __FILE__, __LINE__));
            }
            ::PQclear(res);                                 //����ִ����ɺ�Ҳ��������ִ�н������
        }
        //-------------------------------------------------
        //���ݸ�����sql����ж��Ƿ�Ӧ�Զ���������;�������ʱ����ֱ����������
        //���д�������ִ�е���䶯��,��Ӧ��������execǰ���ô˷���,������ȷ����ʽ�Զ�����Ŀ���
        void do_auto_begin(const char* SQL)
        {
            if (!is_empty(SQL))
            {//����sql��ʱ��,�ж��Ƿ���Ҫ�����Զ�����
                if (!m_auto_trans)
                    return;

                sql_type_t st = get_sql_type(SQL);
                switch (st)
                {
                case ST_UNKNOWN:
                case ST_DECLARE:
                case ST_SELECT:
                case ST_SET:
                    return;
                case ST_BEGIN:                              //ִ��˳�����,���治Ӧ����������������ķ���
                    throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));
                }
            }

            if (!m_handle)                                  //ִ��˳�����,������δ����
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));

            PGTransactionStatusType ts = ::PQtransactionStatus(m_handle);
            if (ts == PQTRANS_IDLE)                         //��ȫ���е�ʱ��,�ſ�����������
                m_exec("begin");
            else if (ts == PQTRANS_INERROR)                 //��֮ǰ��ִ�д���δ���лع�����
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));
        }
    public:
        //-------------------------------------------------
        conn_t(rx::mem_allotter_i& ma = rx_global_mem_allotter()):m_mem(ma), m_handle(NULL)
        {
            m_is_valid = false;
            m_auto_trans = false;
        }
        ~conn_t (){close();}
        //-------------------------------------------------
        bool is_valid(){return m_is_valid;}
        //-------------------------------------------------
        //���ӵ�db������(���ֲ��ɴ�������ʱ���쳣,�ɿ�����ʱ����ora�������)
        //����ֵ:0����;
        int16_t open(const conn_param_t& dst, const env_option_t &op = env_option_t(),uint32_t rw_timeout_sec=10)
        {
            if (is_empty(dst.host) || is_empty(dst.user) || is_empty(dst.db))
                throw (error_info_t (DBEC_BAD_PARAM, __FILE__, __LINE__));
                
            //ÿ������ǰ���ȳ��Թر�֮ǰ������
            close();
            char conn_uri[1024];
            rx::st::snprintf(conn_uri,sizeof(conn_uri),"postgresql://%s:%s@%s:%u/%s?connect_timeout=%u", dst.user, dst.pwd, dst.host, dst.port, dst.db, dst.conn_timeout);
            m_handle = PQconnectdb(conn_uri);
            if (!m_handle)
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__));
            if (::PQstatus(m_handle)!= CONNECTION_OK)
                throw (error_info_t(m_handle, __FILE__, __LINE__));

            //�������ûỰ�ַ���
            if (!is_empty(op.charset))
                exec("SET client_encoding = %s", op.charset);

            //�������ûỰ������ʾ����
            if (!is_empty(op.language))
                exec("SET lc_messages = '%s'", op.language);

            //������ʾ��ʽ
            exec("SET DateStyle='ISO,YMD'", op.charset);

            //��oraģʽһ��,Ĭ�ϲ������Զ��ύ,��Ҫ��ȷ���ֶ��ύ.(pgsql 9.5֮������˴�����)
            try { exec("SET AUTOCOMMIT = OFF"); }
            catch (...) { m_auto_trans = true; }

            m_is_valid = true;
            return 0;
        }
        //-------------------------------------------------
        //�رյ�ǰ������(�����׳��쳣)
        bool close (void)
        {
            if (m_handle)
            {
                ::PQfinish(m_handle);
                m_handle = NULL;
                m_is_valid = false;
                m_auto_trans = false;
                return true;
            }
            return false;
        }
        //-------------------------------------------------
        //ִ��һ��û�н������(��SELECT)��sql���
        //�ڲ�ʵ���ǽ�������ʱ��OiCommand����,����Ƶ��ִ�еĶ���������ʹ�ô˺���
        void exec(const char *sql, ...)
        {
            //��������ʽ��
            rx::tiny_string_t<char, 1024> SQL;
            va_list arg;
            va_start(arg,sql);
            SQL.fmt(sql, arg);
            va_end(arg);

            do_auto_begin(SQL);
            m_exec(SQL);
        }
        //-------------------------------------------------
        //�л���ָ�����û�ר����
        void schema_to(const char *schema) { exec("set search_path = '%s'", schema); }
        //-------------------------------------------------
        //��ǰ������ȷ����������
        void trans_begin() { do_auto_begin(NULL); }
        //-------------------------------------------------
        //�ύ��ǰ����
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            m_exec("commit");
        }
        //-------------------------------------------------
        //�ع���ǰ����
        //����ֵ:�������(�ع��������ʱ�����׳��쳣)
        bool trans_rollback (int32_t *ec=NULL)
        {
            bool rc;
            try { m_exec("rollback"); rc=true; }
            catch (error_info_t &e) { e.c_str(); rc=false; }
            return rc;
        }
        //-------------------------------------------------
        //���з�����ping���,��ʵ���ж������Ƿ���Ч(�����׳��쳣)
        bool ping()
        {
            try { exec("set session tmp.rx_dbc_pgsql_ping_test = 1"); return true; }
            catch (error_info_t &e) { e.c_str(); return false; }
        }
        //-------------------------------------------------
        //��ȡ����oci�����ec,���Ӧ�Ĵ�������
        //����ֵ:�������(����ʱ�����׳��쳣)
        bool get_last_error(int32_t &ec,char *buff,uint32_t max_size)
        {
            return pgsql::get_last_error(ec, buff, max_size, m_handle);
        }
    };
}

#endif
