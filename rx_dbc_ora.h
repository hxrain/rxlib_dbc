#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

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

#include "oci.h"                                            //引入OCI接口,默认在"3rd_deps\ora\include"

#include "dbc_comm/dbc_parse.h"                             //SQL绑定参数的名字解析功能
#include "dbc_comm/dbc_type.h"                              //基础公共类型定义

namespace rx_dbc
{
    //OCI网络中断后的默认传输超时应该是在20sec左右,进行超时判断时需要注意
    #include "dbc_ora/base.h"                                   //实现一些通用功能
    #include "dbc_ora/conn.h"                                   //实现数据库连接
    #include "dbc_ora/param.h"                                  //实现语句段绑定参数
    #include "dbc_ora/stmt.h"                                   //实现sql语句段
    #include "dbc_ora/field.h"                                  //实现记录字段操作对象
    #include "dbc_ora/query.h"                                  //实现记录查询访问对象

    namespace ora
    {
        //-------------------------------------------------
        //将本命名空间中的对外开放类型进行统一声明
        class type_t
        {
        public:
            typedef env_option_t    env_option_t;
            typedef error_info_t    error_info_t;
            typedef datetime_t      datetime_t;

            typedef conn_t          conn_t;
            typedef sql_param_t     sql_param_t;
            typedef stmt_t          stmt_t;

            typedef field_t         field_t;
            typedef query_t         query_t;
        };

    }

}

#endif
