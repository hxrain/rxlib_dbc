#ifndef _RX_DBC_ORA_COMM_H_
#define _RX_DBC_ORA_COMM_H_

    //-----------------------------------------------------
    //在调试状态下,用外部的内存管理函数替代OCI内部的内存管理函数,方便外面进行资源泄露的诊断
    #if RX_DEF_ALLOC_USE_STD
        inline dvoid *DBC_ORA_Malloc (dvoid * ctxp, size_t size) { return malloc(size); }
        inline dvoid *DBC_ORA_Realloc (dvoid * ctxp, dvoid *ptr, size_t size){ return realloc(ptr, size); }
        inline void   DBC_ORA_Free (dvoid * ctxp, dvoid *ptr){ free(ptr); }
    #else
        #define	DBC_ORA_Malloc		NULL
        #define	DBC_ORA_Realloc	    NULL
        #define	DBC_ORA_Free		NULL
    #endif

namespace rx_dbc_ora
{
    //允许使用的字符串的最大长度
    const ub2 MAX_TEXT_BYTES = 1024 * 2;

    //每次批量FEATCH获取的结果集的数量
    const ub2 BAT_FETCH_SIZE = 20;

    //字段名字最大长度
    const ub2 FIELD_NAME_LENGTH = 30;

    //sql语句的长度限制
    const int MAX_SQL_LENGTH = 1024 * 8;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //OCI数字格式:去除空格,最大允许x个整数位,小数点最小保留2位最大保留14位
    const char* NUMBER_FRM_FMT = "FM999999999999999999.00999999999999";
    const ub2 NUMBER_FRM_FMT_LEN = 35;
    //-----------------------------------------------------
    //dbc_ora可以处理的数据类型(绑定参数时,名字前缀可以告知数据类型)
    enum data_type_t
    {
        DT_UNKNOWN,
        DT_NUMBER   = 'n',
        DT_DATE     = 'd',
        DT_TEXT     = 's'
    };

    //-----------------------------------------------------
    //sql语句类型
    enum sql_stmt_t
    {
        ST_UNKNOWN,
        ST_SELECT = OCI_STMT_SELECT,
        ST_UPDATE = OCI_STMT_UPDATE,
        ST_DELETE = OCI_STMT_DELETE,
        ST_INSERT = OCI_STMT_INSERT,
        ST_CREATE = OCI_STMT_CREATE,
        ST_DROP   = OCI_STMT_DROP,
        ST_ALTER  = OCI_STMT_ALTER,
        ST_BEGIN  = OCI_STMT_BEGIN,
        ST_DECLARE = OCI_STMT_DECLARE
    };

