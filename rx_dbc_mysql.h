#ifndef	_RX_DBC_MYSQL_H_
#define	_RX_DBC_MYSQL_H_

//---------------------------------------------------------
#include "mysql.h"                                          //引入mysql接口,默认在"3rd_deps\mysql\include"
#include "errmsg.h"                                         //引入mysql错误消息
#include "mysqld_error.h"                                   //引入mysql扩展错误消息
#include "dbc_comm/dbc_comm.h"                              //引入基础设施
//---------------------------------------------------------
namespace rx_dbc
{
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
            typedef mysql::env_option_t    env_option_t;
            typedef mysql::error_info_t    error_info_t;
            typedef mysql::datetime_t       datetime_t;

            typedef mysql::conn_t           conn_t;
            typedef mysql::param_t          param_t;
            typedef mysql::stmt_t           stmt_t;
            typedef mysql::field_t          field_t;
            typedef mysql::query_t          query_t;
        };
    }
}

#endif
