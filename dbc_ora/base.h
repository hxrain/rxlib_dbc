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

namespace ora
{
    //�ֶ�������󳤶�
    const ub2 FIELD_NAME_LENGTH = 32;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //OCI���ָ�ʽ:ȥ���ո�,�������x������λ,С������С����2λ�����14λ
    const char* NUMBER_FRM_FMT = "FM999999999999999999.00999999999999";
    const ub2 NUMBER_FRM_FMT_LEN = 35;

    inline sql_type_t sql_type_to(int OCI_STMT_TYPE)
    {
        switch(OCI_STMT_TYPE)
        {
        case OCI_STMT_SELECT: return ST_SELECT ;
        case OCI_STMT_UPDATE: return ST_UPDATE ;
        case OCI_STMT_DELETE: return ST_DELETE ;
        case OCI_STMT_INSERT: return ST_INSERT ;
        case OCI_STMT_CREATE: return ST_CREATE ;
        case OCI_STMT_DROP  : return ST_DROP   ;
        case OCI_STMT_ALTER : return ST_ALTER  ;
        case OCI_STMT_BEGIN : return ST_BEGIN  ;
        case OCI_STMT_DECLARE:return ST_DECLARE;
        default:              return ST_UNKNOWN;
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

    //-------------------------------------------------
    //����OCI����ֵ,��ȡ����ϸ�Ĵ�����Ϣ
    inline bool get_last_error(sword oci_result, sword &ec, char *buff, ub4 max_size, OCIError *handle_err, OCIEnv *handle_env = NULL)
    {
        bool get_details = false;
        rx::tiny_string_t<> desc(max_size, buff);

        if (handle_err == NULL && handle_env == NULL)
        {
            desc = "(OCI_ENV_NULL)";
            return false;
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
            char Tmp[512];
            if (handle_err)
                OCIErrorGet(handle_err, 1, NULL, &ec, reinterpret_cast<text *> (Tmp), sizeof(Tmp), OCI_HTYPE_ERROR);
            else
                OCIErrorGet(handle_env, 1, NULL, &ec, reinterpret_cast<text *> (Tmp), sizeof(Tmp), OCI_HTYPE_ENV);
            desc << '<' << Tmp << '>';
        }
        return true;
    }

    //-----------------------------------------------------
    //��¼������Ϣ�Ĺ�����
    class error_info_t
    {
        static const ub4 MAX_BUF_SIZE = 1024 * 2;
    private:
        sword		        m_dbc_ec;  	                    //DBC������
        sb4			        m_ora_ec;		                //OCI������,ORA-xxxxx
        char	            m_err_desc[MAX_BUF_SIZE];	    //��������

        //--------------------------------------------------
        //�õ�Oracle�Ĵ�����ϸ��Ϣ
        void make_oci_error_info(sword ec, const char *msg)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "OCI::" << msg;
            m_dbc_ec = DBEC_DB;
            m_ora_ec = ec;
            desc.repleace('\n','.');
            //����OCI����ϸ��,ӳ�䵽DBEC������
            switch (m_ora_ec)
            {
                case 1      :m_dbc_ec = DBEC_DB_UNIQUECONST; break;
                case 1017   :m_dbc_ec = DBEC_DB_BADPWD; break;
                case 12170  :m_dbc_ec = DBEC_DB_CONNTIMEOUT; break;
                case 28002  :m_dbc_ec = DBEC_DB_PWD_WILLEXPIRE; break;
                default:
                    if (is_connection_lost())
                        m_dbc_ec = DBEC_DB_CONNLOST;
                    else if (is_connect_fail())
                        m_dbc_ec = DBEC_DB_CONNFAIL;
                    break;
            }
        }

        //-------------------------------------------------
        //�õ���ǰ���ڵ���ϸ������Ϣ
        void make_dbc_error(err_type_t dbc_err)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "DBC::" << err_type_str(dbc_err);
            m_dbc_ec = dbc_err;
            m_ora_ec = 0;
        }

        //-------------------------------------------------
        //������Ŀɱ������Ӧ�Ĵ����ӵ���ϸ������Ϣ��
        void make_attached_msg(const char *format, va_list va, const char *source_name = NULL, uint32_t line_number = -1)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc, rx::st::strlen(m_err_desc));
            if (!is_empty(format))
            {
                desc << " @ < ";
                desc(format,va) << " >";
            }

