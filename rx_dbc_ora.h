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
    class dbc_conn_t
    {
        conn_t          m_conn;
        conn_param_t    m_conn_param;
        env_option_t    m_env_param;
        friend class dbc_stmt_t;
        friend class dbc_query_t;
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
    //进行应用级dbc语句对象的功能封装,所有方法都不会抛出异常,便于应用层子类继承后用于实现具体业务
    class dbc_stmt_t
    {
        stmt_t      m_stmt;
        dbc_conn_t  &m_dbc;
        //-------------------------------------------------
        bool m_exec()
        {
            try {

            }
            catch (error_info_t &e)
            {

            }
        }
    public:
        //-------------------------------------------------
        dbc_stmt_t(dbc_conn_t  &dbc):m_stmt(dbc.m_conn),m_dbc(dbc){}
        //-------------------------------------------------
        bool exec()
        {

        }
    };

    //-----------------------------------------------------
    //进行应用级dbc查询对象的功能封装,所有方法都不会抛出异常,便于应用层子类继承后用于实现具体业务
    class dbc_query_t
    {
        query_t     m_query;
        dbc_conn_t  &m_dbc;
    public:
        dbc_query_t(dbc_conn_t  &dbc) :m_query(dbc.m_conn), m_dbc(dbc) {}
    };

}

#endif


