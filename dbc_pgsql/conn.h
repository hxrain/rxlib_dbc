
#ifndef	_RX_DBC_PGSQL_CONN_H_
#define	_RX_DBC_PGSQL_CONN_H_

namespace pgsql
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
        PGconn             *m_handle;
        conn_t(const conn_t&);
        conn_t& operator = (const conn_t&);

        //-------------------------------------------------
        //执行一条语句
        void m_exec(const char *sql)
        {
            if (!m_handle)                                  //执行顺序错误,连接尚未建立
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));
            //执行语句
            PGresult *res = ::PQexec(m_handle, sql);
            if (!res)                                       //出现错误了
                throw (error_info_t(DBEC_DB_CONNLOST, __FILE__, __LINE__));

            //获取执行结果
            ::ExecStatusType ec = ::PQresultStatus(res);
            if (ec != PGRES_COMMAND_OK)
            {//如果不是无结果集命令成功,就进行错误信息记录
                rx::tiny_string_t<char, 1024> tmp;
                tmp = ::PQresultErrorMessage(res);
                ::PQclear(res);                             //必须清理执行结果对象后,再抛出错误异常
                throw (error_info_t(tmp.c_str(), __FILE__, __LINE__));
            }
            ::PQclear(res);                                 //正常执行完成后也必须清理执行结果对象
        }
    public:
        //-------------------------------------------------
        conn_t(rx::mem_allotter_i& ma = rx_global_mem_allotter()):m_mem(ma), m_handle(NULL)
        {
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
            char conn_uri[1024];
            rx::st::snprintf(conn_uri,sizeof(conn_uri),"postgresql://%s:%s@%s:%u/%s?connect_timeout=%u", dst.user, dst.pwd, dst.host, dst.port, dst.db, dst.conn_timeout);
            m_handle = PQconnectdb(conn_uri);
            if (!m_handle)
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__));
            if (::PQstatus(m_handle)!= CONNECTION_OK)
                throw (error_info_t(m_handle, __FILE__, __LINE__));

            //尝试设置会话字符集
            if (!is_empty(op.charset))
                exec("SET client_encoding = %s", op.charset);

            //尝试设置会话交互提示语言
            if (!is_empty(op.language))
                exec("SET lc_messages = '%s'", op.language);

            //默认日期格式化函数的格式
            try { exec("SET DateStyle='ISO,YMD'"); } catch (...) {}

            m_is_valid = true;
            return 0;
        }
        //-------------------------------------------------
        //关闭当前的连接(不会抛出异常)
        bool close (void)
        {
            if (m_handle)
            {
                ::PQfinish(m_handle);
                m_handle = NULL;
                m_is_valid = false;
                return true;
            }
            return false;
        }
        //-------------------------------------------------
        //执行一条没有结果返回(非SELECT)的sql语句
        //内部实现是建立了临时的OiCommand对象,对于频繁执行的动作不建议使用此函数
        void exec(const char *sql, ...)
        {
            //进行语句格式化
            rx::tiny_string_t<char, 1024> SQL;
            va_list arg;
            va_start(arg,sql);
            SQL.fmt(sql, arg);
            va_end(arg);
            m_exec(SQL);
        }
        //-------------------------------------------------
        //切换到指定的用户专属库
        void schema_to(const char *schema) { exec("set search_path = '%s'", schema); }
        //-------------------------------------------------
        //当前连接明确地启动事务
        void trans_begin() 
        { 
            if (!m_handle)                                  //执行顺序错误,连接尚未建立
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));

            PGTransactionStatusType ts = ::PQtransactionStatus(m_handle);
            if (ts == PQTRANS_IDLE)                         //完全空闲的时候,才可以启动事务
                m_exec("begin");
            else if (ts == PQTRANS_INERROR)                 //有之前的执行错误未进行回滚处理
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //提交当前事务
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            if (::PQtransactionStatus(m_handle) == PQTRANS_IDLE)
                return;
            m_exec("commit");
        }
        //-------------------------------------------------
        //回滚当前事务
        //返回值:操作结果(回滚本身出错时不再抛出异常)
        bool trans_rollback (int32_t *ec=NULL)
        {
            if (::PQtransactionStatus(m_handle) == PQTRANS_IDLE)
                return true;

            try { m_exec("rollback"); return true; }
            catch (error_info_t &e) { e.c_str(); return false; }
        }
        //-------------------------------------------------
        //进行服务器ping检查,真实的判断连接是否有效(不会抛出异常)
        bool ping()
        {
            try { exec("set session _tmp_._ping_ = 1"); return true; }
            catch (error_info_t &e) { e.c_str(); return false; }
        }
        //-------------------------------------------------
        //获取最后的dbc错误号ec,与对应的错误描述
        //返回值:操作结果(出错时不再抛出异常)
        bool get_last_error(int32_t &ec,char *buff,uint32_t max_size)
        {
            return pgsql::get_last_error(ec, buff, max_size, m_handle);
        }
    };
}

#endif