    //-----------------------------------------------------
    //错误类别
    enum error_class_t
    {
        ET_UNKNOWN = 0,
        ET_ORACLE,
        ET_DBC,
    };
    inline const char* error_class_name(int ErrType)
    {
        switch (ErrType)
        {
        case ET_ORACLE:return "oci";
        case ET_DBC:return "dbc";
        case ET_UNKNOWN:
        default:return "unknown";
        }
    }
    //-----------------------------------------------------
    //DBC封装操作错误码
    enum dbc_error_code_t
    {
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
    };
    inline const char* dbc_error_code_info(sword dbc_err)
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
        default:                        return "(unknown DBC Error)";
        }
    }

    //-----------------------------------------------------
    //OCI连接的环境选项
    typedef struct env_option_t
    {
        ub2   charset_id;
        const char *charset;
        const char *language;
        const char *date_format;

        env_option_t() { use_english(); }
        void use_chinese()
        {//中文环境
            charset_id = 852;
            charset = "zhs16gbk";
            language = "SIMPLIFIED CHINESE";
            date_format = "yyyy-mm-dd hh24:mi:ss";
        }
        void use_english()
        {//英文环境
            charset_id = 871;
            charset = "UTF8";
            language = "AMERICAN";
            date_format = "yyyy-mm-dd hh24:mi:ss";
        }
    }env_option_t;

    //-----------------------------------------------------
    //Oracle连接参数
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

    //-----------------------------------------------------
    //记录错误信息的功能类
    class error_info_t
    {
        static const ub4 MAX_BUF_SIZE = 1024 * 2;
    private:
        error_class_t	    m_err_type;		                //error type
        sword		        m_dbc_ec;  	                    //DBC错误码
        sb4			        m_ora_ec;		                //Oracle错误码,ORA-xxxxx
        char	            m_err_desc[MAX_BUF_SIZE];	    //错误内容
        char                m_out_buff[MAX_BUF_SIZE];       //返回完整消息时使用的缓冲串

        char                m_bind_host[64];                //绑定过的主机名
        char                m_bind_sid[64];                 //绑定过的主机Oracle实例名字
        char                m_bind_user[64];                //绑定过的用户名
        char	            m_src_file[256];	            // source file, where error was thrown (optional)
        long		        m_src_file_lineno;		        // line number, where error was thrown (optional)

        //--------------------------------------------------
        //得到Oracle的错误详细信息
        void make_oci_error_info(sword oci_result, OCIError *error_handle, OCIEnv *env_handle)
        {
            bool get_details = false;
            rx::tiny_string_ct desc(sizeof(m_err_desc), m_err_desc);
            m_dbc_ec = 0;
            m_err_type = ET_ORACLE;

            if (error_handle == NULL && env_handle == NULL)
            {
                desc = "(OCI_ENV_NULL)";
                return;
            }

            switch (oci_result)
            {
            case	OCI_SUCCESS:                desc = "(OCI_SUCCESS)";
                break;
            case	OCI_SUCCESS_WITH_INFO:      desc = "(OCI_SUCCESS_WITH_INFO)";
                get_details = true; break;
            case	OCI_ERROR:                  desc = "";
                get_details = true; break;
            case	OCI_NO_DATA:                desc = "(OCI_NO_DATA)";
                get_details = true; break;
            case	OCI_INVALID_HANDLE:         desc = "(OCI_INVALID_HANDLE)";
                break;
            case	OCI_NEED_DATA:              desc = "(OCI_NEED_DATA)";
                break;
            case	OCI_STILL_EXECUTING:        desc = "(OCI_STILL_EXECUTING)";
                get_details = true; break;
            case	OCI_CONTINUE:               desc = "(OCI_CONTINUE)";
                break;
            default:                            desc = "unknown";
            }

            // get detailed error m_err_desc
            if (get_details)
            {
                char Tmp[MAX_BUF_SIZE];
                if (error_handle)
                    OCIErrorGet(error_handle, 1, NULL, &m_ora_ec, reinterpret_cast<text *> (Tmp), MAX_BUF_SIZE, OCI_HTYPE_ERROR);
                else
                    OCIErrorGet(env_handle, 1, NULL, &m_ora_ec, reinterpret_cast<text *> (Tmp), MAX_BUF_SIZE, OCI_HTYPE_ENV);
                desc << '<' << Tmp << '>';
            }
        }

        //-------------------------------------------------
        //得到当前库内的详细错误信息
        void make_dbc_error(sword dbc_err)
        {
            rx::tiny_string_ct desc(sizeof(m_err_desc), m_err_desc);
            m_err_type = ET_DBC;
            m_dbc_ec = dbc_err;
            m_ora_ec = 0;
            desc = dbc_error_code_info(dbc_err);
        }

        //-------------------------------------------------
        //将输入的可变参数对应的串连接到详细错误信息中
        void make_attached_msg(const char *format, va_list va, const char *source_name = NULL, long line_number = -1)
        {
            m_out_buff[0] = 0;
            if (format)
            {
                //临时使用存放错误信息的缓冲区尺寸
                const ub2 ERROR_FORMAT_MAX_MSG_LEN = 1024;
                rx_assert(!is_empty(format) && va);
                char Tmp[ERROR_FORMAT_MAX_MSG_LEN];
                vsnprintf(Tmp, ERROR_FORMAT_MAX_MSG_LEN - 1, format, va);
                rx::tiny_string_ct desc(sizeof(m_err_desc), m_err_desc,rx::st::strlen(m_err_desc));
                desc << " @ < " << Tmp <<" >";
            }
            if (!is_empty(source_name))
            {
                rx::st::strcpy(m_src_file, sizeof(m_src_file), source_name);
                m_src_file_lineno = line_number;
            }
            m_bind_host[0] = 0;
        }
        //-------------------------------------------------
        //禁止拷贝赋值
        error_info_t& operator = (const error_info_t&);
    public:
        //-------------------------------------------------
        //构造函数,根据错误句柄得到Oracle的详细错误信息
        error_info_t(sword oci_result, OCIError *error_handle, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_oci_error_info(oci_result, error_handle, NULL);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va,source_name,line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //构造函数,通过环境句柄得到Oracle的详细错误信息
        error_info_t(sword oci_result, OCIEnv *env_handle, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_oci_error_info(oci_result, NULL, env_handle);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //构造函数,记录库内部错误
        error_info_t(sword dbc_err, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_dbc_error(dbc_err);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //绑定发生错误的数据库连接信息后再获取完整的错误输出
        const char* c_str(const char* Host, const char* sid, const char* user)
        {
            rx::st::strcpy(m_bind_host, MAX_PATH, Host);
            rx::st::strcpy(m_bind_sid, MAX_PATH, sid);
            rx::st::strcpy(m_bind_user, MAX_PATH, user);
            m_out_buff[0]=0;
            return c_str();
        }
        const char* c_str(const conn_param_t &cp) { return c_str(cp.host,cp.db,cp.user); }
        //-------------------------------------------------
        //得到错误的详细信息
        const char* c_str(void)
        {
            if (is_empty(m_out_buff))
            {
                rx::st::replace(m_err_desc, '\n', ' ');
                if (!m_bind_host[0])
                    snprintf(m_out_buff, sizeof(m_out_buff), "%s::%s", error_class_name(m_err_type), m_err_desc);
                else
                    snprintf(m_out_buff, sizeof(m_out_buff), "%s::host[%s],db[%s],user[%s]::%s", error_class_name(m_err_type), m_bind_host, m_bind_sid, m_bind_user, m_err_desc);
            }
            return m_out_buff;
        }
        //-------------------------------------------------
        //判断是否为OCI错误类别
        bool is_oci_error() { return m_err_type == ET_ORACLE; }
        //得到oci错误代码
        ub4 oci_error_code() { return m_ora_ec; }
        //判断错误是否为用户名口令错误
        bool is_bad_user_pwd() { return m_ora_ec == 1017; }
        //判断是否为连接错误
        bool is_conn_timeout() { return m_ora_ec == 12170; }
        //密码即将过期
        bool is_pwd_will_expire() { return m_ora_ec == 28002; }
        //连接断开
        bool is_connection_lost() { return m_ora_ec == 3135|| m_ora_ec == 3114 || m_ora_ec == 3113; }
        //连接失败(12541监听器不存在;或口令错误;或连接超时;或连接丢失)
        bool is_connect_fail() { return m_ora_ec == 12541 || is_bad_user_pwd() || is_conn_timeout() || is_connection_lost(); }
    };

    //-----------------------------------------------------
    //操作Oracle格式的日期时间的功能类
    class datetime_t
    {
        OCIDate m_Date;
    public:
        //-------------------------------------------------
        datetime_t(void) { memset(&m_Date, 0, sizeof(m_Date)); };
        datetime_t(ub2 y, ub2 m, ub2 d, ub2 h = 0, ub2 mi = 0, ub2 sec = 0)
        {
            set(y, m, d, h, mi, sec);
        }
        datetime_t(const OCIDate& o) { m_Date = o; }
        //-------------------------------------------------
        //设置各个分量
        void set(ub2 y, ub2 m, ub2 d, ub2 h = 0, ub2 mi = 0, ub2 sec = 0)
        {
            m_Date.OCIDateYYYY = y;
            m_Date.OCIDateMM = (ub1)m;
            m_Date.OCIDateDD = (ub1)d;
            m_Date.OCIDateTime.OCITimeHH = (ub1)h;
            m_Date.OCIDateTime.OCITimeMI = (ub1)mi;
            m_Date.OCIDateTime.OCITimeSS = (ub1)sec;
        }
        void set(const struct tm &ST)
        {
            year(ST.tm_year+1900);
            mon(ST.tm_mon+1);
            day((ub1)ST.tm_mday);
            hour((ub1)ST.tm_hour);
            minute((ub1)ST.tm_min);
            sec((ub1)ST.tm_sec);
        }
        //-------------------------------------------------
        //访问各个分量
        sb2 year(void) const { return m_Date.OCIDateYYYY; }
        void year(sb2 yy) { m_Date.OCIDateYYYY = yy; }

        ub1 mon(void) const { return m_Date.OCIDateMM; }
        void mon(int mm) { m_Date.OCIDateMM = (ub1)mm; }

        ub1 day(void) const { return m_Date.OCIDateDD; }
        void day(ub1 dd) { m_Date.OCIDateDD = dd; }

        ub1 hour(void) const { return m_Date.OCIDateTime.OCITimeHH; }
        void hour(ub1 hh) { m_Date.OCIDateTime.OCITimeHH = hh; }

        ub1 minute(void) const { return m_Date.OCIDateTime.OCITimeMI; }
        void minute(ub1 mi) { m_Date.OCIDateTime.OCITimeMI = mi; }

        ub1 sec(void) const { return m_Date.OCIDateTime.OCITimeSS; }
        void sec(ub1 ss) { m_Date.OCIDateTime.OCITimeSS = ss; }
        //-------------------------------------------------
        datetime_t& operator = (const OCIDate& o) { m_Date = o; return (*this); }
        void to(OCIDate& o) const { o = m_Date; }
        //-------------------------------------------------
        //根据给定的格式,将内部日期转换为字符串形式
        char* to(char *Result, const char* Format = NULL) const
        {
            if (Format == NULL) Format = "%d-%02d-%02d %02d:%02d:%02d";
            sprintf(Result, Format, m_Date.OCIDateYYYY, m_Date.OCIDateMM, m_Date.OCIDateDD, m_Date.OCIDateTime.OCITimeHH, m_Date.OCIDateTime.OCITimeMI, m_Date.OCIDateTime.OCITimeSS);
            return Result;
        }
    };

    //-----------------------------------------------------
    //字段与绑定参数使用的基类,进行数据缓冲区的管理与字段数据的访问
    class col_base_t
    {
    protected:
        typedef rx::buff_t array_datasize_t;                //ub2
        typedef rx::buff_t array_databuff_t;                //ub1
        typedef rx::buff_t array_dataempty_t;               //sb2
        typedef rx::tiny_string_t<char, FIELD_NAME_LENGTH> col_name_t;

        col_name_t          m_name;		                    // 对象名字
        data_type_t	        m_dbc_data_type;		        // 期待的数据类型
        ub2				    m_oci_data_type;		        // Oracle实际数据类型,二者用于自动转换
        int				    m_max_data_size;			    // 每个字段数据最大尺寸

        static const int    m_working_buff_size = 64;       // 临时存放转换字符串的缓冲区
        char                m_working_buff[m_working_buff_size];

        array_datasize_t    m_col_datasize;		            // 文本字段没行的长度数组
        array_databuff_t    m_col_databuff;	                // 记录该字段的每行的实际数据的数组
        array_dataempty_t   m_col_dataempty;	            // 标记该字段的值是否为空的状态数组: 0 - ok; -1 - null

        //-------------------------------------------------
        //字段构造函数,只能被记录集类使用
        void make(const char *name, ub4 name_len, ub2 oci_data_type, data_type_t dbc_data_type, ub4 max_data_size, int bulk_row_count, bool make_datasize_array = false)
        {
            rx_assert(!is_empty(name));
            reset();

            m_name.reset(name, name_len);
            m_oci_data_type = oci_data_type;
            m_dbc_data_type = dbc_data_type;
            m_max_data_size = max_data_size;

            if (make_datasize_array && !m_col_datasize.make<ub2>(bulk_row_count))
            {
                reset();
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }

            //生成字段数据缓冲区
            m_col_databuff.make(m_max_data_size * bulk_row_count);
            //生成字段是否为空的数组
            m_col_dataempty.make<sb2>(bulk_row_count);
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
            m_oci_data_type = 0;
            m_max_data_size = 0;
        }
        //-------------------------------------------------
        //根据字段类型与字段数据缓冲区以及当前行号,进行正确的数据偏移调整
        //入口:行数据数组;数据类型;当前行偏移;字段数据最大尺寸
        ub1* get_data_buff(int RowNo) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                return m_col_databuff.ptr(RowNo*m_max_data_size);
            case DT_NUMBER:
                return (ub1*)m_col_databuff.ptr<OCINumber>(RowNo);
            case DT_DATE:
                return (ub1*)m_col_databuff.ptr<OCIDate>(RowNo);
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为字符串:错误句柄;原始数据缓冲区;原始数据类型;临时字符串缓冲区;临时缓冲区尺寸;转换格式
        PStr comm_as_string(ub1* data_buff,const char* ConvFmt = NULL) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                return (reinterpret_cast <PStr> (data_buff));
            case DT_NUMBER:
            {
                ub4 FmtLen = 3;
                if (ConvFmt == NULL) ConvFmt = "TM9";
                else FmtLen = rx::st::strlen(ConvFmt);
                ub4 ValueSize = m_working_buff_size;
                sword result = OCINumberToText(oci_err_handle(), reinterpret_cast <OCINumber *> (data_buff), (oratext*)ConvFmt, FmtLen, NULL, 0, &ValueSize, (oratext*)m_working_buff);
                if (result == OCI_SUCCESS)
                    return (m_working_buff);
                else
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
            case DT_DATE:
            {
                datetime_t DT(*reinterpret_cast <OCIDate *> (data_buff));
                DT.to((char*)m_working_buff, ConvFmt);
                return m_working_buff;
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为浮点数:错误句柄;原始数据缓冲区;原始数据类型;
        template<class DT>
        DT comm_as_double(ub1* data_buff) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
            {//文本转换为double值
                //借用oci的方法,先把文本串转换为OCI的number
                OCINumber num;
                sword result = ::OCINumberFromText(oci_err_handle(), (const oratext*)data_buff, rx::st::strlen((char*)data_buff), (const oratext*)NUMBER_FRM_FMT, NUMBER_FRM_FMT_LEN, (const oratext*)"NLS_NUMERIC_CHARACTERS='.''", 27, &num);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__, "col(%s)", m_name.c_str()));

                //再将OCI的number转换为原生double
                DT	value;
                result = ::OCINumberToReal(oci_err_handle(), &num, sizeof(DT), &value);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__, "col(%s)", m_name.c_str()));

                return value;
            }
            case DT_NUMBER:
            {//OCI数字转换为double
                DT	value;
                sword result = ::OCINumberToReal(oci_err_handle(), reinterpret_cast <OCINumber *>(data_buff), sizeof(DT), &value);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__, "col(%s)", m_name.c_str()));
                return value;
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为带符号整型数:错误句柄;原始数据缓冲区;原始数据类型;
        int32_t comm_as_long(ub1* data_buff,bool is_signed = true) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                return rx::st::atoi((char*)data_buff);
            case DT_NUMBER:
            {
                int32_t	value;
                sword result = OCINumberToInt(oci_err_handle(), reinterpret_cast <OCINumber *> (data_buff), sizeof(int32_t), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED, &value);
                if (result == OCI_SUCCESS)
                    return (value);
                else
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__,"col(%s)",m_name.c_str()));
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //统一功能函数:将指定的原始类型的数据转换为日期:错误句柄;原始数据缓冲区;原始数据类型;
        datetime_t comm_as_datetime(ub1* data_buff) const
        {
            if (m_dbc_data_type == DT_DATE)
                return (datetime_t(*(reinterpret_cast <OCIDate *> (data_buff))));
            else
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
        }
        //-------------------------------------------------
        //不可直接使用本功能类,必须被继承
        col_base_t(rx::mem_allotter_i &ma) :m_col_datasize(ma), m_col_databuff(ma), m_col_dataempty(ma) { reset(); }
        virtual ~col_base_t() { reset(); }
        bool m_is_null(ub2 idx) const { return (m_col_dataempty.at<sb2>(idx) == -1); }
    protected:
        //-------------------------------------------------
        //子类需要覆盖实现的具体功能函数接口
        //-------------------------------------------------
        //得到错误句柄
        virtual OCIError* oci_err_handle() const = 0;
        //得到当前的访问行号
        virtual ub2 bulk_row_idx() const = 0;
    public:
        //-------------------------------------------------
        const char* name()const { return m_name.c_str(); }
        data_type_t dbc_data_type() { return m_dbc_data_type; }
        int max_data_size() { return m_max_data_size; }
        ub2 oci_data_type() { return m_oci_data_type; }
        //-------------------------------------------------
        bool is_null(void) const { return m_is_null(bulk_row_idx()); }
        //-------------------------------------------------
        //尝试获取内部数据为字符串
        PStr as_string(const char* ConvFmt = NULL) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return NULL;
            return comm_as_string(get_data_buff(idx), ConvFmt);
        }
        //-------------------------------------------------
        //尝试获取内部数据为浮点数
        double as_double(double DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<double>(get_data_buff(idx));
        }
        //-------------------------------------------------
        //尝试获取内部数据为高精度浮点数
        long double as_real(long double DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
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
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx));
        }
        //-------------------------------------------------
        //尝试获取内部数据为无符号整数
        uint32_t as_ulong(uint32_t DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx), false);
        }
        //-------------------------------------------------
        //尝试获取内部数据为日期时间
        datetime_t as_datetime(void) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return datetime_t();
            return comm_as_datetime(get_data_buff(idx));
        }
    };
}

#endif
