#ifndef _RX_DBC_MYSQL_COMM_H_
#define _RX_DBC_MYSQL_COMM_H_

namespace rx_dbc_mysql
{
    //允许使用的字符串的最大长度
    const uint16_t MAX_TEXT_BYTES = 1024 * 2;

    //每次批量FEATCH获取的结果集的数量
    const uint16_t BAT_FETCH_SIZE = 20;

    //字段名字最大长度
    const uint16_t FIELD_NAME_LENGTH = 64;

    //sql语句的长度限制
    const int MAX_SQL_LENGTH = 1024 * 4;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //-----------------------------------------------------
    //dbc_ora可以处理的数据类型(绑定参数时,名字前缀可以告知数据类型)
    enum data_type_t
    {
        DT_UNKNOWN,
        DT_NUMBER   = 'n',                                  //数字类型的字段或参数
        DT_DATE     = 'd',                                  //日期类型
        DT_TEXT     = 's'                                   //文本串类型
    };

    //-----------------------------------------------------
    //sql语句类型
    enum sql_stmt_t
    {
        ST_UNKNOWN,
        ST_SELECT = 1,
        ST_UPDATE = 2,
        ST_DELETE = 3,
        ST_INSERT = 4,
        ST_CREATE = 5,
        ST_DROP   = 6,
        ST_ALTER  = 7,
        ST_BEGIN  = 8,
        ST_SET = 9
    };

    //-----------------------------------------------------
    //DBC封装操作错误码
    enum dbc_error_code_t
    {
        DBEC_OK=0,
        DBEC_ENV_FAIL = 1000,                               //OCI环境创建错误
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
    };

