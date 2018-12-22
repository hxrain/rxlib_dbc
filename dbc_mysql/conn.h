
#ifndef	_RX_DBC_MYSQL_CONN_H_
#define	_RX_DBC_MYSQL_CONN_H_

namespace mysql
{
    //-----------------------------------------------------
    //管理db连接的功能对象
    class conn_t
    {
    private:
        friend class stmt_t;
        friend class param_t;
        friend class query_t;
        friend class field_t;

        rx::mem_allotter_i &m_mem;                          //内存分配器
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
        //连接到db服务器(出现不可处理问题时抛异常,可控问题时给出ora错误代码)
        //返回值:0正常;
        int16_t open(const conn_param_t& dst, const env_option_t &op = env_option_t(),uint32_t rw_timeout_sec=10)
        {
            if (is_empty(dst.host) || is_empty(dst.user) || is_empty(dst.db))
                throw (error_info_t (DBEC_BAD_PARAM, __FILE__, __LINE__));

            //每次连接前都先尝试关闭之前的连接
            close();

            mysql_init(&m_handle);
            mysql_options(&m_handle, MYSQL_OPT_CONNECT_TIMEOUT, &dst.conn_timeout);
            mysql_options(&m_handle, MYSQL_OPT_READ_TIMEOUT, &rw_timeout_sec);
            mysql_options(&m_handle, MYSQL_OPT_WRITE_TIMEOUT, &rw_timeout_sec);
            MYSQL *rc=mysql_real_connect(&m_handle, dst.host, dst.user, dst.pwd, dst.db, dst.port, NULL, 0);
            if (!rc)
                throw (error_info_t(&m_handle, __FILE__, __LINE__));

            //与ora模式一致,默认不进行自动提交,需要明确的手动提交
            if (mysql_autocommit(&m_handle, false))
                throw (error_info_t(&m_handle, __FILE__, __LINE__));

            //尝试设置会话字符集
            if (!is_empty(op.charset))
                exec("SET NAMES %s", op.charset);

            //尝试设置会话交互提示语言
            if (!is_empty(op.language))
                exec("SET SESSION lc_messages = '%s'", op.language);

            m_is_valid = true;
            return 0;
        }
        //-------------------------------------------------
        //关闭当前的连接(不会抛出异常)
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
        //执行一条没有结果返回(非SELECT)的sql语句
        //内部实现是建立了临时的OiCommand对象,对于频繁执行的动作不建议使用此函数
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
        //切换到指定的用户专属库
        void schema_to(const char *schema) { exec("use %s", schema); }
        //-------------------------------------------------
        //当前连接启动事务.本封装使用了非自动提交模式,显式事务的启动就无需特殊处理.
        void trans_begin() { rx_assert(m_is_valid); }
        //-------------------------------------------------
        //提交当前事务
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            if (mysql_commit(&m_handle))
                throw (error_info_t(&m_handle, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //回滚当前事务
        //返回值:操作结果(回滚本身出错时不再抛出异常)
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
        //进行服务器ping检查,真实的判断连接是否有效(不会抛出异常)
        bool ping()
        {
            rx_assert(m_is_valid);
            return  mysql_ping(&m_handle)==0;
        }
        //-------------------------------------------------
        //获取最后的oci错误号ec,与对应的错误描述
        //返回值:操作结果(出错时不再抛出异常)
        bool get_last_error(int32_t &ec,char *buff,uint32_t max_size)
        {
            rx_assert(m_is_valid);
            return mysql::get_last_error(ec, buff, max_size, &m_handle);
        }
    };
}

#endif
