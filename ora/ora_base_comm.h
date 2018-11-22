#ifndef _RX_DBC_ORA_COMM_H_
#define _RX_DBC_ORA_COMM_H_

//定义中文环境字符集ID
#define OCI_ZHS16GBK_CHARSET_ID 852
#define OCI_ZHS16GBK_CHARSET "zhs16gbk"
#define OCI_LANGUAGE_CHINA "SIMPLIFIED CHINESE"
#define OCI_DEF_DATE_FORMAT "yyyy-mm-dd hh24:mi:ss"

namespace rx_dbc_ora
{
    //对于参数来说,允许缓存的字符串的最大长度
    const ub2 MAX_TEXT_BYTES = 1024 * 2;

    //每次批量FEATCH获取的数据行的数量
    const ub2 FETCH_SIZE = 100;

    //字段名字最大长度
    const ub2 FIELD_NAME_LENGTH = 60;

    //临时使用存放错误信息的缓冲区尺寸
    const ub2 ERROR_FORMAT_MAX_MSG_LEN = 1024;

    //SQL语句的长度限制
    const int MAX_SQL_LENGTH = 1024 * 32;

    typedef std::string String;
    typedef const char *PStr;
    const int CHAR_SIZE = sizeof(char);

    //-----------------------------------------------------
    //OLIB可以处理的数据类型
    enum data_type_t
    {
        DT_UNKNOWN,
        DT_NUMBER,
        DT_DATE,
        DT_TEXT
    };

    //-----------------------------------------------------
    //绑定参数时,名字前缀可以告知数据类型
    enum param_prex_type_t
    {
        PP_NUMERIC = 'n',
        PP_DATE = 'd',
        PP_TEXT = 's',
        PP_RESULT_SET = 'c'
    };

    //-----------------------------------------------------
    //SQL语句类型
    enum sql_stmt_t
    {
        ST_UNKNOWN,
        ST_SELECT = OCI_STMT_SELECT,
        ST_UPDATE = OCI_STMT_UPDATE,
        ST_DELETE = OCI_STMT_DELETE,
        ST_INSERT = OCI_STMT_INSERT,
        ST_CREATE = OCI_STMT_CREATE,
        ST_DROP = OCI_STMT_DROP,
        ST_ALTER = OCI_STMT_ALTER,
        ST_BEGIN = OCI_STMT_BEGIN,
        ST_DECLARE = OCI_STMT_DECLARE
    };

