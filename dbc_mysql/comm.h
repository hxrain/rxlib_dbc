#ifndef _RX_DBC_MYSQL_COMM_H_
#define _RX_DBC_MYSQL_COMM_H_

namespace rx_dbc_mysql
{
    //����ʹ�õ��ַ�������󳤶�
    const uint16_t MAX_TEXT_BYTES = 1024 * 2;

    //ÿ������FEATCH��ȡ�Ľ����������
    const uint16_t BAT_FETCH_SIZE = 20;

    //�ֶ�������󳤶�
    const uint16_t FIELD_NAME_LENGTH = 32;

    //sql���ĳ�������
    const int MAX_SQL_LENGTH = 1024 * 4;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //-----------------------------------------------------
    //dbc_ora���Դ������������(�󶨲���ʱ,����ǰ׺���Ը�֪��������)
    enum data_type_t
    {
        DT_UNKNOWN,
        DT_NUMBER   = 'n',                                  //�������͵��ֶλ����
        DT_DATE     = 'd',                                  //��������
        DT_TEXT     = 's'                                   //�ı�������
    };

    //-----------------------------------------------------
    //sql�������
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
    //DBC��װ����������
    enum dbc_error_code_t
    {
        DBEC_OK=0,
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

        DBEC_DB,                                            //DB����
        DBEC_DB_BADPWD,                                     //DB����ϸ��:�˺ſ������
        DBEC_DB_PWD_WILLEXPIRE,                             //DB����ϸ��:���������,������������Ӧ�ý��и澯
        DBEC_DB_CONNTIMEOUT,                                //DB����ϸ��:���ӳ�ʱ
        DBEC_DB_CONNLOST,                                   //DB����ϸ��:�Ѿ����������ӶϿ���.
        DBEC_DB_CONNFAIL,                                   //DB����ϸ��:����ʧ��,�޷���������
        DBEC_DB_UNIQUECONST,                                //DB����ϸ��:ΨһԼ�����µĴ���
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
    //����ѡ��
    typedef struct env_option_t
    {
        const char *charset;
        const char *language;

        env_option_t() { use_english(); }
        void use_chinese()
        {//���Ļ���
            charset = "gbk";
            language = "en_US";
        }
        void use_english()
        {//Ӣ�Ļ���
            charset = "utf8mb4";
            language = "en_US";
        }
    }env_option_t;

    //-----------------------------------------------------
    //���Ӳ���
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

    //-------------------------------------------------
    //����OCI����ֵ,��ȡ����ϸ�Ĵ�����Ϣ
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
    //��¼������Ϣ�Ĺ�����
    class error_info_t
    {
        static const uint32_t MAX_BUF_SIZE = 1024 * 2;
    private:
        int32_t		        m_dbc_ec;  	                    //DBC������
        int32_t		        m_mysql_ec;		                //mysql������
        char	            m_err_desc[MAX_BUF_SIZE];	    //��������

        //--------------------------------------------------
        //�õ�Oracle�Ĵ�����ϸ��Ϣ
        void make_db_error_info(int32_t ec, const char *msg)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "MYSQL::" << msg;
            m_dbc_ec = DBEC_DB;
            m_mysql_ec = ec;
            desc.repleace('\n', '.');
            //����OCI����ϸ��,ӳ�䵽DBEC������
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
        //�õ���ǰ���ڵ���ϸ������Ϣ
        void make_dbc_error(int32_t dbc_err)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "DBC::" << dbc_error_code_info(dbc_err);
            m_dbc_ec = dbc_err;
            m_mysql_ec = 0;
        }

        //-------------------------------------------------
        //������Ŀɱ������Ӧ�Ĵ����ӵ���ϸ������Ϣ��
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
        //��ֹ������ֵ
        error_info_t& operator = (const error_info_t&);
    public:
        //-------------------------------------------------
        //���캯��,ͨ����������õ�Oracle����ϸ������Ϣ
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
        //���캯��,��¼���ڲ�����
        error_info_t(int32_t dbc_err, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
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
        //�ж��Ƿ�Ϊdb�������
        bool is_db_error() { return m_mysql_ec != 0; }
        //�õ�db�������
        uint32_t db_error_code() { return m_mysql_ec; }
        //�õ�dbc�������
        uint32_t dbc_error_code() { return m_dbc_ec; }
        //-------------------------------------------------
        //�жϴ����Ƿ�Ϊ�û����������
        bool is_bad_user_pwd() { return m_mysql_ec == ER_ACCESS_DENIED_ERROR; }
        //�ж��Ƿ�Ϊ���Ӵ���
        bool is_conn_timeout() { return m_mysql_ec == CR_SERVER_LOST; }
        //���ӶϿ�
        bool is_connection_lost() { return m_mysql_ec == CR_SERVER_LOST; }
        //����ʧ��(12541������������;��������;�����ӳ�ʱ;�����Ӷ�ʧ)
        bool is_connect_fail() { return m_mysql_ec == CR_CONN_HOST_ERROR || m_mysql_ec == CR_UNKNOWN_HOST || m_mysql_ec == ER_BAD_DB_ERROR || is_bad_user_pwd() || is_conn_timeout() || is_connection_lost(); }
        //-------------------------------------------------
        //�Ƿ�ΪΨһ��Լ����ͻ
        bool is_unique_constraint() { return m_mysql_ec == ER_DUP_ENTRY; }
    };


