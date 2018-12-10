#ifndef _RX_DBC_TYPE_H_
#define _RX_DBC_TYPE_H_

namespace rx_dbc
{
    //允许使用的字符串的最大长度
    const unsigned short MAX_TEXT_BYTES = 1024 * 2;

    //每次批量FEATCH获取的结果集的数量
    const unsigned short BAT_FETCH_SIZE = 20;

    //sql语句的长度限制
    const int MAX_SQL_LENGTH = 1024 * 4;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //-----------------------------------------------------
    //DB类型
    typedef enum db_type_t
    {
        DBT_UNKNOWN=-1,
        DBT_ORA,
        DBT_MYSQL,
        DBT_PGSQL,
        DBT_SQLITE,
    }db_type_t;

    //-----------------------------------------------------
    //DB连接参数
    typedef struct conn_param_t
    {
        char        host[64];                               //数据库服务器所在地址
        char        user[64];                               //数据库用户名
        char        pwd[64];                                //数据库口令
        char        db[64];                                 //数据库实例名
        int         port;                                   //数据库端口
        int         conn_timeout;                           //连接超时时间
        conn_param_t()
        {
            host[0] = 0;
            db[0] = 0;
            user[0] = 0;
            pwd[0] = 0;
            port = 1521;
            conn_timeout = 3;
        }
    }conn_param_t;

    //-----------------------------------------------------
    //dbc_ora可以处理的数据类型(绑定参数时,名字前缀可以告知数据类型)
    typedef enum data_type_t
    {
        DT_UNKNOWN=-1,
        DT_LONG     = 'i',                                  //带符号整数,long/int32_t
        DT_ULONG    = 'u',                                  //无符号整数,ulong/uint32_t
        DT_FLOAT    = 'f',                                  //浮点数,double
        DT_DATE     = 'd',                                  //日期类型
        DT_TEXT     = 's'                                   //文本串类型
    }data_type_t;

    //-----------------------------------------------------
    //sql语句类型
    typedef enum sql_type_t
    {
        ST_UNKNOWN=-1,
        ST_SELECT   ,
        ST_UPDATE   ,
        ST_DELETE   ,
        ST_INSERT   ,
        ST_CREATE   ,
        ST_DROP     ,
        ST_ALTER    ,
        ST_BEGIN    ,
        ST_DECLARE  ,
        ST_SET
    }sql_type_t;
    
    //-----------------------------------------------------
    //DBC封装操作错误码
    typedef enum err_type_t
    {
        DBEC_OK = 0,
        DBEC_ENV_FAIL = 1000,                               //环境创建错误
        DBEC_NO_MEMORY,                                     //内存不足
        DBEC_NO_BUFFER,                                     //缓冲区不足
        DBEC_IDX_OVERSTEP,                                  //下标越界
        DBEC_BAD_PARAM,                                     //参数错误
        DBEC_BAD_INPUT,                                     //待绑定参数的数据类型错误
        DBEC_BAD_OUTPUT,                                    //不支持的输出数据类型
        DBEC_BAD_TYPEPREFIX,                                //参数自动绑定时,名字前缀不准确
        DBEC_UNSUP_TYPE,                                    //未支持的数据类型
        DBEC_PARAM_NOT_FOUND,                               //访问的参数对象不存在
        DBEC_FIELD_NOT_FOUND,                               //访问的列对象不存在
        DBEC_METHOD_CALL,                                   //方法调用的顺序错误
        DBEC_NOT_PARAM,                                     //sql语句中没有':'前缀的参数,但尝试绑定参数
        DBEC_PARSE_PARAM,                                   //sql语句自动解析参数错误

        DBEC_DB,                                            //DB错误
        DBEC_DB_BADPWD,                                     //DB错误细分:账号口令错误
        DBEC_DB_PWD_WILLEXPIRE,                             //DB错误细分:口令即将过期,不是致命错误但应该进行告警
        DBEC_DB_CONNTIMEOUT,                                //DB错误细分:连接超时
        DBEC_DB_CONNLOST,                                   //DB错误细分:已经建立的连接断开了.
        DBEC_DB_CONNFAIL,                                   //DB错误细分:连接失败,无法建立连接
        DBEC_DB_UNIQUECONST,                                //DB错误细分:唯一约束导致的错误

        DBEC_OTHER=-1,
    }err_type_t;


    inline const char* err_type_str(err_type_t dbc_err)
    {
        switch (dbc_err)
        {
        case	DBEC_ENV_FAIL:          return "(DBEC_ENV_FAIL):environment handle creation failed";
        case	DBEC_NO_MEMORY:         return "(DBEC_NO_MEMORY):memory allocation request has failed";
        case	DBEC_NO_BUFFER:         return "(DBEC_NO_BUFFER):memory buffer not enough";
        case	DBEC_IDX_OVERSTEP:      return "(DBEC_IDX_OVERSTEP):index access overstep the boundary";
        case	DBEC_BAD_PARAM:         return "(DBEC_BAD_PARAM):func param is incorrect";
        case	DBEC_BAD_INPUT:         return "(DBEC_BAD_INPUT):input bind data doesn't have expected type";
        case	DBEC_BAD_OUTPUT:        return "(DBEC_BAD_OUTPUT):output convert type incorrect";
        case	DBEC_BAD_TYPEPREFIX:    return "(DBEC_BAD_TYPEPREFIX):input bind parameter prefix incorrect";
        case	DBEC_UNSUP_TYPE:        return "(DBEC_UNSUP_TYPE):unsupported Oracle type - cannot be converted";
        case	DBEC_PARAM_NOT_FOUND:   return "(DBEC_PARAM_NOT_FOUND):name not found in statement's parameters";
        case	DBEC_FIELD_NOT_FOUND:   return "(DBEC_FIELD_NOT_FOUND):resultset doesn't contain field_t with such name";
        case    DBEC_METHOD_CALL:       return "(DBEC_METHOD_CALL):func method called order error";
        case    DBEC_NOT_PARAM:         return "(DBEC_NOT_PARAM):sql not parmas";
        case    DBEC_PARSE_PARAM:       return "(DBEC_PARSE_PARAM): auto bind sql param error";
        case    DBEC_DB:                return "(DBEC_DB_ERROR)";
        case    DBEC_DB_BADPWD:         return "(DBEC_DB_BADPWD)";
        case    DBEC_DB_PWD_WILLEXPIRE: return "(DBEC_DB_PWD_WILLEXPIRE)";
        case    DBEC_DB_CONNTIMEOUT:    return "(DBEC_DB_CONNTIMEOUT)";
        case    DBEC_DB_CONNLOST:       return "(DBEC_DB_CONNLOST)";
        case    DBEC_DB_CONNFAIL:       return "(DBEC_DB_CONNFAIL)";
        case    DBEC_DB_UNIQUECONST:    return "(DBEC_DB_UNIQUECONST)";
        default:                        return "(DBEC_UNKNOW_ERROR)";
        }
    }

}


#endif