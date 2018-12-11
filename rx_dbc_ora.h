#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

//-----------------------------------------------------
//OCI网络中断后的默认传输超时应该是在20sec左右,进行超时判断时需要注意
#include "oci.h"                                            //引入OCI接口,默认在"3rd_deps\ora\include"
#include "dbc_comm/dbc_comm.h"                              //引入基础设施

//-----------------------------------------------------
namespace rx_dbc
{
    #include "dbc_ora/base.h"                               //实现一些通用功能
    #include "dbc_ora/conn.h"                               //实现数据库连接
    #include "dbc_ora/param.h"                              //实现语句段绑定参数
    #include "dbc_ora/stmt.h"                               //实现sql语句段
    #include "dbc_ora/field.h"                              //实现记录字段操作对象
    #include "dbc_ora/query.h"                              //实现记录查询访问对象

    namespace ora
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