            if (!is_empty(source_name))
                desc << " # (" << source_name << ':' << rx::n2s_t(line_number) << ')';
        }
        //-------------------------------------------------
        //��ֹ������ֵ
        error_info_t& operator = (const error_info_t&);
    public:
        //-------------------------------------------------
        //���캯��,ͨ����������õ�Oracle����ϸ������Ϣ
        error_info_t(sword oci_rc, OCIError *handle_err, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            sword oci_ec;
            char msg[1024];
            get_last_error(oci_rc, oci_ec, msg, sizeof(msg), handle_err);
            make_oci_error_info(oci_ec,msg);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //���캯��,��¼���ڲ�����
        error_info_t(err_type_t dbc_err, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            make_dbc_error(dbc_err);

            va_list	va;
            va_start(va, format);
            make_attached_msg(format, va, source_name, line_number);
            va_end(va);
        }
        //-------------------------------------------------
        //�󶨷�����������ݿ�������Ϣ���ٻ�ȡ�����Ĵ������
        const char* c_str(const conn_param_t &cp)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc, rx::st::strlen(m_err_desc));
            desc << "::host[" << cp.host << "]db[" << cp.db << "]user[" << cp.user << ']';
            return m_err_desc;
        }
        //-------------------------------------------------
        //�õ��������ϸ��Ϣ
        const char* c_str(void) { return m_err_desc; }
        //-------------------------------------------------
        //�ж��Ƿ�ΪOCI�������
        bool is_db_error() { return m_ora_ec != 0; }
        //�õ�oci�������
        ub4 db_error_code() { return m_ora_ec; }
        //�õ�dbc�������
        ub4 dbc_error_code() { return m_dbc_ec; }
        //-------------------------------------------------
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
        //-------------------------------------------------
        //�Ƿ�ΪΨһ��Լ����ͻ
        bool is_unique_constraint() { return m_ora_ec == 1; }
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
        data_type_t	    m_dbc_data_type;		        // �ڴ�����������
        int				    m_max_data_size;			    // ÿ���ֶ��������ߴ�

        static const int    m_working_buff_size = 64;       // ��ʱ���ת���ַ����Ļ�����
        char                m_working_buff[m_working_buff_size];

        array_datasize_t    m_col_datasize;		            // �ı��ֶ�ÿ�еĳ�������
        array_databuff_t    m_col_databuff;	                // ��¼���ֶε�ÿ�е�ʵ�����ݵ�����
        array_dataempty_t   m_col_dataempty;	            // ��Ǹ��ֶε�ֵ�Ƿ�Ϊ�յ�״̬����: 0 - ok; -1 - null

        //-------------------------------------------------
        //�ֶι��캯��,ֻ�ܱ���¼����ʹ��
        void make(const char *name, ub4 name_len, data_type_t dbc_data_type, ub4 max_data_size, int bulk_row_count, bool make_datasize_array = false)
        {
            rx_assert(!is_empty(name));
            reset();

            m_name.assign(name, name_len);
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
            case DT_INT:
            case DT_UINT:
            case DT_LONG:
            case DT_FLOAT:
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
            case DT_INT:
            case DT_UINT:
            case DT_LONG:
            case DT_FLOAT:
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
            case DT_INT:
            case DT_UINT:
            case DT_LONG:
            case DT_FLOAT:
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
        template<typename DT>
        DT comm_as_int(ub1* data_buff,bool is_signed = true) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
                if (sizeof(DT)==4)
                    return is_signed?rx::st::atoi((char*)data_buff):rx::st::atoul((char*)data_buff);
                else
                    return (DT)rx::st::atoi64((char*)data_buff);
            case DT_INT:
            case DT_UINT:
            case DT_LONG:
            case DT_FLOAT:
            {
                DT	value;
                sword result = OCINumberToInt(oci_err_handle(), reinterpret_cast <OCINumber *> (data_buff), sizeof(value), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED, &value);
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
        col_base_t& to(char* buff, uint32_t max_size = 0)
        {
            if (max_size)
                rx::st::strcpy(buff, max_size, as_string());
            else
                rx::st::strcpy(buff, as_string());
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ������
        double as_double(double DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<double>(get_data_buff(idx));
        }
        col_base_t& to(double &buff, double def_val = 0)
        {
            buff = as_double(def_val);
            return *this;
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
        int64_t as_long(int64_t DefValue = 0) const 
        { 
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_int<int64_t>(get_data_buff(idx));
        }
        col_base_t& to(int64_t &buff, int64_t def_val = 0)
        {
            buff = as_long(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����������
        int32_t as_int(int32_t DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_int<int32_t>(get_data_buff(idx));
        }
        col_base_t& to(int32_t &buff, int32_t def_val = 0)
        {
            buff = as_int(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�޷�������
        uint32_t as_uint(uint32_t DefValue = 0) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_int<uint32_t>(get_data_buff(idx), false);
        }
        col_base_t& to(uint32_t &buff, uint32_t def_val = 0)
        {
            buff = as_uint(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����ʱ��
        datetime_t as_datetime(void) const
        {
            ub2 idx = bulk_row_idx();
            if (m_is_null(idx)) return datetime_t();
            return comm_as_datetime(get_data_buff(idx));
        }
        col_base_t& to(datetime_t &buff)
        {
            buff = as_datetime();
            return *this;
        }
    };
}

#endif