    //-----------------------------------------------------
    //本库中抛出的异常错误码
    enum error_code_t
    {
        EC_OCI_ERROR = -1,
        EC_ENV_CREATE_FAILED = 1000,
        EC_TIMEOUT,
        EC_NO_MEMORY,
        EC_NO_BUFFER,
        EC_BAD_PARAM_TYPE,
        EC_POOL_NOT_SETUP,
        EC_BAD_INPUT_TYPE,
        EC_BAD_OUTPUT_TYPE,
        EC_BAD_TRANSFORM,
        EC_BAD_PARAM_PREFIX,
        EC_UNSUP_ORA_TYPE,
        EC_PARAMETER_NOT_FOUND,
        EC_COLUMN_NOT_FOUND,
        EC_INTERNAL,
        EC_PARAMNAME_DUP,                                   //绑定的参数名字重复
        EC_PARAMNAME_TOO_MANY,                              //绑定的参数数量太多了,与实际初始化告知的数量不符
        EC_METHOD_ORDER,                                    //方法调用的顺序错误
        EC_METHOD_PARAM,                                    //方法调用时参数错误
        EC_SQL_NOT_PARAM,                                   //SQL语句中没有:前缀的参数,但尝试绑定参数
        EC_PARAM_BULKDATA_TOO_MANY                          //参数要绑定的批量数据太多了
    };
    inline const char* error_code_info(sword olib_err)
    {
        switch (olib_err)
        {
        case	EC_ENV_CREATE_FAILED:           return "(EC_ENV_CREATE_FAILED) Environment handle creation failed";
        case	EC_TIMEOUT:                     return "(EC_TIMEOUT) Statement took too long to complete and has been aborted";
        case	EC_NO_MEMORY:                   return "(EC_NO_MEMORY) Memory allocation request has failed";
        case	EC_BAD_PARAM_TYPE:              return "(EC_BAD_PARAM_TYPE) Parameter type is incorrect";
        case	EC_POOL_NOT_SETUP:              return "(EC_POOL_NOT_SETUP) Connection pool has not been setup yet";
        case	EC_BAD_INPUT_TYPE:              return "(EC_BAD_INPUT_TYPE) Input data doesn't have expected type";
        case	EC_BAD_OUTPUT_TYPE:             return "(EC_BAD_OUTPUT_TYPE) Cannot convert to requested type";
        case	EC_BAD_TRANSFORM:               return "(EC_BAD_TRANSFORM) Requested transformation is not possible";
        case	EC_BAD_PARAM_PREFIX:            return "(EC_BAD_PARAM_PREFIX) Parameter prefix is not known";
        case	EC_INTERNAL:                    return "(EC_INTERNAL) Internal library error. Please, report to developers";
        case	EC_UNSUP_ORA_TYPE:              return "(EC_UNSUP_ORA_TYPE) Unsupported Oracle type - cannot be converted to numeric, date or text";
        case	EC_PARAMETER_NOT_FOUND:         return "(EC_PARAMETER_NOT_FOUND) Name not found in statement's parameters";
        case	EC_COLUMN_NOT_FOUND:            return "(EC_COLUMN_NOT_FOUND) Result set doesn't contain field_t with such name";
        case    EC_PARAMNAME_DUP:               return "(EC_PARAMNAME_DUP) binded param name is duplication";   //绑定的参数名字重复
        case    EC_METHOD_ORDER:                return "(EC_METHOD_ORDER) func method called order error";      //方法调用的顺序错误
        case    EC_METHOD_PARAM:                return "(EC_METHOD_PARAM) func method call param error";        //方法调用时参数错误
        case    EC_PARAMNAME_TOO_MANY:          return "(EC_PARAMNAME_TOO_MANY) param count too many";          //待绑定的参数数量过多
        case    EC_SQL_NOT_PARAM:               return "(EC_SQL_NOT_PARAM) SQL not parmas";                     //待绑定的参数数量过少
        case    EC_PARAM_BULKDATA_TOO_MANY:     return "(EC_PARAM_BULKDATA_TOO_MANY) Param bind bulk data is too many";
        default:                                return "unknown";
        }
    }
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
    //月份枚举
    enum month_name_t
    {
        jan = 1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
    };

    //-----------------------------------------------------
    //Oracle连接参数
    typedef struct conn_param_t
    {
        String     DBHost;                               //数据库服务器所在地址
        uint32_t   DBPort;
        String     ServiceName;                          //数据库实例名
        String     UserName;                             //数据库用户名
        String     Password;                             //数据库口令
    }conn_param_t;

    //-----------------------------------------------------
    //记录错误信息的功能类
    class error_info_t
    {
        static const ub4 MAX_BUF_SIZE = 1024 * 2;
    private:
        error_class_t	    m_Type;		                    // type
        sword		        m_Code;			                // error code if library error or -1 if Oracle error
        sb4			        m_OraCode;		                // Oracle's error code - ORA-xxxxx
        char	            m_Description[MAX_BUF_SIZE];	// error description as a text
        char	            m_Source[MAX_BUF_SIZE];			// source file, where error was thrown (optional)
        char                m_RetInfo[MAX_BUF_SIZE];        //返回完整消息时使用的缓冲串
        char                m_BindHost[MAX_PATH];           //绑定过的主机名
        char                m_BindServiceName[MAX_PATH];    //绑定过的主机Oracle实例名字
        char                m_BindUserName[MAX_PATH];       //绑定过的用户名
        long		        m_LineNo;		                // line number, where error was thrown (optional)

