#ifndef _RX_DBC_MYSQL_COMM_H_
#define _RX_DBC_MYSQL_COMM_H_

namespace rx_dbc_mysql
{
    //����ʹ�õ��ַ�������󳤶�
    const uint16_t MAX_TEXT_BYTES = 1024 * 2;

    //ÿ������FEATCH��ȡ�Ľ����������
    const uint16_t BAT_FETCH_SIZE = 20;

    //�ֶ�������󳤶�
    const uint16_t FIELD_NAME_LENGTH = 64;

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
        ST_SET = 9
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
        {//���Ļ���,�ַ����п�����ֱ�ӵ�gbk�ַ���
            charset = "gbk";
            language = "en_US";
        }
        void use_english()
        {//Ӣ�Ļ���,���Ҫ��������,����Ҫ����gbk2utf8��ת��
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
            port = 3306;
            conn_timeout = 3;
        }
    }conn_param_t;

    //-------------------------------------------------
    //��ȡdb����ϸ�Ĵ�����Ϣ
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
    //��ȡDB/STMT����ϸ�Ĵ�����Ϣ
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
    //��¼������Ϣ�Ĺ�����
    class error_info_t
    {
        static const uint32_t MAX_BUF_SIZE = 1024 * 2;
    private:
        int32_t		        m_dbc_ec;  	                    //DBC������
        int32_t		        m_mysql_ec;		                //mysql������
        char	            m_err_desc[MAX_BUF_SIZE];	    //��������

        //--------------------------------------------------
        //�õ�dn�Ĵ�����ϸ��Ϣ
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
        //���캯��,ͨ����������õ�db����ϸ������Ϣ
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
        //���캯��,ͨ����������õ�db����ϸ������Ϣ
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

    //-------------------------------------------------
    //��ȡ�������
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
    //����db��ʽ������ʱ��Ĺ�����
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

    //�������Զ���ʹ�õ���
    class conn_t;
    class stmt_t;
    class query_t;
    class sql_param_t;
    class field_t;
    //-------------------------------------------------
    //���������ռ��еĶ��⿪�����ͽ���ͳһ����
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
