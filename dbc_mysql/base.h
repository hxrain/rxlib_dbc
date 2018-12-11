#ifndef _RX_DBC_MYSQL_COMM_H_
#define _RX_DBC_MYSQL_COMM_H_

namespace mysql
{
    //字段名字最大长度
    const uint16_t FIELD_NAME_LENGTH = 64;
    //dummy
    static const uint16_t BAT_BULKS_SIZE = 1;
    //-----------------------------------------------------

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
    //API库环境初始化
    class env_t
    {
    public:
        env_t() { mysql_library_init(0, NULL, NULL); }
        ~env_t() { mysql_library_end(); }
    };

    //-------------------------------------------------
    //获取db更详细的错误信息
    inline bool get_last_error(int32_t &ec, char *buff, uint32_t max_size,MYSQL *handle)
    {
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
        void make_dbc_error(err_type_t dbc_err)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "DBC::" << err_type_str(dbc_err);
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
        error_info_t(err_type_t dbc_err, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
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
    inline sql_type_t get_sql_type(const char* SQL)
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
}

#endif
