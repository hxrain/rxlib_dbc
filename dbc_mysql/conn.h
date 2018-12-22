
#ifndef	_RX_DBC_MYSQL_CONN_H_
#define	_RX_DBC_MYSQL_CONN_H_

namespace mysql
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
        MYSQL               m_handle;

        conn_t(const conn_t&);
        conn_t& operator = (const conn_t&);

    public:
        //-------------------------------------------------
        conn_t(rx::mem_allotter_i& ma = rx_global_mem_allotter()):m_mem(ma)
        {
            memset(&m_handle,0,sizeof(m_handle));
            m_is_valid = false;
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

            mysql_init(&m_handle);
            mysql_options(&m_handle, MYSQL_OPT_CONNECT_TIMEOUT, &dst.conn_timeout);
            mysql_options(&m_handle, MYSQL_OPT_READ_TIMEOUT, &rw_timeout_sec);
            mysql_options(&m_handle, MYSQL_OPT_WRITE_TIMEOUT, &rw_timeout_sec);
            MYSQL *rc=mysql_real_connect(&m_handle, dst.host, dst.user, dst.pwd, dst.db, dst.port, NULL, 0);
            if (!rc)
                throw (error_info_t(&m_handle, __FILE__, __LINE__));

            //��oraģʽһ��,Ĭ�ϲ������Զ��ύ,��Ҫ��ȷ���ֶ��ύ
            if (mysql_autocommit(&m_handle, false))
                throw (error_info_t(&m_handle, __FILE__, __LINE__));

            //�������ûỰ�ַ���
            if (!is_empty(op.charset))
                exec("SET NAMES %s", op.charset);

            //�������ûỰ������ʾ����
            if (!is_empty(op.language))
                exec("SET SESSION lc_messages = '%s'", op.language);

            m_is_valid = true;
            return 0;
        }
        //-------------------------------------------------
        //�رյ�ǰ������(�����׳��쳣)
        bool close (void)
        {
            bool ret = m_is_valid;
            if (m_handle.host)
            {
                mysql_close(&m_handle);
                m_handle.host=NULL;
            }

            m_is_valid = false;
            return ret;
        }
        //-------------------------------------------------
        //ִ��һ��û�н������(��SELECT)��sql���
        //�ڲ�ʵ���ǽ�������ʱ��OiCommand����,����Ƶ��ִ�еĶ���������ʹ�ô˺���
        void exec(const char *sql, ...)
        {
            rx::tiny_string_t<char, 1024> SQL;
            va_list arg;
            va_start(arg,sql);
            SQL.fmt(sql, arg);
            va_end(arg);
            if (mysql_query(&m_handle,SQL.c_str())!=0)
                throw (error_info_t(&m_handle, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //�л���ָ�����û�ר����
        void schema_to(const char *schema) { exec("use %s", schema); }
        //-------------------------------------------------
        //��ǰ������������.����װʹ���˷��Զ��ύģʽ,��ʽ������������������⴦��.
        void trans_begin() { rx_assert(m_is_valid); }
        //-------------------------------------------------
        //�ύ��ǰ����
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            if (mysql_commit(&m_handle))
                throw (error_info_t(&m_handle, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //�ع���ǰ����
        //����ֵ:�������(�ع��������ʱ�����׳��쳣)
        bool trans_rollback (int32_t *ec=NULL)
        {
            rx_assert(m_is_valid);
            bool rc = mysql_rollback(&m_handle)==0;
            if (!rc)
            {
                char tmp[1024];
                int32_t ec = 0;
                get_last_error(ec,tmp,sizeof(tmp));
            }
            return rc;
        }
        //-------------------------------------------------
        //���з�����ping���,��ʵ���ж������Ƿ���Ч(�����׳��쳣)
        bool ping()
        {
            rx_assert(m_is_valid);
            return  mysql_ping(&m_handle)==0;
        }
        //-------------------------------------------------
        //��ȡ����oci�����ec,���Ӧ�Ĵ�������
        //����ֵ:�������(����ʱ�����׳��쳣)
        bool get_last_error(int32_t &ec,char *buff,uint32_t max_size)
        {
            rx_assert(m_is_valid);
            return mysql::get_last_error(ec, buff, max_size, &m_handle);
        }
    };
}

#endif