        //--------------------------------------------------
        //得到Oracle的错误详细信息
        void make_oci_error_info(sword ora_err, OCIError *error_handle, OCIEnv *env_handle)
        {
            bool	get_details = false;
            rx::strcat_ct desc(m_Description, sizeof(m_Description));

            if (error_handle == NULL && env_handle == NULL)
            {
                desc = "(OCI_ENV_NULL)";
                return;
            }

            m_Code = ora_err;
            switch (ora_err)
            {
            case	OCI_SUCCESS:                desc = "(OCI_SUCCESS)"; break;
            case	OCI_SUCCESS_WITH_INFO:      desc = "(OCI_SUCCESS_WITH_INFO)"; get_details = true; break;
            case	OCI_ERROR:                  desc = "(OCI_ERROR)"; get_details = true; break;
            case	OCI_NO_DATA:                desc = "(OCI_NO_DATA)"; get_details = true; break;
            case	OCI_INVALID_HANDLE:         desc = "(OCI_INVALID_HANDLE)"; break;
            case	OCI_NEED_DATA:              desc = "(OCI_NEED_DATA)"; break;
            case	OCI_STILL_EXECUTING:        desc = "(OCI_STILL_EXECUTING)"; get_details = true; break;
            case	OCI_CONTINUE:               desc = "(OCI_CONTINUE)"; break;
            default:                            desc = "unknown";
            }

            // get detailed error m_Description
            if (get_details)
            {
                char Tmp[MAX_BUF_SIZE];
                if (error_handle)
                    OCIErrorGet(error_handle, 1, NULL, &m_OraCode, reinterpret_cast<text *> (Tmp), MAX_BUF_SIZE, OCI_HTYPE_ERROR);
                else
                    OCIErrorGet(env_handle, 1, NULL, &m_OraCode, reinterpret_cast<text *> (Tmp), MAX_BUF_SIZE, OCI_HTYPE_ENV);
                desc << ' ' << Tmp;
            }
        }

        //-------------------------------------------------
        //得到当前库内的详细错误信息
        void make_dbc_error(sword olib_err)
        {
            rx::strcat_ct desc(m_Description, sizeof(m_Description));
            m_Code = olib_err;
            desc = error_code_info(olib_err);
        }

