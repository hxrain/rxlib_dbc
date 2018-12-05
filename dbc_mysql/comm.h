#ifndef _RX_DBC_MYSQL_COMM_H_
#define _RX_DBC_MYSQL_COMM_H_

namespace rx_dbc_mysql
{
    //允许使用的字符串的最大长度
    const uint16_t MAX_TEXT_BYTES = 1024 * 2;

    //每次批量FEATCH获取的结果集的数量
    const uint16_t BAT_FETCH_SIZE = 20;

    //字段名字最大长度
    const uint16_t FIELD_NAME_LENGTH = 32;

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
        ST_DECLARE = 9,
        ST_SET = 10
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
        {//中文环境
            charset = "gbk";
            language = "en_US";
        }
        void use_english()
        {//英文环境
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
            port = 1521;
            conn_timeout = 3;
        }
    }conn_param_t;

    //-------------------------------------------------
    //根据OCI返回值,获取更详细的错误信息
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
        //得到Oracle的错误详细信息
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
            case ER_DUP_ENTRY:m_dbc_ec = DBEC_DB_UNIQUECONST; break;
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
        //构造函数,通过环境句柄得到Oracle的详细错误信息
        error_info_t(MYSQL *handle, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            int32_t oci_ec;
            char msg[1024];
            get_last_error(oci_ec, msg, sizeof(msg),handle);
            make_db_error_info(oci_ec, msg);

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


    //-----------------------------------------------------
    //操作Oracle格式的日期时间的功能类
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

    //-----------------------------------------------------
    //字段与绑定参数使用的基类,进行数据缓冲区的管理与字段数据的访问
    class col_base_t
    {
    protected:
        typedef rx::buff_t array_datasize_t;                //uint16_t
        typedef rx::buff_t array_databuff_t;                //uint8_t
        typedef rx::buff_t array_dataempty_t;               //int16_t
        typedef rx::tiny_string_t<char, FIELD_NAME_LENGTH> col_name_t;

        col_name_t          m_name;		                    // 对象名字
        data_type_t	        m_dbc_data_type;		        // 期待的数据类型
        int				    m_max_data_size;			    // 每个字段数据最大尺寸

        static const int    m_working_buff_size = 64;       // 临时存放转换字符串的缓冲区
        char                m_working_buff[m_working_buff_size];

        array_datasize_t    m_col_datasize;		            // 文本字段每行的长度数组
        array_databuff_t    m_col_databuff;	                // 记录该字段的每行的实际数据的数组
        array_dataempty_t   m_col_dataempty;	            // 标记该字段的值是否为空的状态数组: 0 - ok; -1 - null

        //-------------------------------------------------
        //字段构造函数,只能被记录集类使用
        void make(const char *name, uint32_t name_len, data_type_t dbc_data_type, uint32_t max_data_size, int bulk_row_count, bool make_datasize_array = false)
        {
            rx_assert(!is_empty(name));
            reset();

            m_name.assign(name, name_len);
            m_dbc_data_type = dbc_data_type;
            m_max_data_size = max_data_size;

            if (make_datasize_array && !m_col_datasize.make<uint16_t>(bulk_row_count))
            {
                reset();
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }

            //生成字段数据缓冲区
            m_col_databuff.make(m_max_data_size * bulk_row_count);
            //生成字段是否为空的数组
            m_col_dataempty.make<int16_t>(bulk_row_count);
            if (!m_col_dataempty.capacity() || !m_col_databuff.capacity())
            {//判断是否内存不足
                reset();
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-------------------------------------------------
        void reset()
        {
            m_col_datasize.clear();
            m_col_databuff.clear();
            m_col_dataempty.clear();
            m_dbc_data_type = DT_UNKNOWN;
            m_max_data_size = 0;
        }
        //-------------------------------------------------
        //根据字段类型与字段数据缓冲区以及当前行号,进行正确的数据偏移调整
        //入口:行数据数组;数据类型;当前行偏移;字段数据最大尺寸
        uint8_t* get_data_buff(int RowNo) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                return m_col_databuff.ptr(RowNo*m_max_data_size);
            case DT_NUMBER:
            case DT_DATE:
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为字符串:错误句柄;原始数据缓冲区;原始数据类型;临时字符串缓冲区;临时缓冲区尺寸;转换格式
        PStr comm_as_string(uint8_t* data_buff,const char* ConvFmt = NULL) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                return (reinterpret_cast <PStr> (data_buff));
            case DT_NUMBER:
            {
            }
            case DT_DATE:
            {
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为浮点数:错误句柄;原始数据缓冲区;原始数据类型;
        template<class DT>
        DT comm_as_double(uint8_t* data_buff) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
            {//文本转换为double值
            }
            case DT_NUMBER:
            {//OCI数字转换为double
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为带符号整型数:错误句柄;原始数据缓冲区;原始数据类型;
        int32_t comm_as_long(uint8_t* data_buff,bool is_signed = true) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                return rx::st::atoi((char*)data_buff);
            case DT_NUMBER:
            {
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为日期:错误句柄;原始数据缓冲区;原始数据类型;
        datetime_t comm_as_datetime(uint8_t* data_buff) const
        {
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
        }
        //-------------------------------------------------
        //不可直接使用本功能类,必须被继承
        col_base_t(rx::mem_allotter_i &ma) :m_col_datasize(ma), m_col_databuff(ma), m_col_dataempty(ma) { reset(); }
        virtual ~col_base_t() { reset(); }
        bool m_is_null(uint16_t idx) const { return (m_col_dataempty.at<int16_t>(idx) == -1); }
    protected:
        //-------------------------------------------------
        //子类需要覆盖实现的具体功能函数接口
        //-------------------------------------------------
        //得到错误句柄
        virtual void* oci_err_handle() const = 0;
        //得到当前的访问行号
        virtual uint16_t bulk_row_idx() const = 0;
    public:
        //-------------------------------------------------
        const char* name()const { return m_name.c_str(); }
        data_type_t dbc_data_type() { return m_dbc_data_type; }
        int max_data_size() { return m_max_data_size; }
        //-------------------------------------------------
        bool is_null(void) const { return m_is_null(bulk_row_idx()); }
        //-------------------------------------------------
        //尝试获取内部数据为字符串
        PStr as_string(const char* ConvFmt = NULL) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return NULL;
            return comm_as_string(get_data_buff(idx), ConvFmt);
        }
        //-------------------------------------------------
        //尝试获取内部数据为浮点数
        double as_double(double DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<double>(get_data_buff(idx));
        }
        //-------------------------------------------------
        //尝试获取内部数据为高精度浮点数
        long double as_real(long double DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<long double>(get_data_buff(idx));
        }
        //-------------------------------------------------
        //尝试获取内部数据为超大整数
        int64_t as_bigint(int64_t DefValue = 0) const { return int64_t(as_real((long double)DefValue)); }
        //-------------------------------------------------
        //尝试获取内部数据为带符号整数
        int32_t as_long(int32_t DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx));
        }
        //-------------------------------------------------
        //尝试获取内部数据为无符号整数
        uint32_t as_ulong(uint32_t DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx), false);
        }
        //-------------------------------------------------
        //尝试获取内部数据为日期时间
        datetime_t as_datetime(void) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return datetime_t();
            return comm_as_datetime(get_data_buff(idx));
        }
    };
}

#endif
