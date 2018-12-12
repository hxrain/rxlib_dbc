#ifndef _RX_DBC_PGSQL_COMM_H_
#define _RX_DBC_PGSQL_COMM_H_

namespace pgsql
{
    //�ֶ�������󳤶�
    const uint16_t FIELD_NAME_LENGTH = 64;
    //dummy
    static const uint16_t BAT_BULKS_SIZE = 1;
    //-----------------------------------------------------

    //-----------------------------------------------------
    //����ѡ��
    typedef struct env_option_t
    {
        const char *charset;
        const char *language;

        env_option_t() { use_english(); }
        void use_chinese()
        {//���Ļ���,�ַ����п�����ֱ�ӵ�gbk�ַ���
            charset = "GBK";
            language = "en_US.UTF-8";
        }
        void use_english()
        {//Ӣ�Ļ���,���Ҫ��������,����Ҫ����gbk2utf8��ת��
            charset = "UTF8";
            language = "en_US.UTF-8";
        }
    }env_option_t;

    //--------------------------------------------------
    //����PGû��ͳһ�Ĵ������,������Ҫ�Թؼ���Ϣ�������,������Ҫ�Ĵ������
    inline int conv_pg_err_code(const char* msg)
    {
        if (is_empty(msg))
            return DBEC_OK;
        if (rx::st::strncmp(msg, "FATAL:  password authentication failed for user", 47) == 0)
            return DBEC_DB_BADPWD;
        if (rx::st::strstr(msg, "FATAL:  database \"") && rx::st::strstr(msg, "\" does not exist"))
            return DBEC_DB_CONNFAIL;
        if (rx::st::strncmp(msg, "ERROR:  duplicate key value violates unique constraint", 54) == 0)
            return DBEC_DB_UNIQUECONST;
            
        return DBEC_OTHER;
    }
    //-------------------------------------------------
    //��ȡdb���ӵĴ�����Ϣ
    inline bool get_last_error(int32_t &ec, char *buff, uint32_t max_size,PGconn *handle)
    {
        bool get_details = false;
        rx::tiny_string_t<> desc(max_size, buff);
        
        if (handle)
        {
            const char *err = ::PQerrorMessage(handle);
            ec = conv_pg_err_code(err);
            desc << "(DB_ERROR)<" << rx::n2s_t((uint32_t)ec) << ':';
            desc << err << '>';
        }
        else
        {
            ec = 0;
            desc = "(UNKNOWN_ERROR)";
        }
        return true;
    }
    //��ȡDB/STMT����ϸ�Ĵ�����Ϣ
    inline bool get_last_error(int32_t &ec, const char* msg,char *buff, uint32_t max_size)
    {
        bool get_details = false;
        rx::tiny_string_t<> desc(max_size, buff);

        if (!is_empty(msg))
        {
            ec = conv_pg_err_code(msg);
            desc << "(DB_ERROR)<" << rx::n2s_t((uint32_t)ec) << ':';
            desc << msg << '>';
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
        int32_t		        m_pg_ec;		                //pgsql������
        char	            m_err_desc[MAX_BUF_SIZE];	    //��������

        //--------------------------------------------------
        //�õ�dn�Ĵ�����ϸ��Ϣ
        void make_db_error_info(int32_t ec, const char *msg)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "pgsql::" << msg;
            m_dbc_ec = DBEC_DB;
            m_pg_ec = ec;
            desc.repleace('\n', '.');
        }

        //-------------------------------------------------
        //�õ���ǰ���ڵ���ϸ������Ϣ
        void make_dbc_error(err_type_t dbc_err)
        {
            rx::tiny_string_t<> desc(sizeof(m_err_desc), m_err_desc);
            desc << "DBC::" << err_type_str(dbc_err);
            m_dbc_ec = dbc_err;
            m_pg_ec = 0;
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
        error_info_t(PGconn *handle, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
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
        //���캯��,ͨ��db����ϸ������Ϣ�����õ�ϸ������
        error_info_t(const char* err, const char *source_name = NULL, uint32_t line_number = -1, const char *format = NULL, ...)
        {
            int32_t db_ec;
            char msg[1024];
            get_last_error(db_ec, err, msg, sizeof(msg));
            make_db_error_info(db_ec, msg);

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
        //�ж��Ƿ�Ϊdb�������
        bool is_db_error() { return m_pg_ec != 0; }
        //�õ�db�������
        uint32_t db_error_code() { return m_pg_ec; }
        //�õ�dbc�������
        uint32_t dbc_error_code() { return m_dbc_ec; }
        //-------------------------------------------------
        //�жϴ����Ƿ�Ϊ�û����������
        bool is_bad_user_pwd() { return m_pg_ec == DBEC_DB_BADPWD; }
        //�ж��Ƿ�Ϊ���Ӵ���
        bool is_conn_timeout() { return m_pg_ec == DBEC_DB_CONNTIMEOUT; }
        //���ӶϿ�
        bool is_connection_lost() { return m_pg_ec == DBEC_DB_CONNLOST; }
        //����ʧ��(12541������������;��������;�����ӳ�ʱ;�����Ӷ�ʧ)
        bool is_connect_fail() { return m_pg_ec == DBEC_DB_CONNFAIL || is_bad_user_pwd() || is_conn_timeout() || is_connection_lost(); }
        //-------------------------------------------------
        //�Ƿ�ΪΨһ��Լ����ͻ
        bool is_unique_constraint() { return m_pg_ec == DBEC_DB_UNIQUECONST; }
    };

    //-----------------------------------------------------
    //����db��ʽ������ʱ��Ĺ�����
    class datetime_t
    {
        struct tm  m_Date;
    public:
        //-------------------------------------------------
        datetime_t(void) { memset(&m_Date, 0, sizeof(m_Date)); };
        datetime_t(uint16_t y, uint16_t m, uint16_t d, uint16_t h = 0, uint16_t mi = 0, uint16_t sec = 0)
        {
            set(y, m, d, h, mi, sec);
        }
        datetime_t(const struct tm& o) { set(o); }
        //-------------------------------------------------
        //���ø�������
        void set(uint16_t y, uint16_t m, uint16_t d, uint16_t h = 0, uint16_t mi = 0, uint16_t sc = 0)
        {
            year(y);mon(m);day(d);hour(h);minute(mi);sec(sc);
        }
        void set(const struct tm &ST)
        {
            m_Date = ST;
        }
        //-------------------------------------------------
        //���ʸ�������
        uint16_t year(void) const { return m_Date.tm_year+1900; }
        void year(uint16_t yy) { m_Date.tm_year = yy-1900; }

        uint16_t mon(void) const { return m_Date.tm_mon+1; }
        void mon(uint16_t mm) { m_Date.tm_mon = (uint8_t)mm-1; }

        uint16_t day(void) const { return m_Date.tm_mday; }
        void day(uint16_t dd) { m_Date.tm_mday = dd; }

        uint16_t hour(void) const { return m_Date.tm_hour; }
        void hour(uint16_t hh) { m_Date.tm_hour = hh; }

        uint16_t minute(void) const { return m_Date.tm_min; }
        void minute(uint16_t mi) { m_Date.tm_min = mi; }

        uint16_t sec(void) const { return m_Date.tm_sec; }
        void sec(uint16_t ss) { m_Date.tm_sec = ss; }
        //-------------------------------------------------
        datetime_t& operator = (const struct tm& o) { m_Date = o; return (*this); }
        void to(struct tm& o) const { o = m_Date; }
        //-------------------------------------------------
        //���ݸ����ĸ�ʽ,���ڲ�����ת��Ϊ�ַ�����ʽ
        char* to(char *Result, const char* Format = NULL) const
        {
            if (Format == NULL) Format = "%d-%02d-%02d %02d:%02d:%02d";
            sprintf(Result, Format, year(), mon(), day(), hour(), minute(), sec());
            return Result;
        }
    };
}

#endif