        //-------------------------------------------------
        //将输入的可变参数对应的串连接到详细错误信息中
        void make_attached_msg(const char *format, va_list va)
        {
            rx_assert(!is_empty(format) && va);
            char Tmp[ERROR_FORMAT_MAX_MSG_LEN];
            vsnprintf(Tmp, ERROR_FORMAT_MAX_MSG_LEN - 1, format, va);
            rx::strcat_ct desc(m_Description, sizeof(m_Description), rx::st::strlen(m_Description));
            desc << ": " << Tmp;
        }
        //-------------------------------------------------
        //禁止拷贝赋值
        error_info_t& operator = (const error_info_t&);
    public:
        //-------------------------------------------------
        //构造函数,根据错误句柄得到Oracle的详细错误信息
        error_info_t(sword ora_err, OCIError *error_handle, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_oci_error_info(ora_err, error_handle, NULL);
            if (format)
            {
                va_list	va;
                va_start(va, format);
                make_attached_msg(format, va);
                va_end(va);
            }

            m_Type = ET_ORACLE;
            rx::st::strcpy(m_Source, sizeof(m_Source), source_name);
            m_LineNo = line_number;
            m_BindHost[0] = 0;
        }
        //-------------------------------------------------
        //构造函数,通过环境句柄得到Oracle的详细错误信息
        error_info_t(sword ora_err, OCIEnv *env_handle, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_oci_error_info(ora_err, NULL, env_handle);

            if (format)
            {
                va_list	va;
                va_start(va, format);
                make_attached_msg(format, va);
                va_end(va);
            }

            m_Type = ET_ORACLE;
            rx::st::strcpy(m_Source, sizeof(m_Source), source_name);
            m_LineNo = line_number;
            m_BindHost[0] = 0;
        }
        //-------------------------------------------------
        //构造函数,记录库内部错误
        error_info_t(sword olib_err, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            // sets-up code and m_Description
            make_dbc_error(olib_err);

            // concat user-specified details
            if (format)
            {
                va_list	va;
                va_start(va, format);
                make_attached_msg(format, va);
                va_end(va);
            }

            m_Type = ET_DBC;
            m_OraCode = 0;
            rx::st::strcpy(m_Source, sizeof(m_Source), source_name);
            m_LineNo = line_number;
            m_BindHost[0] = 0;
        }
        //-------------------------------------------------
        //绑定发生错误的数据库连接信息
        void bind(const char* Host, const char* ServiceName, const char* UserName)
        {
            rx::st::strcpy(m_BindHost, MAX_PATH, Host);
            rx::st::strcpy(m_BindServiceName, MAX_PATH, ServiceName);
            rx::st::strcpy(m_BindUserName, MAX_PATH, UserName);
        }
        //-------------------------------------------------
        //得到错误的详细信息
        const char* c_str(void)
        {
            rx::st::replace(m_Description, '\n', ' ');
            if (!m_BindHost[0])
                sprintf(m_RetInfo, "errClass[%s],errInfo{%s}", error_class_name(m_Type), m_Description);
            else
                sprintf(m_RetInfo, "errClass[%s],DBHost[%s],DBServiceName[%s],DBUser[%s],errInfo{%s}", error_class_name(m_Type), m_BindHost, m_BindServiceName, m_BindUserName, m_Description);
            return m_RetInfo;
        }
        //-------------------------------------------------
        //得到错误代码
        ub4 oci_error_code() { return m_OraCode; }
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
            year(ST.tm_year);
            mon(ST.tm_mon);
            day((ub1)ST.tm_mday);
            hour((ub1)ST.tm_hour);
            minute((ub1)ST.tm_min);
            sec((ub1)ST.tm_sec);
        }
        //-------------------------------------------------
        //访问各个分量
        sb2 year(void) const { return m_Date.OCIDateYYYY; }
        void year(sb2 yy) { m_Date.OCIDateYYYY = yy; }

