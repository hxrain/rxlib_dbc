#ifndef _RX_DBC_UTIL_H_
#define _RX_DBC_UTIL_H_

/*
    对rx_dbc::ora,rx_dbc::mysql等底层功能进行功能封装.
    如果不使用本工具框架中的功能而是直接使用底层功能封装,则需要进行完整的错误处理与捕捉与输出.
    外面使用的时候,应该按照如下顺序进行头文件引入:

    #include "../rx_dbc_ora.h"
    #include "../rx_dbc_util.h"

    或

    #include "../rx_dbc_mysql.h"
    #include "../rx_dbc_util.h"
*/

namespace rx_dbc
{
    //-----------------------------------------------------
    //日志输出函数的委托类型
    typedef rx::delegate3_t<const char*, const char*, va_list, void> dbc_log_delegate_t;

    //默认的日志输出功能函数,打印输出到控制台
    inline void default_dbc_log_func(const char* type, const char* msg, va_list arg, void*)
    {
        static rx::atomic_t<uint32_t> msg_seq;

        char cur_time_str[20];
        rx_iso_datetime(cur_time_str);
        printf("[%s][%04d][%s]:", cur_time_str, msg_seq.inc(), type);
        vprintf(msg, arg);
        puts("");
    }

    //-----------------------------------------------------
    //进行应用级dbc连接对象的功能封装,所有方法都不会抛出异常,便于应用层使用
    //-----------------------------------------------------
    template<class TT>
    class dbc_conn_t
    {
    public:
        //将外部提供的功能封装类在本类内部进行重新导入
        typedef typename TT::env_option_t    env_option_t;
        typedef typename TT::error_info_t    error_info_t;
        typedef typename TT::datetime_t      datetime_t;
        typedef typename TT::conn_t          conn_t;
        typedef typename TT::param_t         param_t;
        typedef typename TT::stmt_t          stmt_t;
        typedef typename TT::field_t         field_t;
        typedef typename TT::query_t         query_t;
    protected:
        //定义底层功能对象
        conn_t              m_conn;
        conn_param_t        m_conn_param;
        env_option_t        m_env_param;
        err_type_t          m_last_error;

