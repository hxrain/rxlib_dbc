#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "oci.h"                                            //引入OCI接口,默认在"3rd_deps\ora\include"

#include "rx_cc_macro.h"                                    //引入基础宏定义
#include "rx_assert.h"                                      //引入断言
#include "rx_str_util_std.h"                                //引入基础字符串功能
#include "rx_str_util_ex.h"                                 //引入扩展字符串功能
#include "rx_str_tiny.h"                                    //引入定长字符串功能
#include "rx_mem_alloc_cntr.h"                              //引入内存分配容器
#include "rx_datetime.h"                                    //引入日期时间功能
#include "rx_dtl_buff.h"                                    //引入缓冲区功能
#include "rx_dtl_array_ex.h"                                //引入别名数组
#include "rx_ct_delegate.h"                                 //引入委托功能
#include "rx_datetime_ex.h"                                 //引入日期时间功能扩展
#include "rx_cc_atomic.h"                                   //引入原子变量功能

#include "dbc_comm/sql_param_parse.h"                       //SQL绑定参数的名字解析功能

#include "dbc_ora/comm.h"                                   //实现一些通用功能
#include "dbc_ora/conn.h"                                   //实现数据库连接
#include "dbc_ora/param.h"                                  //实现语句段绑定参数
#include "dbc_ora/stmt.h"                                   //实现sql语句段
#include "dbc_ora/field.h"                                  //实现记录字段操作对象
#include "dbc_ora/query.h"                                  //实现记录查询访问对象

//---------------------------------------------------------
namespace rx_dbc_ora
{
    //日志输出函数的委托类型
    typedef rx::delegate3_t<const char*,const char*, va_list,void> dbc_log_delegate_t;
    static inline void default_dbc_log_func(const char* type, const char* msg, va_list arg, void*) 
    { 
        static rx::atomic_t<uint32_t> msg_seq;

        char cur_time_str[20];
        rx_iso_datetime(cur_time_str);
        printf("[%s][%04d][%s]", cur_time_str, msg_seq.inc(),type);
        vprintf(msg, arg);
        puts("\n");
    }

