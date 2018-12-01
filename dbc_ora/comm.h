#ifndef _RX_DBC_ORA_COMM_H_
#define _RX_DBC_ORA_COMM_H_

    //-----------------------------------------------------
    //�ڵ���״̬��,���ⲿ���ڴ���������OCI�ڲ����ڴ������,�������������Դй¶�����
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
    //����ʹ�õ��ַ�������󳤶�
    const ub2 MAX_TEXT_BYTES = 1024 * 2;

    //ÿ������FEATCH��ȡ�Ľ����������
    const ub2 BAT_FETCH_SIZE = 20;

    //�ֶ�������󳤶�
    const ub2 FIELD_NAME_LENGTH = 30;

    //sql���ĳ�������
    const int MAX_SQL_LENGTH = 1024 * 8;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //OCI���ָ�ʽ:ȥ���ո�,�������x������λ,С������С����2λ�����14λ
    const char* NUMBER_FRM_FMT = "FM999999999999999999.00999999999999";
    const ub2 NUMBER_FRM_FMT_LEN = 35;
    //-----------------------------------------------------
    //dbc_ora���Դ������������(�󶨲���ʱ,����ǰ׺���Ը�֪��������)
    enum data_type_t
    {
        DT_UNKNOWN,
        DT_NUMBER   = 'n',
        DT_DATE     = 'd',
        DT_TEXT     = 's'
    };

    //-----------------------------------------------------
    //sql�������
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
    //�������
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
    //DBC��װ����������
    enum dbc_error_code_t
    {
        DBEC_ENV_FAIL = 1000,                               //OCI������������
        DBEC_NO_MEMORY,                                     //�ڴ治��
        DBEC_NO_BUFFER,                                     //����������
        DBEC_IDX_OVERSTEP,                                  //�±�Խ��
        DBEC_BAD_PARAM,                                     //��������
        DBEC_BAD_INPUT,                                     //���󶨲������������ʹ���
        DBEC_BAD_OUTPUT,                                    //��֧�ֵ������������
        DBEC_BAD_TYPEPREFIX,                                //�����Զ���ʱ,����ǰ׺��׼ȷ
        DBEC_UNSUP_TYPE,                                    //δ֧�ֵ���������
        DBEC_PARAM_NOT_FOUND,                               //���ʵĲ������󲻴���
        DBEC_FIELD_NOT_FOUND,                               //���ʵ��ж��󲻴���
        DBEC_METHOD_CALL,                                   //�������õ�˳�����
        DBEC_NOT_PARAM,                                     //sql�����û��':'ǰ׺�Ĳ���,�����԰󶨲���
        DBEC_PARSE_PARAM,                                   //sql����Զ�������������
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
    //OCI���ӵĻ���ѡ��
    typedef struct env_option_t
    {
        ub2   charset_id;
        const char *charset;
        const char *language;
        const char *date_format;

        env_option_t() { use_english(); }
        void use_chinese()
        {//���Ļ���
            charset_id = 852;
            charset = "zhs16gbk";
            language = "SIMPLIFIED CHINESE";
            date_format = "yyyy-mm-dd hh24:mi:ss";
        }
        void use_english()
        {//Ӣ�Ļ���
            charset_id = 871;
            charset = "UTF8";
            language = "AMERICAN";
            date_format = "yyyy-mm-dd hh24:mi:ss";
        }
    }env_option_t;

    //-----------------------------------------------------
    //Oracle���Ӳ���
    typedef struct conn_param_t
    {
        char        host[64];                               //���ݿ���������ڵ�ַ
        char        user[64];                               //���ݿ��û���
        char        pwd[64];                                //���ݿ����
        char        db[64];                                 //���ݿ�ʵ����
        uint32_t    port;                                   //���ݿ�˿�
        uint32_t    conn_timeout;                           //���ӳ�ʱʱ��
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
    //��¼������Ϣ�Ĺ�����
    class error_info_t
    {
        static const ub4 MAX_BUF_SIZE = 1024 * 2;
    private:
        error_class_t	    m_err_type;		                //error type
        sword		        m_dbc_ec;  	                    //DBC������
        sb4			        m_ora_ec;		                //Oracle������,ORA-xxxxx
        char	            m_err_desc[MAX_BUF_SIZE];	    //��������
        char                m_out_buff[MAX_BUF_SIZE];       //����������Ϣʱʹ�õĻ��崮

        char                m_bind_host[64];                //�󶨹���������
        char                m_bind_sid[64];                 //�󶨹�������Oracleʵ������
        char                m_bind_user[64];                //�󶨹����û���
        char	            m_src_file[256];	            // source file, where error was thrown (optional)
        long		        m_src_file_lineno;		        // line number, where error was thrown (optional)

        //--------------------------------------------------
        //�õ�Oracle�Ĵ�����ϸ��Ϣ
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
        //�õ���ǰ���ڵ���ϸ������Ϣ
        void make_dbc_error(sword dbc_err)
        {
            rx::tiny_string_ct desc(sizeof(m_err_desc), m_err_desc);
            m_err_type = ET_DBC;
            m_dbc_ec = dbc_err;
            m_ora_ec = 0;
            desc = dbc_error_code_info(dbc_err);
        }

        //-------------------------------------------------
        //������Ŀɱ������Ӧ�Ĵ����ӵ���ϸ������Ϣ��
        void make_attached_msg(const char *format, va_list va, const char *source_name = NULL, long line_number = -1)
        {
            m_out_buff[0] = 0;
            if (format)
            {
                //��ʱʹ�ô�Ŵ�����Ϣ�Ļ������ߴ�
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
        //��ֹ������ֵ
        error_info_t& operator = (const error_info_t&);
    public:
        //-------------------------------------------------
        //���캯��,���ݴ������õ�Oracle����ϸ������Ϣ
        error_info_t(sword oci_result, OCIError *error_handle, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_oci_error_info(oci_result, error_handle, NULL);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va,source_name,line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //���캯��,ͨ����������õ�Oracle����ϸ������Ϣ
        error_info_t(sword oci_result, OCIEnv *env_handle, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_oci_error_info(oci_result, NULL, env_handle);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //���캯��,��¼���ڲ�����
        error_info_t(sword dbc_err, const char *source_name = NULL, long line_number = -1, const char *format = NULL, ...)
        {
            make_dbc_error(dbc_err);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //�󶨷�����������ݿ�������Ϣ���ٻ�ȡ�����Ĵ������
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
        //�õ��������ϸ��Ϣ
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
        //�ж��Ƿ�ΪOCI�������
        bool is_oci_error() { return m_err_type == ET_ORACLE; }
        //�õ�oci�������
        ub4 oci_error_code() { return m_ora_ec; }
        //�жϴ����Ƿ�Ϊ�û����������
        bool is_bad_user_pwd() { return m_ora_ec == 1017; }
        //�ж��Ƿ�Ϊ���Ӵ���
        bool is_conn_timeout() { return m_ora_ec == 12170; }
        //���뼴������
        bool is_pwd_will_expire() { return m_ora_ec == 28002; }
        //���ӶϿ�
        bool is_connection_lost() { return m_ora_ec == 3135|| m_ora_ec == 3114 || m_ora_ec == 3113; }
        //����ʧ��(12541������������;��������;�����ӳ�ʱ;�����Ӷ�ʧ)
        bool is_connect_fail() { return m_ora_ec == 12541 || is_bad_user_pwd() || is_conn_timeout() || is_connection_lost(); }
    };

    //-----------------------------------------------------
    //����Oracle��ʽ������ʱ��Ĺ�����
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
        //���ø�������
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
        //���ʸ�������
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
        //���ݸ����ĸ�ʽ,���ڲ�����ת��Ϊ�ַ�����ʽ
        char* to(char *Result, const char* Format = NULL) const
        {
            if (Format == NULL) Format = "%d-%02d-%02d %02d:%02d:%02d";
            sprintf(Result, Format, m_Date.OCIDateYYYY, m_Date.OCIDateMM, m_Date.OCIDateDD, m_Date.OCIDateTime.OCITimeHH, m_Date.OCIDateTime.OCITimeMI, m_Date.OCIDateTime.OCITimeSS);
            return Result;
        }
    };

    //-----------------------------------------------------
    //�ֶ���󶨲���ʹ�õĻ���,�������ݻ������Ĺ������ֶ����ݵķ���
    class col_base_t
    {
    protected:
        typedef rx::buff_t array_datasize_t;                //ub2
        typedef rx::buff_t array_databuff_t;                //ub1
        typedef rx::buff_t array_dataempty_t;               //sb2
        typedef rx::tiny_string_t<char, FIELD_NAME_LENGTH> col_name_t;

        col_name_t          m_name;		                    // ��������
        data_type_t	        m_dbc_data_type;		        // �ڴ�����������
        ub2				    m_oci_data_type;		        // Oracleʵ����������,���������Զ�ת��
        int				    m_max_data_size;			    // ÿ���ֶ��������ߴ�

        static const int    m_working_buff_size = 64;       // ��ʱ���ת���ַ����Ļ�����
        char                m_working_buff[m_working_buff_size];

        array_datasize_t    m_col_datasize;		            // �ı��ֶ�û�еĳ�������
        array_databuff_t    m_col_databuff;	                // ��¼���ֶε�ÿ�е�ʵ�����ݵ�����
        array_dataempty_t   m_col_dataempty;	            // ��Ǹ��ֶε�ֵ�Ƿ�Ϊ�յ�״̬����: 0 - ok; -1 - null

        //-------------------------------------------------
        //�ֶι��캯��,ֻ�ܱ���¼����ʹ��
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

            //�����ֶ����ݻ�����
            m_col_databuff.make(m_max_data_size * bulk_row_count);
            //�����ֶ��Ƿ�Ϊ�յ�����
            m_col_dataempty.make<sb2>(bulk_row_count);
            if (!m_col_dataempty.capacity() || !m_col_databuff.capacity())
            {//�ж��Ƿ��ڴ治��
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
        //�����ֶ��������ֶ����ݻ������Լ���ǰ�к�,������ȷ������ƫ�Ƶ���
        //���:����������;��������;��ǰ��ƫ��;�ֶ��������ߴ�
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ�ַ���:������;ԭʼ���ݻ�����;ԭʼ��������;��ʱ�ַ���������;��ʱ�������ߴ�;ת����ʽ
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������:������;ԭʼ���ݻ�����;ԭʼ��������;
        template<class DT>
        DT comm_as_double(ub1* data_buff) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
            {//�ı�ת��Ϊdoubleֵ
                //����oci�ķ���,�Ȱ��ı���ת��ΪOCI��number
                OCINumber num;
                sword result = ::OCINumberFromText(oci_err_handle(), (const oratext*)data_buff, rx::st::strlen((char*)data_buff), (const oratext*)NUMBER_FRM_FMT, NUMBER_FRM_FMT_LEN, (const oratext*)"NLS_NUMERIC_CHARACTERS='.''", 27, &num);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__, "col(%s)", m_name.c_str()));

                //�ٽ�OCI��numberת��Ϊԭ��double
                DT	value;
                result = ::OCINumberToReal(oci_err_handle(), &num, sizeof(DT), &value);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, oci_err_handle(), __FILE__, __LINE__, "col(%s)", m_name.c_str()));

                return value;
            }
            case DT_NUMBER:
            {//OCI����ת��Ϊdouble
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������������:������;ԭʼ���ݻ�����;ԭʼ��������;
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ����:������;ԭʼ���ݻ�����;ԭʼ��������;
        datetime_t comm_as_datetime(ub1* data_buff) const
        {
            if (m_dbc_data_type == DT_DATE)
                return (datetime_t(*(reinterpret_cast <OCIDate *> (data_buff))));
            else
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
        }
        //-------------------------------------------------
        //����ֱ��ʹ�ñ�������,���뱻�̳�
        col_base_t(rx::mem_allotter_i &ma) :m_col_datasize(ma), m_col_databuff(ma), m_col_dataempty(ma) { reset(); }
        virtual ~col_base_t() { reset(); }
        bool m_is_null(ub2 idx) const { return (m_col_dataempty.at<sb2>(idx) == -1); }
    protected:
        //-------------------------------------------------
        //������Ҫ����ʵ�ֵľ��幦�ܺ����ӿ�
        //-------------------------------------------------
        //�õ�������
        virtual OCIError* oci_err_handle() const = 0;
        //�õ���ǰ�ķ����к�
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
        //���Ի�ȡ�ڲ�����Ϊ�ַ���
        PStr as_string(const char* ConvFmt = NULL) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return NULL;
            return comm_as_string(get_data_buff(idx), ConvFmt);
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ������
        double as_double(double DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<double>(get_data_buff(idx));
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�߾��ȸ�����
        long double as_real(long double DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<long double>(get_data_buff(idx));
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ��������
        int64_t as_bigint(int64_t DefValue = 0) const { return int64_t(as_real((long double)DefValue)); }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����������
        int32_t as_long(int32_t DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx));
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�޷�������
        uint32_t as_ulong(uint32_t DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx), false);
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����ʱ��
        datetime_t as_datetime(void) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return datetime_t();
            return comm_as_datetime(get_data_buff(idx));
        }
    };
}

#endif
