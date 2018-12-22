#ifndef	_RX_DBC_PGSQL_H_
#define	_RX_DBC_PGSQL_H_

//---------------------------------------------------------
#include "libpq-fe.h"                                       //引入libpq接口,默认在"3rd_deps\pgsql\include"
#include "dbc_comm/dbc_comm.h"                              //引入基础设施
#include "pg_type_sc.h"                                     //引入pqsql数据类型OID

//---------------------------------------------------------
namespace rx_dbc
{
    #include "dbc_pgsql/base.h"                             //实现一些通用功能
    #include "dbc_pgsql/conn.h"                             //实现数据库连接
    #include "dbc_pgsql/field.h"                            //实现记录字段操作对象

    #include "dbc_pgsql/param.h"                            //实现语句段绑定参数
    #include "dbc_pgsql/stmt.h"                             //实现sql语句段
    #include "dbc_pgsql/query.h"                            //实现记录查询访问对象

    namespace pgsql
    {
        //-------------------------------------------------
        //将本命名空间中的对外开放类型进行统一声明,便于dbc上层工具的使用.
        class type_t
        {
        public:
            typedef pgsql::env_option_t    env_option_t;
            typedef pgsql::error_info_t    error_info_t;
            typedef pgsql::datetime_t      datetime_t;

            typedef pgsql::conn_t          conn_t;
            typedef pgsql::param_t         param_t;
            typedef pgsql::stmt_t          stmt_t;
            typedef pgsql::field_t         field_t;
            typedef pgsql::query_t         query_t;
        };
    }
}

#endif