    //-----------------------------------------------------
    //进行应用级dbc连接对象的功能封装,所有方法都不会抛出异常,便于应用层使用
    //-----------------------------------------------------
    class dbc_conn_t
    {
        conn_t          m_conn;
        conn_param_t    m_conn_param;
        env_option_t    m_env_param;
        friend class dbc_t;
        friend class tiny_dbc_t;
    public:
        //-------------------------------------------------
        dbc_log_delegate_t  log_func;                       //日志输出方法,默认为default_dbc_log_func.
        dbc_conn_t() { log_func.bind(default_dbc_log_func); }
        dbc_conn_t(rx::mem_allotter_i& ma):m_conn(ma) { log_func.bind(default_dbc_log_func); }
        virtual ~dbc_conn_t() {}
        //-------------------------------------------------
        //日志输出功能封装
        void log_warn(const char* msg,...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("warn", msg, arg);
        }
        void log_err(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("err", msg, arg);
        }
        void log_info(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("info", msg, arg);
        }
        //-------------------------------------------------
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
                log_err(e.c_str(m_conn_param));
                return false;
            }
        }
        //-------------------------------------------------
        //是否使用中文语言环境
        void use_chinese_env(bool flag = true) { flag ? m_env_param.use_chinese() : m_env_param.use_english(); }
        //-------------------------------------------------
        //设置连接参数
        void set_conn_param(const char* host,const char* user,const char* pwd,const char* db="oradb",uint16_t port=1521,uint16_t conn_timeout_sec=3) 
        {
            rx::st::strcpy(m_conn_param.host, sizeof(m_conn_param.host), host);
            rx::st::strcpy(m_conn_param.user, sizeof(m_conn_param.user), user);
            rx::st::strcpy(m_conn_param.pwd, sizeof(m_conn_param.pwd), pwd);
            rx::st::strcpy(m_conn_param.db, sizeof(m_conn_param.db), db);
            m_conn_param.port = port;
            m_conn_param.conn_timeout = conn_timeout_sec;
        }
        void set_conn_param(const conn_param_t &p) { set_conn_param(p.host,p.user,p.pwd,p.db,p.port,p.conn_timeout); }
        //-------------------------------------------------
        //进行连接动作,或检查连接是否成功
        //返回值:连接是否成功
        bool connect(bool force_check=false, const char* schema = NULL)
        {
            if (force_check)
            {//如果要求强制检查,则进行真正的连接ping动作
                if (m_conn.ping())
                    return true;                            //连接检查成功,直接返回
            }
            else if (m_conn.is_valid())
                return true;

            //现在,连接无效,需要进行连接动作
            try {
                m_conn.open(m_conn_param);
                if (!is_empty(schema))
                    m_conn.schema_to(schema);
                on_connect(m_conn);                         //给出连接完成动作事件
                return true;
            }
            catch (error_info_t &e)
            {
                log_err(e.c_str(m_conn_param));
                return false;
            }
        }

        //-------------------------------------------------
    protected:
        //连接完成事件
        virtual void on_connect(conn_t& conn) {}
    };

    //-----------------------------------------------------
    //对数据库访问功能进行轻量级封装,仅进行了连接重连与统一异常捕捉处理
    //-----------------------------------------------------
    class tiny_dbc_t
    {
        query_t                     m_query;                //实际语句的底层执行器
        dbc_conn_t                 &m_dbconn;               //连接器功能对象的引用

        //-------------------------------------------------
        //真正执行语句
        //返回值:<0错误; 0用户要求放弃; >0完成
        int m_exec(const char* sql,bool manual_trans, void *usrdat)
        {
            try {
                if (manual_trans)
                    m_query.conn().trans_begin();           //回调函数里面要执行多条语句,需要进行手动事务处理

                int rc=on_exec(m_query, sql, usrdat);       //执行用户给定的语句,动作开始
                if (rc <= 0)
                {
                    if (manual_trans)
                        m_query.conn().trans_rollback();    //要求放弃或出错,回滚
                    return rc;
                }

                if (m_query.sql_type()!=ST_SELECT)
                    m_query.conn().trans_commit();          //如果不是查询语句,必须进行提交

                return rc;
            }
            catch (error_info_t &e)
            {
                m_query.conn().trans_rollback();            //出现任何错误,都尝试回滚
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));//给出错误日志
                return -102;
            }
        }
    public:
        //-------------------------------------------------
        tiny_dbc_t(dbc_conn_t  &c):m_query(c.m_conn), m_dbconn(c){}
        //以下对外输出功能方法,都不会抛出异常且会输出错误日志,简化外部调用者的错误处理.
        //-------------------------------------------------
        //进行sql语句的数据绑定并执行
        //返回值:<0错误;0用户要求放弃;>0完成
        int operator()(const char* sql, void *usrdat = NULL, bool manual_trans = false, bool can_retry = true)
        {
            if (!m_dbconn.connect())
                return -100;                                //进行连接或连接检查失败,直接返回

            int rc = m_exec(sql,manual_trans, usrdat);      //调用真正的执行动作
            if (!can_retry || rc >= 0)
                return rc;                                  //无需重试或成功完成,直接返回
            
            //执行失败需要重试
            if (!m_dbconn.connect(true))
                return -101;                                //强制连接或连接检查失败,说明连接确实断开了

            return m_exec(sql,manual_trans, usrdat);        //尝试再次执行
        }
    protected:
        //-------------------------------------------------
        //数据绑定相关事件
        //-------------------------------------------------
        //执行前给出事件,可以进行多条语句的处理
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_exec(query_t &q, const char *sql, void *usrdat) = 0;
    };


    //-----------------------------------------------------
    //定义数据绑定事件的委托类型
    typedef rx::delegate1_t<query_t&, int32_t> dbc_delegate_t;
    //DBC事件委托对应的函数指针类型;//返回值:<0错误; 0用户要求放弃; >0完成,批量深度
    typedef int32_t (*dbc_event_func_t)(query_t &q,void *usrdat);

    //-----------------------------------------------------
    //进行应用级dbc语句对象的功能封装,所有方法都不会抛出异常,便于应用层子类继承后用于实现具体业务
    //-----------------------------------------------------
    class dbc_t
    {
        query_t                     m_query;                //实际语句的底层执行器
        dbc_conn_t                 &m_dbconn;               //连接器功能对象的引用
        dbc_delegate_t              m_databind_dgt;         //数据绑定事件的委托类型
        dbc_delegate_t              m_datafetch_dgt;        //数据提取事件的委托类型
        uint32_t                    m_bind_bulks;           //记录实际执行时绑定的数据批量深度
        //-------------------------------------------------
        //进行参数数据的绑定处理
        //返回值:<0错误; 0用户要求放弃; >0完成
        int m_bind_data(void *usrdat)
        {
            try {
                int bind_bulks = on_bind_data(m_query, usrdat);
                if (bind_bulks <= 0)
                    return bind_bulks;
                else
                    m_bind_bulks = bind_bulks;
                return m_bind_bulks;
            }
            catch (error_info_t &e)
            {
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));
                return -103;
            }
        }
        //-------------------------------------------------
        //真正执行语句
        //返回值:<0错误; 0用户要求放弃; >0完成
        int m_exec(bool manual_trans, void *usrdat)
        {
            try {
                if (manual_trans)
                    m_query.conn().trans_begin();           //回调函数里面要执行多条语句,需要进行手动事务处理

                int rc=on_exec_befor(m_query, usrdat);      //执行用户给定的语句,动作开始
                if (rc <= 0)
                {
                    if (manual_trans)
                        m_query.conn().trans_rollback();    //要求放弃或出错,回滚
                    return rc;
                }

                rc=on_exec_after(m_query, usrdat);          //执行用户给定的语句,动作结束
                if (rc <= 0)
                {
                    if (manual_trans)
                        m_query.conn().trans_rollback();    //要求放弃或出错,回滚
                    return rc;
                }

                if (m_query.sql_type()!=ST_SELECT)
                    m_query.conn().trans_commit();          //如果不是查询语句,必须进行提交

                return rc;
            }
            catch (error_info_t &e)
            {
                m_query.conn().trans_rollback();            //出现任何错误,都尝试回滚
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));//给出错误日志
                return -102;
            }
        }
    public:
        //-------------------------------------------------
        dbc_t(dbc_conn_t  &c):m_query(c.m_conn), m_dbconn(c){}
        //以下对外输出功能方法,都不会抛出异常且会输出错误日志,简化外部调用者的错误处理.
        //-------------------------------------------------
        //进行sql语句的预解析
        //返回值:是否成功
        bool prepare(const char* sql,...)
        {
            if (!m_dbconn.connect())                        //进行轻量级连接检查即可,要求句柄的有效性
                return false;

            try {
                va_list arg;
                va_start(arg, sql);
                m_query.prepare(sql, arg);                  //预解析,

                uint32_t max_bulk_deep = on_auto_bind();    //判断是否可以自动绑定参数
                if (max_bulk_deep)
                    m_query.auto_bind(max_bulk_deep);       //可以自动绑定
                else
                    on_bind_param(m_query);                 //尝试进行手动绑定
                return true;
            }
            catch (error_info_t &e)
            {
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));
                return false;
            }
        }
        //-------------------------------------------------
        //进行sql语句的数据绑定并执行
        //返回值:<0错误;0用户要求放弃;>0完成
        int exec(void *usrdat=NULL,bool manual_trans=false,bool can_retry=true)
        {
            int rc = m_bind_data(usrdat);                   //调用on_bind_data绑定数据
            if (rc <= 0)
                return rc;                                  //如果绑定数据出错或要求放弃,那么就不用继续了

            if (!m_dbconn.connect())
                return -100;                                //进行连接或连接检查失败,直接返回

            rc = m_exec(manual_trans, usrdat);              //调用真正的执行动作
            if (!can_retry || rc >= 0)
                return rc;                                  //无需重试或成功完成,直接返回
            
            //执行失败需要重试
            if (!m_dbconn.connect(true))
                return -101;                                //强制连接或连接检查失败,说明连接确实断开了

            return m_exec(manual_trans, usrdat);            //尝试再次执行
        }
        //-------------------------------------------------
        //便捷方法,直接解析并执行
        int operator()(const char* sql, void *usrdat = NULL, bool manual_trans = false, bool can_retry = true)
        {
            if (!prepare(sql))
                return -104;
            return exec(usrdat, manual_trans, can_retry);
        }
        //-------------------------------------------------
        //执行了select后,可以进行结果的提取;此方法可以反复多次调用,直到结果遍历完成
        //返回值:<0错误;0结束;>0本次提取的数量
        int fetch(void *usrdat=NULL,uint32_t loop_count = 100)
        {
            if (m_query.sql_type() != ST_SELECT)
            {
                m_dbconn.log_err("non-select statements were fetched resultset! (%s)",m_query.sql_string());
                return -200;
            }

            try {
                uint32_t rc = 0;
                for (; !m_query.eof() && rc<loop_count; m_query.next(),++rc)
                    on_row_data(m_query, usrdat);
                return rc;
            }
            catch (error_info_t &e)
            {
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));
                return -201;
            }
        }
        //-------------------------------------------------
        //关联数据绑定处理回调函数(如果func为空,则可以进行usrdat的更新)
        bool event_data_bind(dbc_event_func_t func, void *usrdat=NULL) 
        { 
            if (func == NULL)
                func = m_databind_dgt.cb_func();
            if (func == NULL)
                return false;
            m_databind_dgt.bind(func,usrdat); 
            return true;
        }
        //关联数据提取处理的回调函数
        bool event_data_row(dbc_event_func_t func, void *usrdat = NULL) 
        {
            if (func == NULL)
                func = m_datafetch_dgt.cb_func();
            if (func == NULL)
                return false;
            m_datafetch_dgt.bind(func, usrdat);
            return true;
        }

    protected:
        //以下所有的事件,都不用进行额外的异常捕捉,只要编写业务代码即可
        //-------------------------------------------------
        //参数绑定相关事件
        //-------------------------------------------------
        //是否进行自动参数绑定
        //返回值:0不自动绑定参数;>0为参数数据块的最大批量深度
        virtual uint32_t on_auto_bind() { return 1; }
        //-------------------------------------------------
        //进行手动参数绑定动作,需要告知最大的批量深度并逐一进行参数绑定
        virtual void on_bind_param(query_t &q) { q.manual_bind(1); }
    protected:
        //-------------------------------------------------
        //数据绑定相关事件
        //-------------------------------------------------
        //执行前给出事件,可以进行多条语句的处理
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_exec_befor(query_t &q, void *usrdat) { q.exec(BAT_FETCH_SIZE, m_bind_bulks); return 1; }
        //-------------------------------------------------
        //执行完成后给出事件,比如可以与on_bind_data配合,进行剩余待绑定数据偏移量的调整
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_exec_after(query_t &q, void *usrdat) { return 1; }
        //-------------------------------------------------
        //!!关键!!进行参数数据的绑定动作;
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_bind_data(query_t &q, void *usrdat)
        { 
            if (m_databind_dgt.is_valid()) 
                return m_databind_dgt(q);                   //默认实现,尝试使用委托对象中的函数进行调用
            return 0; 
        }
    protected:
        //-------------------------------------------------
        //结果提取相关事件
        //-------------------------------------------------
        //!!关键!!获取到结果,访问当前行数据;
        //返回值:<0错误;0用户要求放弃;>0完成
        virtual int32_t on_row_data(query_t &q,void *usrdat) { if (m_datafetch_dgt.is_valid()) m_datafetch_dgt(q); return 1; }
    };

    //-----------------------------------------------------
    //语法糖,便于函数指针绑定数据并执行语句
    class dbc_exec_t
    {
        dbc_t   m_dbc;
    public:
        //初始绑定必要元素
        dbc_exec_t(dbc_conn_t  &c, dbc_event_func_t func,void *usrdat=NULL):m_dbc(c) { m_dbc.event_data_bind(func,usrdat); }
        //执行sql语句,或更换数据后再次执行
        int operator()(const char* sql, void *usrdat=NULL, dbc_event_func_t func=NULL)
        {
            if (!is_empty(sql)&&!m_dbc.prepare(sql))
                return -300;
            if (usrdat&&!m_dbc.event_data_bind(func, usrdat))
                return -301;
            return m_dbc.exec();
        }
        //更换数据后再次执行
        int operator()(void *usrdat, dbc_event_func_t func = NULL)
        {
            if (!m_dbc.event_data_bind(func,usrdat))
                return -301;
            return m_dbc.exec();
        }
    };
}

#endif