    inline const char* dbc_error_code_info(int16_t dbc_err)
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
        case	DBEC_UNSUP_TYPE:        return "(DBEC_UNSUP_TYPE):unsupported data type - cannot be converted";
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
        default:                        return "(unknown DBC Error)";
        }
    }

    //-----------------------------------------------------
    //环境选项
    typedef struct env_option_t
    {
        const char *charset;
        const char *language;

        env_option_t() { use_english(); }
        void use_chinese()
        {//中文环境,字符串中可以是直接的gbk字符集
            charset = "gbk";
            language = "en_US";
        }
        void use_english()
        {//英文环境,如果要插入中文,则需要进行gbk2utf8的转换
            charset = "utf8mb4";
            language = "en_US";
        }
    }env_option_t;

    //-----------------------------------------------------
    //连接参数
    typedef struct conn_param_t
    {
        char        host[64];                               //数据库服务器所在地址
        char        user[64];                               //数据库用户名
        char        pwd[64];                                //数据库口令
        char        db[64];                                 //数据库实例名
        uint32_t    port;                                   //数据库端口
        uint32_t    conn_timeout;                           //连接超时时间
        conn_param_t()
        {
            host[0] = 0;
            db[0] = 0;
            user[0] = 0;
            pwd[0] = 0;
            port = 3306;
            conn_timeout = 3;
        }
    }conn_param_t;

    //-------------------------------------------------
    //获取db更详细的错误信息
    inline bool get_last_error(int32_t &ec, char *buff, uint32_t max_size,MYSQL *handle)
    {
        bool get_details = false;
        rx::tiny_string_t<> desc(max_size, buff);
        
        if (handle)
        {
            ec = mysql_errno(handle);
            desc << "(DB_ERROR)<" << rx::n2s_t((uint32_t)ec) << ':';
            desc << mysql_error(handle) << '>';
        }
        else
        {
            ec = 0;
            desc = "(UNKNOWN_ERROR)";
        }
        return true;
    }
    //获取DB/STMT更详细的错误信息
    inline bool get_last_error(int32_t &ec, char *buff, uint32_t max_size, MYSQL_STMT *handle)
    {
        bool get_details = false;
        rx::tiny_string_t<> desc(max_size, buff);

        if (handle)
        {
            ec = mysql_stmt_errno(handle);
            desc << "(DB_ERROR)<" << rx::n2s_t((uint32_t)ec) << ':';
            desc << mysql_stmt_error(handle) << '>';
        }
        else
        {
            ec = 0;
            desc = "(UNKNOWN_ERROR)";
        }
        return true;
    }
    //-----------------------------------------------------
    //记录错误信息的功能类
    class error_info_t
    {
        static const uint32_t MAX_BUF_SIZE = 1024 * 2;
    private:
        int32_t		        m_dbc_ec;  	                    //DBC错误码
        int32_t		        m_mysql_ec;		                //mysql错误码
        char	            m_err_desc[MAX_BUF_SIZE];	    //错误内容

        //--------------------------------------------------
        //得到dn的错误详细信息
        void make_db_error_info(int32_t ec, const char *msg)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "MYSQL::" << msg;
            m_dbc_ec = DBEC_DB;
            m_mysql_ec = ec;
            desc.repleace('\n', '.');
            //进行OCI错误细分,映射到DBEC错误码
            switch (m_mysql_ec)
            {
            case ER_DUP_ENTRY:
                m_dbc_ec = DBEC_DB_UNIQUECONST; break;
            case ER_ACCESS_DENIED_ERROR:
                m_dbc_ec = DBEC_DB_BADPWD; break;
            case ER_BAD_DB_ERROR:
                m_dbc_ec = DBEC_DB_CONNFAIL; break;
            case CR_CONN_HOST_ERROR:
            case CR_UNKNOWN_HOST:
            case CR_SERVER_LOST:
                m_dbc_ec = DBEC_DB_CONNTIMEOUT; break;
            default:
                if (is_connection_lost())
                    m_dbc_ec = DBEC_DB_CONNLOST;
                else if (is_connect_fail())
                    m_dbc_ec = DBEC_DB_CONNFAIL;
                break;
            }
        }

        //-------------------------------------------------
        //得到当前库内的详细错误信息
        void make_dbc_error(int32_t dbc_err)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "DBC::" << dbc_error_code_info(dbc_err);
            m_dbc_ec = dbc_err;
            m_mysql_ec = 0;
        }

        //-------------------------------------------------
        //将输入的可变参数对应的串连接到详细错误信息中
        void make_attached_msg(const char *format, va_list va, const char *source_name = NULL, uint32_t line_number = -1)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc, rx::st::strlen(m_err_desc));
            if (!is_empty(format))
            {
                desc << " @ < ";
                desc(format, va) << " >";
            }

            if (!is_empty(source_name))
                desc << " # (" << source_name << ':' << rx::n2s_t(line_number) << ')';
        }
        //-------------------------------------------------
        //禁止拷贝赋值
        error_info_t& operator = (const error_info_t&);
    public:
        //-------------------------------------------------
        //构造函数,通过环境句柄得到db的详细错误信息
        error_info_t(MYSQL *handle, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            int32_t db_ec;
            char msg[1024];
            get_last_error(db_ec, msg, sizeof(msg),handle);
            make_db_error_info(db_ec, msg);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //构造函数,通过环境句柄得到db的详细错误信息
        error_info_t(MYSQL_STMT *handle, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            int32_t db_ec;
            char msg[1024];
            get_last_error(db_ec, msg, sizeof(msg), handle);
            make_db_error_info(db_ec, msg);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }        
        //-------------------------------------------------
        //构造函数,记录库内部错误
        error_info_t(int32_t dbc_err, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            make_dbc_error(dbc_err);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //绑定发生错误的数据库连接信息后再获取完整的错误输出
        const char* c_str(const conn_param_t &cp)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc, rx::st::strlen(m_err_desc));
            desc << "::host[" << cp.host << "]db[" << cp.db << "]user[" << cp.user << ']';
            return m_err_desc;
        }
        //-------------------------------------------------
        //得到错误的详细信息
        const char* c_str(void) { return m_err_desc; }
        //-------------------------------------------------
        //判断是否为db错误类别
        bool is_db_error() { return m_mysql_ec != 0; }
        //得到db错误代码
        uint32_t db_error_code() { return m_mysql_ec; }
        //得到dbc错误代码
        uint32_t dbc_error_code() { return m_dbc_ec; }
        //-------------------------------------------------
        //判断错误是否为用户名口令错误
        bool is_bad_user_pwd() { return m_mysql_ec == ER_ACCESS_DENIED_ERROR; }
        //判断是否为连接错误
        bool is_conn_timeout() { return m_mysql_ec == CR_SERVER_LOST; }
        //连接断开
        bool is_connection_lost() { return m_mysql_ec == CR_SERVER_LOST; }
        //连接失败(12541监听器不存在;或口令错误;或连接超时;或连接丢失)
        bool is_connect_fail() { return m_mysql_ec == CR_CONN_HOST_ERROR || m_mysql_ec == CR_UNKNOWN_HOST || m_mysql_ec == ER_BAD_DB_ERROR || is_bad_user_pwd() || is_conn_timeout() || is_connection_lost(); }
        //-------------------------------------------------
        //是否为唯一性约束冲突
        bool is_unique_constraint() { return m_mysql_ec == ER_DUP_ENTRY; }
    };

    //-------------------------------------------------
    //获取语句类型
    inline sql_stmt_t get_sql_type(const char* SQL)
    {
        if (is_empty(SQL))
            return ST_UNKNOWN;

        while (*SQL)
        {
            if (*SQL == ' ')
                ++SQL;
            else
                break;
        }

        char tmp[5]; tmp[4] = 0;
        rx::st::strncpy(tmp, SQL, 4);
        rx::st::strupr(tmp);

        switch (*(uint32_t*)tmp)
        {
        case 0x454c4553:return ST_SELECT;
        case 0x41445055:return ST_UPDATE;
        case 0x45535055:return ST_UPDATE;
        case 0x454c4544:return ST_DELETE;
        case 0x45534e49:return ST_INSERT;
        case 0x41455243:return ST_CREATE;
        case 0x504f5244:return ST_DROP;
        case 0x45544c41:return ST_ALTER;
        case 0x49474542:return ST_BEGIN;
        case 0x20544553:return ST_SET;
        default:return ST_UNKNOWN;
        }
    }

    //-----------------------------------------------------
    //操作db格式的日期时间的功能类
    class datetime_t
    {
        MYSQL_TIME m_Date;
    public:
        //-------------------------------------------------
        datetime_t(void) { memset(&m_Date, 0, sizeof(m_Date)); };
        datetime_t(uint16_t y, uint16_t m, uint16_t d, uint16_t h = 0, uint16_t mi = 0, uint16_t sec = 0)
        {
            set(y, m, d, h, mi, sec);
        }
        datetime_t(const MYSQL_TIME& o) { m_Date = o; }
        //-------------------------------------------------
        //设置各个分量
        void set(uint16_t y, uint16_t m, uint16_t d, uint16_t h = 0, uint16_t mi = 0, uint16_t sec = 0)
        {
            m_Date.year = y;
            m_Date.month = (uint8_t)m;
            m_Date.day = (uint8_t)d;
            m_Date.hour = (uint8_t)h;
            m_Date.minute = (uint8_t)mi;
            m_Date.second = (uint8_t)sec;
        }
        void set(const struct tm &ST)
        {
            year(ST.tm_year+1900);
            mon(ST.tm_mon+1);
            day((uint8_t)ST.tm_mday);
            hour((uint8_t)ST.tm_hour);
            minute((uint8_t)ST.tm_min);
            sec((uint8_t)ST.tm_sec);
        }
        //-------------------------------------------------
        //访问各个分量
        int16_t year(void) const { return m_Date.year; }
        void year(int16_t yy) { m_Date.year = yy; }

        uint8_t mon(void) const { return m_Date.month; }
        void mon(int mm) { m_Date.month = (uint8_t)mm; }

        uint8_t day(void) const { return m_Date.day; }
        void day(uint8_t dd) { m_Date.day = dd; }

        uint8_t hour(void) const { return m_Date.hour; }
        void hour(uint8_t hh) { m_Date.hour = hh; }

        uint8_t minute(void) const { return m_Date.minute; }
        void minute(uint8_t mi) { m_Date.minute = mi; }

        uint8_t sec(void) const { return m_Date.second; }
        void sec(uint8_t ss) { m_Date.second = ss; }
        //-------------------------------------------------
        datetime_t& operator = (const MYSQL_TIME& o) { m_Date = o; return (*this); }
        void to(MYSQL_TIME& o) const { o = m_Date; }
        //-------------------------------------------------
        //根据给定的格式,将内部日期转换为字符串形式
        char* to(char *Result, const char* Format = NULL) const
        {
            if (Format == NULL) Format = "%d-%02d-%02d %02d:%02d:%02d";
            sprintf(Result, Format, m_Date.year, m_Date.month, m_Date.day, m_Date.hour, m_Date.minute, m_Date.second);
            return Result;
        }
    };

    //声明可以对外使用的类
    class conn_t;
    class stmt_t;
    class query_t;
    class sql_param_t;
    class field_t;
    //-------------------------------------------------
    //将本命名空间中的对外开放类型进行统一声明
    class type_t
    {
    public:
        typedef data_type_t     data_type_t;
        typedef sql_stmt_t      sql_stmt_t;
        typedef conn_param_t    conn_param_t;
        typedef env_option_t    env_option_t;
        typedef dbc_error_code_t dbc_error_code_t;
        typedef error_info_t    error_info_t;
        typedef datetime_t      datetime_t;

        typedef conn_t          conn_t;
        typedef sql_param_t     sql_param_t;
        typedef stmt_t          stmt_t;

        typedef field_t         field_t;
        typedef query_t         query_t;
    };
}

#endif