        template<typename T> friend class dbc_ext_t;
        template<typename T> friend class dbc_tiny_t;
        template<typename T> friend class err_log_t;
    public:
        //-------------------------------------------------
        dbc_log_delegate_t  log_func;                       //日志输出方法
        //-------------------------------------------------
        //默认日志输出方法为default_dbc_log_func.
        dbc_conn_t(dbc_log_delegate_t::cb_func_t logfunc= default_dbc_log_func) { log_func.bind(logfunc); }
        dbc_conn_t(rx::mem_allotter_i& ma, dbc_log_delegate_t::cb_func_t logfunc = default_dbc_log_func) :m_conn(ma) { log_func.bind(logfunc); }
        virtual ~dbc_conn_t() {}
        //-------------------------------------------------
        err_type_t last_err() { return m_last_error; }
        //-------------------------------------------------
        //日志输出功能封装
        void log_warn(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("warn", msg, arg);
            va_end(arg);
        }
        void log_err(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("errr", msg, arg);
            va_end(arg);
        }
        void log_info(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("info", msg, arg);
            va_end(arg);
        }
        //-------------------------------------------------
        //切换到指定的用户域
        bool schema_to(const char *schema)
        {
            if (!connect())
                return false;

            try {
                m_conn.schema_to(schema);
                return true;
            }
            catch (error_info_t &e)
            {
                do_error(e, NULL);
                return false;
            }
        }
        //-------------------------------------------------
        //是否使用中文语言环境
        void use_chinese_env(bool flag = true) { flag ? m_env_param.use_chinese() : m_env_param.use_english(); }
        //-------------------------------------------------
        //设置连接参数
        void set_conn_param(const char* host, const char* user, const char* pwd, const char* db = "oradb", uint16_t port = 1521, uint16_t conn_timeout_sec = 3)
        {
            rx::st::strcpy(m_conn_param.host, sizeof(m_conn_param.host), host);
            rx::st::strcpy(m_conn_param.user, sizeof(m_conn_param.user), user);
            rx::st::strcpy(m_conn_param.pwd, sizeof(m_conn_param.pwd), pwd);
            rx::st::strcpy(m_conn_param.db, sizeof(m_conn_param.db), db);
            m_conn_param.port = port;
            m_conn_param.conn_timeout = conn_timeout_sec;
        }
        void set_conn_param(const conn_param_t &p) { set_conn_param(p.host, p.user, p.pwd, p.db, p.port, p.conn_timeout); }
        //-------------------------------------------------
        //进行连接动作,或检查连接是否成功
        //返回值:连接是否成功,0-连接失败;1连接正常;2首次建立;3重连完成.
        uint32_t connect(bool force_check = false)
        {
            if (force_check)
            {//如果要求强制检查,则进行真正的连接ping动作
                if (m_conn.ping())
                    return 1;                               //连接检查成功,直接返回
            }
            else if (m_conn.is_valid())
                return 1;

            set_last_error(DBEC_OK);

            //现在,连接无效,需要进行连接或重连动作
            try {
                bool is_opened = m_conn.is_valid();
                int32_t rc = m_conn.open(m_conn_param);
                if (rc)
                    log_warn("connect OK with error code[%d]:host(%s),port(%d),user(%s),db(%s)", rc, m_conn_param.host, m_conn_param.port, m_conn_param.user, m_conn_param.db);

                on_connect(m_conn, m_conn_param);           //给出连接完成动作事件
                return is_opened ? 3 : 2;
            }
            catch (error_info_t &e)
            {
                do_error(e, NULL);
                return 0;
            }
        }
        //-------------------------------------------------
        //获取表中记录的数量,可指定查询条件(where之后的部分)
        //返回值:<0错误;>=0为结果
        int records(const char* tblname, const char* cond = NULL)
        {
            if (!connect())
                return -1;

            query_t q(m_conn);
            try {
                return q.query_records(tblname, cond);
            }
            catch (error_info_t &e)
            {
                do_error(e, &q);
                return -2;
            }
        }
        //-------------------------------------------------
        //执行没有返回结果的语句
        bool exec(const char *sql, ...)
        {
            if (!connect())
                return false;

            query_t q(m_conn);

            va_list arg;
            va_start(arg, sql);
            bool rc = true;

            try {
                q.exec(sql, arg);
                q.conn().trans_commit();
            }
            catch (error_info_t &e)
            {
                q.conn().trans_rollback();
                do_error(e, &q);
                rc = false;
            }
            va_end(arg);
            return rc;
        }
        //-------------------------------------------------
    protected:
        //-------------------------------------------------
        //单纯的记录最后的dbc错误号
        void set_last_error(err_type_t e) { m_last_error = e; }
        //-------------------------------------------------
        //对错误输出进行绑定参数的自动打印
        template<typename T,int>
        class err_log_t
        {
        public:
            //进行错误记录与日志输出
            static void do_error(dbc_conn_t &conn,error_info_t &e, query_t *q)
            {
                rx::tiny_string_t<char, 1024> str;          //定义局部小串对象,准备拼装参数的值

                try {
                    uint32_t params = q->params();
                    str.assign();                           //缓冲区复位

                    for (uint32_t i = 0; i < params; ++i)   //循环拼装当前块深度的参数值
                    {
                        param_t &sp = q->param(i);
                        str << sp.name() << '=' << sp.as_string();
                        if (i + 1 < params) str << ' ';
                    }

                    conn.log_info("params(%d)<%s>", q->params(), str.c_str());
                }
                catch (...) {}
            }
        };
#if defined(_RX_DBC_ORA_COMM_H_)
        //对于ora功能封装进行特化,用于自动打印批量深度对应的全部参数
        template<int dummy>
        class err_log_t<ora::type_t,dummy>
        {
        public:
            //进行错误记录与日志输出
            static void do_error(dbc_conn_t &conn, error_info_t &e, query_t *q)
            {
                rx::tiny_string_t<char, 1024> str;          //定义局部小串对象,准备拼装参数的值

                try {
                    uint32_t bulks = q->bulks(false);
                    uint32_t params = q->params();
                    for (uint32_t bi = 0; bi < bulks; ++bi)
                    {//对最后的批量深度进行遍历
                        q->bulk(bi);                        //设置块深度
                        str.assign();                       //缓冲区复位

                        for (uint32_t i = 0; i < params; ++i)   //循环拼装当前块深度的参数值
                        {
                            str << q->param(i).name() << '=' << q->param(i).as_string();
                            if (i + 1 < params) str << ' ';
                        }

                        conn.log_info("bulk[%d/%d]params(%d)<%s>", bi + 1, bulks, q->params(), str.c_str());
                    }
                }
                catch (...) {}
            }
        };
#endif
        //-------------------------------------------------
        //进行错误记录与日志输出
        void do_error(error_info_t &e, query_t *q)
        {
            log_err(e.c_str(m_conn_param));                 //先输出异常内容
            set_last_error((err_type_t)e.dbc_error_code()); //记录最后的统一错误码
            if (!q || !q->params()) return;                 //没有语句处理对象,或没有绑定的参数,返回

            err_log_t<TT,0>::do_error(*this,e,q);
        }
        //-------------------------------------------------
        //连接完成事件
        virtual void on_connect(conn_t& conn, const conn_param_t &param) {}
    };