    //-----------------------------------------------------
    //����Oracle��ʽ������ʱ��Ĺ�����
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
        //���ø�������
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
        //���ʸ�������
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
        //���ݸ����ĸ�ʽ,���ڲ�����ת��Ϊ�ַ�����ʽ
        char* to(char *Result, const char* Format = NULL) const
        {
            if (Format == NULL) Format = "%d-%02d-%02d %02d:%02d:%02d";
            sprintf(Result, Format, m_Date.year, m_Date.month, m_Date.day, m_Date.hour, m_Date.minute, m_Date.second);
            return Result;
        }
    };

    //-----------------------------------------------------
    //�ֶ���󶨲���ʹ�õĻ���,�������ݻ������Ĺ������ֶ����ݵķ���
    class col_base_t
    {
    protected:
        typedef rx::buff_t array_datasize_t;                //uint16_t
        typedef rx::buff_t array_databuff_t;                //uint8_t
        typedef rx::buff_t array_dataempty_t;               //int16_t
        typedef rx::tiny_string_t<char, FIELD_NAME_LENGTH> col_name_t;

        col_name_t          m_name;		                    // ��������
        data_type_t	        m_dbc_data_type;		        // �ڴ�����������
        int				    m_max_data_size;			    // ÿ���ֶ��������ߴ�

        static const int    m_working_buff_size = 64;       // ��ʱ���ת���ַ����Ļ�����
        char                m_working_buff[m_working_buff_size];

        array_datasize_t    m_col_datasize;		            // �ı��ֶ�ÿ�еĳ�������
        array_databuff_t    m_col_databuff;	                // ��¼���ֶε�ÿ�е�ʵ�����ݵ�����
        array_dataempty_t   m_col_dataempty;	            // ��Ǹ��ֶε�ֵ�Ƿ�Ϊ�յ�״̬����: 0 - ok; -1 - null

        //-------------------------------------------------
        //�ֶι��캯��,ֻ�ܱ���¼����ʹ��
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

            //�����ֶ����ݻ�����
            m_col_databuff.make(m_max_data_size * bulk_row_count);
            //�����ֶ��Ƿ�Ϊ�յ�����
            m_col_dataempty.make<int16_t>(bulk_row_count);
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ�ַ���:������;ԭʼ���ݻ�����;ԭʼ��������;��ʱ�ַ���������;��ʱ�������ߴ�;ת����ʽ
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������:������;ԭʼ���ݻ�����;ԭʼ��������;
        template<class DT>
        DT comm_as_double(uint8_t* data_buff) const
        {
            switch (m_dbc_data_type)
            {
            case DT_TEXT:
            {//�ı�ת��Ϊdoubleֵ
            }
            case DT_NUMBER:
            {//OCI����ת��Ϊdouble
            }
            default:
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������������:������;ԭʼ���ݻ�����;ԭʼ��������;
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
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ����:������;ԭʼ���ݻ�����;ԭʼ��������;
        datetime_t comm_as_datetime(uint8_t* data_buff) const
        {
                throw (error_info_t(DBEC_BAD_OUTPUT, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
        }
        //-------------------------------------------------
        //����ֱ��ʹ�ñ�������,���뱻�̳�
        col_base_t(rx::mem_allotter_i &ma) :m_col_datasize(ma), m_col_databuff(ma), m_col_dataempty(ma) { reset(); }
        virtual ~col_base_t() { reset(); }
        bool m_is_null(uint16_t idx) const { return (m_col_dataempty.at<int16_t>(idx) == -1); }
    protected:
        //-------------------------------------------------
        //������Ҫ����ʵ�ֵľ��幦�ܺ����ӿ�
        //-------------------------------------------------
        //�õ�������
        virtual void* oci_err_handle() const = 0;
        //�õ���ǰ�ķ����к�
        virtual uint16_t bulk_row_idx() const = 0;
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
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return NULL;
            return comm_as_string(get_data_buff(idx), ConvFmt);
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ������
        double as_double(double DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_double<double>(get_data_buff(idx));
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�߾��ȸ�����
        long double as_real(long double DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
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
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx));
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�޷�������
        uint32_t as_ulong(uint32_t DefValue = 0) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return DefValue;
            return comm_as_long(get_data_buff(idx), false);
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����ʱ��
        datetime_t as_datetime(void) const
        {
            uint16_t idx = bulk_row_idx();
            if (m_is_null(idx)) return datetime_t();
            return comm_as_datetime(get_data_buff(idx));
        }
    };
}

#endif
