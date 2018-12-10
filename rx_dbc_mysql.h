#ifndef	_RX_DBC_MYSQL_H_
#define	_RX_DBC_MYSQL_H_

//---------------------------------------------------------
#include "mysql.h"                                          //引入mysql接口,默认在"3rd_deps\mysql\include"
#include "errmsg.h"                                         //引入mysql错误消息
#include "mysqld_error.h"                                   //引入mysql扩展错误消息
//---------------------------------------------------------
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

//---------------------------------------------------------
namespace rx_dbc
{
    #include "dbc_comm/dbc_parse.h"                         //SQL绑定参数的名字解析功能
    #include "dbc_comm/dbc_type.h"                          //统一类型定义
    #include "dbc_mysql/base.h"                             //实现一些通用功能
    #include "dbc_mysql/conn.h"                             //实现数据库连接
    #include "dbc_mysql/field.h"                            //实现记录字段操作对象
    #include "dbc_mysql/param.h"                            //实现语句段绑定参数
    #include "dbc_mysql/stmt.h"                             //实现sql语句段
    #include "dbc_mysql/query.h"                            //实现记录查询访问对象

    namespace mysql
    {
        //-------------------------------------------------
        //将本命名空间中的对外开放类型进行统一声明,便于dbc上层工具的使用.
        class type_t
        {
        public:
            typedef env_option_t    env_option_t;
            typedef error_info_t    error_info_t;
            typedef datetime_t      datetime_t;

            typedef conn_t          conn_t;
            typedef param_t         param_t;
            typedef stmt_t          stmt_t;
            typedef field_t         field_t;
            typedef query_t         query_t;
        };
    }
}

#endif
