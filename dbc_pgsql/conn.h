
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

            //与ora模式一致,默认不进行自动提交,需要明确的手动提交
            exec("SET AUTOCOMMIT = OFF");

            //尝试设置会话字符集
            if (!is_empty(op.charset))
                exec("SET NAMES %s", op.charset);

            //尝试设置会话交互提示语言
            if (!is_empty(op.language))
                exec("SET lc_messages = '%s'", op.language);

            //日期显示格式
            exec("SET DateStyle='ISO,YMD'", op.charset);

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
            if (!m_handle)                                  //执行顺序错误,连接尚未建立
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));

            //进行语句格式化
            rx::tiny_string_t<char, 1024> SQL;
            va_list arg;
            va_start(arg,sql);
            SQL.fmt(sql, arg);
            va_end(arg);

            //执行语句
            PGresult *res = ::PQexec(m_handle,SQL.c_str());
            if (!res)                                       //出现内存不足的错误了
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__));

            //获取执行结果
            ::ExecStatusType ec = ::PQresultStatus(res);
            if (ec != PGRES_COMMAND_OK)
            {//如果不是无结果集命令成功,就进行错误信息记录
                SQL = ::PQresultErrorMessage(res);
                ::PQclear(res);                             //必须清理执行结果对象后,再抛出错误异常
                throw (error_info_t(SQL.c_str(), __FILE__, __LINE__));
            }
            ::PQclear(res);                                 //正常执行完成后也必须清理执行结果对象
        }
        //-------------------------------------------------
        //切换到指定的用户专属库
        void schema_to(const char *schema) { exec("set search_path = '%s'", schema); }
        //-------------------------------------------------
        //当前连接启动事务.本封装使用了非自动提交模式,显式事务的启动就无需特殊处理.
        void trans_begin() { exec("begin"); }
        //-------------------------------------------------
        //提交当前事务
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            exec("commit");
        }
        //-------------------------------------------------
        //回滚当前事务
        //返回值:操作结果(回滚本身出错时不再抛出异常)
        bool trans_rollback (int32_t *ec=NULL)
        {
            try { exec("rollback"); return true; }
            catch (error_info_t &e) { e.c_str(); return false; }
        }
        //-------------------------------------------------
        //进行服务器ping检查,真实的判断连接是否有效(不会抛出异常)
        bool ping()
        {
            try { exec("set session tmp.rx_dbc_pgsql_ping_test = 1"); return true; }
            catch (error_info_t &e) { e.c_str(); return false; }
        }
        //-------------------------------------------------
        //获取最后的oci错误号ec,与对应的错误描述
        //返回值:操作结果(出错时不再抛出异常)
        bool get_last_error(int32_t &ec,char *buff,uint32_t max_size)
        {
            return pgsql::get_last_error(ec, buff, max_size, m_handle);
        }
    };
}

#endif