    //-----------------------------------------------------
    //对数据库访问功能进行轻量级封装,仅进行了连接重连与统一异常捕捉处理
    //-----------------------------------------------------
    template<typename TT>
    class dbc_tiny_t:public TT
    {
    public:
        typedef typename TT::env_option_t    env_option_t;
        typedef typename TT::error_info_t    error_info_t;
        typedef typename TT::datetime_t      datetime_t;
        typedef typename TT::conn_t          conn_t;
        typedef typename TT::param_t         param_t;
        typedef typename TT::stmt_t          stmt_t;
        typedef typename TT::field_t         field_t;
        typedef typename TT::query_t         query_t;

        typedef dbc_conn_t<TT> dbc_conn_t;
    protected:
        query_t                     m_query;                //实际语句的底层执行器
        dbc_conn_t                 &m_dbconn;               //连接器功能对象的引用

        template<typename T>
        friend class dbc_ext_t;
        //-------------------------------------------------
        //预处理语句
        //返回值:<0错误; 0用户要求放弃; >0完成
        int m_prepare(const char* sql)
        {
            try {
                m_query.prepare(sql);                       //预解析动作
                return 1;
            }
            catch (error_info_t &e)
            {
                m_dbconn.do_error(e, &m_query);
                return -103;
            }
        }
        //-------------------------------------------------
        //真正执行语句
        //返回值:<0错误; 0用户要求放弃; >0完成
        int m_exec(bool explicit_trans, void *usrdat)
        {
            try {
                if (explicit_trans)
                    m_query.conn().trans_begin();           //回调函数里面要执行多条语句,需要进行手动事务处理

                int rc = on_exec(m_query, usrdat);          //执行用户给定的语句
                if (rc <= 0)
                {
                    if (explicit_trans)
                        m_query.conn().trans_rollback();    //要求放弃或出错,回滚
                    return rc;
                }

                if (m_query.sql_type() != ST_SELECT)
                    m_query.conn().trans_commit();          //如果不是查询语句,必须进行提交

                return rc;
            }
            catch (error_info_t &e)
            {
                m_dbconn.do_error(e, &m_query);
                m_query.conn().trans_rollback();            //出现任何错误,都尝试回滚
                return -102;
            }
        }
        //-------------------------------------------------
        //进行重试与重连的动作处理
        int do_action(const char* sql, void *usrdat, bool explicit_trans, uint32_t retry)
        {
            if (!m_dbconn.connect())
                return -100;                                //进行连接或连接检查失败,直接返回

            for (uint32_t ri = 0; ri <= retry; ++ri)
            {
                int rc;
                if (!is_empty(sql))
                {
                    rc = m_prepare(sql);                    //如果给出了sql则预解析
                    if (rc <= 0)
                        return rc;
                }

                rc = m_exec(explicit_trans, usrdat);        //调用真正的执行动作
                if (!retry || rc >= 0)
                    return rc;                              //无需重试或成功完成,直接返回

                uint32_t cc = m_dbconn.connect(true);       //执行失败需要重试
                if (!cc)
                    return -101;                            //强制连接或连接检查失败,说明连接确实断开了

                if (cc != 3)
                    return rc;                              //如果连接动作不是立即重连完成,则不用继续重试了.
            }
            return -104;
        }
        //-------------------------------------------------
        //尝试提取fetch_count数量的结果行
        //返回值:<0错误;0结束;>0本次提取的数量
        int m_fetch(uint32_t fetch_count)
        {
            try {
                uint32_t rc = 0;
                for (; !m_query.eof() && rc<fetch_count; m_query.next(), ++rc)
                    on_row(m_query, &rc);
                return rc;
            }
            catch (error_info_t &e)
            {
                m_dbconn.do_error(e, &m_query);
                return -201;
            }
        }
    public:
        //-------------------------------------------------
        dbc_tiny_t(dbc_conn_t &c) :m_query(c.m_conn), m_dbconn(c) {}
        virtual ~dbc_tiny_t() {}
        //以下对外输出功能方法,都不会抛出异常且会输出错误日志,简化外部调用者的错误处理.
        //-------------------------------------------------
        //执行sql语句,并给定待处理数据;反复多次处理数据的时候不再指定sql语句(给NULL或"").
        //返回值:<0错误;0用户要求放弃;>0完成
        int action(const char* sql, void *usrdat = NULL, bool explicit_trans = false, uint32_t retry = 1)
        {
            const char* event_sql = is_empty(sql) ? m_query.sql_string() : sql;
            if (!on_begin(m_dbconn, m_query, event_sql, usrdat))//给出动作开始前的处理机会
                return 0;
            int rc = do_action(sql, usrdat, explicit_trans, retry);
            on_end(rc, m_dbconn, m_query, event_sql, usrdat);
            return rc;
        }
        int operator()(const char* sql, void *usrdat = NULL, bool explicit_trans = false, uint32_t retry = 1)
        {
            return action(sql, usrdat, explicit_trans, retry);
        }
        //-------------------------------------------------
        //执行了select后,可以进行结果的提取;此方法可以反复多次调用,直到结果遍历完成
        //返回值:<0错误;0结束;>0本次提取的数量
        int fetch(uint32_t fetch_count = 0)
        {
            m_dbconn.set_last_error(DBEC_OK);
            if (m_query.sql_type() != ST_SELECT)
            {
                m_dbconn.log_err("non-select statements were fetched resultset! (%s)", m_query.sql_string());
                m_dbconn.set_last_error(DBEC_METHOD_CALL);
                return -200;
            }

            int total = 0;
            if (fetch_count == 0)
                fetch_count = -1;                           //默认尝试一次性提取所有记录

            do {//循环并分批次提取
                int rc = m_fetch(rx::Min((uint32_t)BAT_FETCH_SIZE, fetch_count));
                if (rc > 0)
                {
                    total += rc;                            //总数增加
                    fetch_count -= rc;                      //剩余数减少
                    if (!fetch_count)
                        break;                              //结束了
                }
                else if (rc == 0)
                    break;                                  //没有数据了
                else
                    return -201;                            //提取出错了
            } while (1);

            return total;                                   //正常结束,给出本次提取总数
        }
    protected:
        //-------------------------------------------------
        //子类可以实现动作前后触发事件的回调
        //动作开始之前进行处理(可进行计时/重连/调试等处理);返回值:是否继续执行
        virtual bool on_begin(dbc_conn_t &conn, query_t &q, const char* sql, void *usrdat) { return true; }
        //动作完成之后进行处理(可进行计时,重连等处理)
        virtual void on_end(int rc, dbc_conn_t &conn, query_t &q, const char* sql, void *usrdat) {}
        //-------------------------------------------------
        //获取到结果,访问当前行数据;
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_row(query_t &q, void *usrdat) { return 1; }
        //-------------------------------------------------
        //给用户提供执行事件,可以进行多条语句的处理;sql已经被预解析了.
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_exec(query_t &q, void *usrdat) { q.exec(); return 1; }
    };