        month_name_t mon(void) const { return month_name_t(m_Date.OCIDateMM); }
        void mon(month_name_t mm) { m_Date.OCIDateMM = (ub1)mm; }
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
    //统一功能函数:将指定的原始类型的数据转换为字符串:错误句柄;原始数据缓冲区;原始数据类型;临时字符串缓冲区;临时缓冲区尺寸;转换格式
    inline PStr CommAsString (OCIError *ErrHandle,ub1* DataBuf,data_type_t OlibDataType,char *TmpBuf,int TmpBufSize,const char* ConvFmt=NULL)
    {
        switch(OlibDataType)
        {
            case DT_TEXT:
                return (reinterpret_cast <PStr> (DataBuf));
            case DT_NUMBER:
            {
                ub4 FmtLen=3;
                if (ConvFmt==NULL) ConvFmt="TM9";
                else FmtLen= rx::st::strlen(ConvFmt);
                ub4 ValueSize=TmpBufSize;
                sword result = OCINumberToText (ErrHandle,reinterpret_cast <OCINumber *> (DataBuf),(oratext*)ConvFmt,FmtLen,NULL,0,&ValueSize,(oratext*)TmpBuf);
                if (result == OCI_SUCCESS)
                    return (TmpBuf);
                else
                    throw (rx_dbc_ora::error_info_t (result, ErrHandle, __FILE__, __LINE__));
            }
            case DT_DATE:
            {
                datetime_t DT(*reinterpret_cast <OCIDate *> (DataBuf));
                DT.to((char*)TmpBuf,ConvFmt);
                return TmpBuf;
            }
            default:
                throw (rx_dbc_ora::error_info_t (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
        }
    }
    //-----------------------------------------------------
    //根据字段类型与字段数据缓冲区以及当前行号,进行正确的数据偏移调整
    //入口:行数据数组;数据类型;当前行偏移;字段数据最大尺寸
    inline ub1* CommFieldRowDataOffset(ub1* DataArrayBuf,data_type_t OlibDataType,int RowNo,int MaxFieldSize)
    {
        switch(OlibDataType)
        {
            case DT_TEXT:
                return DataArrayBuf + RowNo*MaxFieldSize;
            case DT_NUMBER:
                return (ub1*)(reinterpret_cast <OCINumber *> (DataArrayBuf) + RowNo);
            case DT_DATE:
                return (ub1*)(reinterpret_cast <OCIDate *> (DataArrayBuf) + RowNo);
            default:
                throw (rx_dbc_ora::error_info_t (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
        }
    }
    //-----------------------------------------------------
    //统一功能函数:将指定的原始类型的数据转换为浮点数:错误句柄;原始数据缓冲区;原始数据类型;
    inline double CommAsDouble (OCIError *ErrHandle,ub1* DataBuf,data_type_t OlibDataType)
    {
        if (OlibDataType == DT_NUMBER)
        {
            double	value;
            sword result = ::OCINumberToReal (ErrHandle,reinterpret_cast <OCINumber *>(DataBuf),sizeof (double),&value);
            if (result == OCI_SUCCESS)
                return (value);
            else
                throw (rx_dbc_ora::error_info_t (result, ErrHandle, __FILE__, __LINE__));
        }
        else if (OlibDataType == DT_TEXT)
        {
            return rx::st::atof((char*)DataBuf);
        }
        else
            throw (rx_dbc_ora::error_info_t (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
    }

    //-----------------------------------------------------
    //统一功能函数:将指定的原始类型的数据转换为带符号整型数:错误句柄;原始数据缓冲区;原始数据类型;
    inline long CommAsLong (OCIError *ErrHandle,ub1* DataBuf,data_type_t OlibDataType)
    {
        if (OlibDataType == DT_NUMBER)
        {
            long	value;
            sword result = OCINumberToInt (ErrHandle,reinterpret_cast <OCINumber *> (DataBuf),sizeof (long),OCI_NUMBER_SIGNED,&value);
            if (result == OCI_SUCCESS)
                return (value);
            else
                throw (rx_dbc_ora::error_info_t (result, ErrHandle, __FILE__, __LINE__));
        }
        else if (OlibDataType == DT_TEXT)
        {
            return rx::st::atoi((char*)DataBuf);
        }
        else
            throw (rx_dbc_ora::error_info_t (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
    }

    //-----------------------------------------------------
    //统一功能函数:将指定的原始类型的数据转换为日期:错误句柄;原始数据缓冲区;原始数据类型;
    inline datetime_t CommAsDateTime (OCIError *ErrHandle,ub1* DataBuf,data_type_t OlibDataType)
    {
        if (OlibDataType == DT_DATE)
            return (datetime_t (*(reinterpret_cast <OCIDate *> (DataBuf))));
        else
            throw (rx_dbc_ora::error_info_t (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
    }
    
    //-----------------------------------------------------
    //在调试状态下,用外部的内存管理函数替代OCI内部的内存管理函数,方便外面进行资源泄露的诊断
    #if defined(RLIB_CC_DEBUG)
        inline dvoid *DBG_Malloc_Func (dvoid * ctxp, size_t size) { return (malloc (size)); }
        inline dvoid *DBG_Realloc_Func (dvoid * ctxp, dvoid *ptr, size_t size){ return (realloc (ptr, size)); }
        inline void   DBG_Free_Func (dvoid * ctxp, dvoid *ptr){ free (ptr); }
    #else
        #define	DBG_Malloc_Func		NULL
        #define	DBG_Realloc_Func	NULL
        #define	DBG_Free_Func		NULL
    #endif

}

#endif