    //-----------------------------------------------------
    //进行应用级dbc语句对象的功能封装,所有方法都不会抛出异常,扩展了外部事件回调,细化了数据绑定动作事件.
    //-----------------------------------------------------
    template<typename TT>
    class dbc_ext_t :public dbc_tiny_t<TT>
    {
    public:
        typedef typename TT::env_option_t    env_option_t;
        typedef typename TT::error_info_t    error_info_t;
        typedef typename TT::datetime_t      datetime_t;
        typedef typename TT::conn_t          conn_t;
        typedef typename TT::param_t         param_t;
        typedef typename TT::stmt_t          stmt_t;
        typedef typename TT::field_t         field_t;
        typedef typename TT::query_t         query_t;

        typedef dbc_tiny_t<TT> super_t;
        typedef typename super_t::dbc_conn_t dbc_conn_t;

        //定义dbc事件的委托类型
        typedef rx::delegate1_t<typename TT::query_t&, int32_t> dbc_delegate_t;
        //DBC事件委托对应的函数指针类型;//返回值:<0错误; 0用户要求放弃; >0完成,批量深度
        typedef int32_t(*dbc_event_func_t)(typename TT::query_t &q, void *usrdat);

        dbc_delegate_t          m_databind_dgt;         //数据绑定事件的委托类型
        dbc_delegate_t          m_datafetch_dgt;        //数据提取事件的委托类型
    public:
        //-------------------------------------------------
        //构造函数,绑定db连接与业务处理回调函数
        dbc_ext_t(dbc_conn_t  &c, dbc_event_func_t on_bind = NULL, dbc_event_func_t on_row = NULL) :super_t(c)
        {
            if (on_bind) event_on_bind(on_bind);
            if (on_row) event_on_row(on_row);
        }
        virtual ~dbc_ext_t() {}
        //-------------------------------------------------
        //执行子类提供的sql语句,处理数据
        //返回值:<0错误;0用户要求放弃;>0完成
        int action(void *usrdat, bool explicit_trans = false, uint32_t retry = 1)
        {
            if (is_empty(super_t::m_query.sql_string()))
                return super_t::action(on_sql(), usrdat, explicit_trans, retry);  //首次执行,需要解析sql
            else
                return super_t::action(NULL, usrdat, explicit_trans, retry);     //后续执行,不需要再解析sql
        }
        //-------------------------------------------------
        //关联数据绑定处理回调函数
        void event_on_bind(dbc_event_func_t func) { m_databind_dgt.bind(func); }
        //关联数据提取处理的回调函数
        void event_on_row(dbc_event_func_t func) { m_datafetch_dgt.bind(func); }
    protected:
        //-------------------------------------------------
        //执行事件,可以进行多条语句的处理
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_exec(query_t &q, void *usrdat)
        {
            if (!q.params())
                q.auto_bind(10);                            //尝试自动绑定,告知最大批量深度
            int rc = on_bind_data(q, usrdat);               //驱动on_bind_data事件
            if (rc <= 0) return rc;
            q.exec(rc);                                     //执行真正的OCI/ORA动作
            return 1;
        }
        //-------------------------------------------------
        //!!关键!!获取到结果,访问当前行数据;
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_row(query_t &q, void *usrdat)
        {
            if (m_datafetch_dgt.is_valid())
            {
                m_datafetch_dgt.bind(m_datafetch_dgt.cb_func(), usrdat);
                return m_datafetch_dgt(q);                  //默认实现,尝试使用委托对象中的函数进行调用
            }
            return 1;
        }
    protected:
        //-------------------------------------------------
        //通过子类给定sql语句
        virtual const char* on_sql() { return NULL; }
        //-------------------------------------------------
        //!!关键!!进行参数数据的绑定动作;
        //返回值:<0错误;0用户要求放弃;>0为绑定批量块的深度
        virtual int32_t on_bind_data(query_t &q, void *usrdat)
        {
            if (m_databind_dgt.is_valid())
            {//每次都需要尝试更新用户绑定数据函数对应的数据指针
                m_databind_dgt.bind(m_databind_dgt.cb_func(), usrdat);
                return m_databind_dgt(q);                   //调用委托对象中的函数
            }
            return 0;
        }
    };
}


#endif