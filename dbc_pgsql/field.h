#ifndef	_RX_DBC_PGSQL_FIELD_H_
#define	_RX_DBC_PGSQL_FIELD_H_

namespace pqsql
{
    class query_t;
    //-----------------------------------------------------
    //�����ʹ�õ��ֶη��ʶ���
    //-----------------------------------------------------
    //�ֶ���󶨲���ʹ�õĻ���,�������ݻ������Ĺ������ֶ����ݵķ���
    class field_t
    {
    protected:
        typedef rx::tiny_string_t<char, ::FIELD_NAME_LENGTH> col_name_t;
        friend class query_t;

        col_name_t          m_name;		                    // ��������
        pqsql_BIND         *m_metainfo;                     // ָ��pqsql����Ҫ��Ԫ��Ϣ�ṹ
        uint8_t             m_buff[MAX_TEXT_BYTES];         // ���ݻ�����
        char                m_working_buff[64];             // ��ʱ���ת���ַ����Ļ�����

        //-------------------------------------------------
        //�ֶι��캯��,ֻ�ܱ���¼����ʹ��
        void make(const char *name, pqsql_BIND *bind)
        {
            rx_assert(!is_empty(name));
            reset();

            m_name.assign(name);
            
            m_metainfo=bind;
            memset(m_metainfo,0,sizeof(*m_metainfo));
            //��Ԫ��Ϣ�ṹ�ĳ�ʼ��
            m_metainfo->buffer = m_buff;
            m_metainfo->buffer_length = sizeof(m_buff);
            m_metainfo->is_null = &m_metainfo->is_null_value;
            m_metainfo->length = &m_metainfo->length_value;
            m_metainfo->error = &m_metainfo->error_value;
        }
        //-------------------------------------------------
        void reset()
        {
            m_metainfo = NULL;
            m_buff[0] = 0;
            m_name.assign();
        }

        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ�ַ���:������;ԭʼ���ݻ�����;ԭʼ��������;��ʱ�ַ���������;��ʱ�������ߴ�;ת����ʽ
        PStr comm_as_string(const char* ConvFmt = NULL) const
        {
            #define NUM2STR(sfunc,ufunc,stype,utype) m_metainfo->is_unsigned? rx::st::ufunc(*(utype*)m_buff,(char*)m_working_buff) :rx::st::sfunc(*(stype*)m_buff,(char*)m_working_buff)

            switch(m_metainfo->buffer_type)
            {
            case pqsql_TYPE_TINY:
                return NUM2STR(itoa, ultoa, int8_t, uint8_t);
            case pqsql_TYPE_SHORT:
                return NUM2STR(itoa, ultoa, int16_t, uint16_t);
            case pqsql_TYPE_INT24:
            case pqsql_TYPE_LONG:
                return NUM2STR(itoa, ultoa, int32_t, uint32_t);
            case pqsql_TYPE_LONGLONG:
                return NUM2STR(itoa64, utoa64, int64_t, uint64_t);
            case pqsql_TYPE_FLOAT:
                return rx::st::ftoa(*(float*)m_buff,(char*)m_working_buff);
            case pqsql_TYPE_DOUBLE:
                return rx::st::ftoa(*(double*)m_buff,(char*)m_working_buff);
            case pqsql_TYPE_DECIMAL:
            case pqsql_TYPE_NEWDECIMAL:
                return (char*)m_buff;
            case pqsql_TYPE_DATE:
            case pqsql_TYPE_NEWDATE:
            case pqsql_TYPE_TIME:
            case pqsql_TYPE_TIME2:
            case pqsql_TYPE_DATETIME:
            case pqsql_TYPE_DATETIME2:
            case pqsql_TYPE_TIMESTAMP:
            case pqsql_TYPE_TIMESTAMP2:
            {
                datetime_t dt = *(pqsql_TIME*)m_buff;
                return dt.to((char*)m_working_buff);
            }
            case pqsql_TYPE_VARCHAR:
            case pqsql_TYPE_TINY_BLOB:
            case pqsql_TYPE_MEDIUM_BLOB:
            case pqsql_TYPE_LONG_BLOB:
            case pqsql_TYPE_BLOB:
            case pqsql_TYPE_VAR_STRING:
            case pqsql_TYPE_STRING:
                return (char*)m_buff;
            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
            #undef NUM2STR
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������:������;ԭʼ���ݻ�����;ԭʼ��������;
        double comm_as_double() const
        {
            #define NUM2NUM(stype,utype) m_metainfo->is_unsigned? (*(utype*)m_buff) : (*(stype*)m_buff)
            switch (m_metainfo->buffer_type)
            {
            case pqsql_TYPE_TINY:
                return NUM2NUM(int8_t, uint8_t);
            case pqsql_TYPE_SHORT:
                return NUM2NUM(int16_t, uint16_t);
            case pqsql_TYPE_INT24:
            case pqsql_TYPE_LONG:
                return NUM2NUM(int32_t, uint32_t);
            case pqsql_TYPE_LONGLONG:
                return (double)(NUM2NUM(int64_t, uint64_t));
            case pqsql_TYPE_FLOAT:
                return *(float*)m_buff;
            case pqsql_TYPE_DOUBLE:
                return *(double*)m_buff;
            case pqsql_TYPE_DECIMAL:
            case pqsql_TYPE_NEWDECIMAL:
                return rx::st::atof((char*)m_buff);
            case pqsql_TYPE_VARCHAR:
            case pqsql_TYPE_TINY_BLOB:
            case pqsql_TYPE_MEDIUM_BLOB:
            case pqsql_TYPE_LONG_BLOB:
            case pqsql_TYPE_BLOB:
            case pqsql_TYPE_VAR_STRING:
            case pqsql_TYPE_STRING:
                return rx::st::atof((char*)m_buff);
            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
            #undef NUM2NUM
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������������:������;ԭʼ���ݻ�����;ԭʼ��������;
        int32_t comm_as_int(bool is_signed = true) const
        {
            #define NUM2NUM(stype,utype) m_metainfo->is_unsigned? (*(utype*)m_buff) : (*(stype*)m_buff)
            switch (m_metainfo->buffer_type)
            {
            case pqsql_TYPE_TINY:
                return NUM2NUM(int8_t, uint8_t);
            case pqsql_TYPE_SHORT:
                return NUM2NUM(int16_t, uint16_t);
            case pqsql_TYPE_INT24:
            case pqsql_TYPE_LONG:
                return NUM2NUM(int32_t, uint32_t);
            case pqsql_TYPE_LONGLONG:
                return int32_t(NUM2NUM(int64_t, uint64_t));
            case pqsql_TYPE_FLOAT:
                return int32_t(*(float*)m_buff);
            case pqsql_TYPE_DOUBLE:
                return int32_t(*(double*)m_buff);
            case pqsql_TYPE_DECIMAL:
            case pqsql_TYPE_NEWDECIMAL:
                return int32_t(rx::st::atoi((char*)m_buff));
            case pqsql_TYPE_VARCHAR:
            case pqsql_TYPE_TINY_BLOB:
            case pqsql_TYPE_MEDIUM_BLOB:
            case pqsql_TYPE_LONG_BLOB:
            case pqsql_TYPE_BLOB:
            case pqsql_TYPE_VAR_STRING:
            case pqsql_TYPE_STRING:
                return int32_t(rx::st::atoi((char*)m_buff));
            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
            #undef NUM2NUM
        }
        //-----------------------------------------------------
        int64_t comm_as_intlong() const
        {
            #define NUM2NUM(stype,utype) m_metainfo->is_unsigned? (*(utype*)m_buff) : (*(stype*)m_buff)
            switch (m_metainfo->buffer_type)
            {
            case pqsql_TYPE_TINY:
                return NUM2NUM(int8_t, uint8_t);
            case pqsql_TYPE_SHORT:
                return NUM2NUM(int16_t, uint16_t);
            case pqsql_TYPE_INT24:
            case pqsql_TYPE_LONG:
                return NUM2NUM(int32_t, uint32_t);
            case pqsql_TYPE_LONGLONG:
                return NUM2NUM(int64_t, uint64_t);
            case pqsql_TYPE_FLOAT:
                return int64_t(*(float*)m_buff);
            case pqsql_TYPE_DOUBLE:
                return int64_t(*(double*)m_buff);
            case pqsql_TYPE_DECIMAL:
            case pqsql_TYPE_NEWDECIMAL:
                return rx::st::atoi64((char*)m_buff);
            case pqsql_TYPE_VARCHAR:
            case pqsql_TYPE_TINY_BLOB:
            case pqsql_TYPE_MEDIUM_BLOB:
            case pqsql_TYPE_LONG_BLOB:
            case pqsql_TYPE_BLOB:
            case pqsql_TYPE_VAR_STRING:
            case pqsql_TYPE_STRING:
                return rx::st::atoi64((char*)m_buff);
            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
            #undef NUM2NUM
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ����:������;ԭʼ���ݻ�����;ԭʼ��������;
        datetime_t comm_as_datetime() const
        {
            switch (m_metainfo->buffer_type)
            {
            case pqsql_TYPE_DATE:
            case pqsql_TYPE_NEWDATE:
            case pqsql_TYPE_TIME:
            case pqsql_TYPE_TIME2:
            case pqsql_TYPE_DATETIME:
            case pqsql_TYPE_DATETIME2:
            case pqsql_TYPE_TIMESTAMP:
            case pqsql_TYPE_TIMESTAMP2:
            {
                datetime_t dt = *(pqsql_TIME*)m_buff;
                return dt;
            }
            case pqsql_TYPE_VARCHAR:
            case pqsql_TYPE_TINY_BLOB:
            case pqsql_TYPE_MEDIUM_BLOB:
            case pqsql_TYPE_LONG_BLOB:
            case pqsql_TYPE_BLOB:
            case pqsql_TYPE_VAR_STRING:
            case pqsql_TYPE_STRING:
            {
                struct tm st;
                if (!rx_iso_datetime((char*)m_buff, st))
                    throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
                datetime_t dt;
                dt.set(st);
                return dt;
            }
            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-------------------------------------------------
        bool m_is_null() const { return m_metainfo==NULL||m_metainfo->is_null_value; }
    public:
        field_t(){ reset(); }
        virtual ~field_t() { reset(); }
        //-------------------------------------------------
        const char* name()const { return m_name.c_str(); }
        //-------------------------------------------------
        data_type_t dbc_data_type() 
        {
            if (m_is_null()) return DT_UNKNOWN;
            switch (m_metainfo->buffer_type)
            {
            case pqsql_TYPE_TINY:
            case pqsql_TYPE_SHORT:
            case pqsql_TYPE_INT24:
            case pqsql_TYPE_LONG:
                return m_metainfo->is_unsigned ? DT_UINT : DT_INT;
            case pqsql_TYPE_LONGLONG:
                return DT_LONG;
            case pqsql_TYPE_FLOAT:
            case pqsql_TYPE_DOUBLE:
            case pqsql_TYPE_DECIMAL:
            case pqsql_TYPE_NEWDECIMAL:
                return DT_FLOAT;

            case pqsql_TYPE_DATE:
            case pqsql_TYPE_NEWDATE:
            case pqsql_TYPE_TIME:
            case pqsql_TYPE_TIME2:
            case pqsql_TYPE_DATETIME:
            case pqsql_TYPE_DATETIME2:
            case pqsql_TYPE_TIMESTAMP:
            case pqsql_TYPE_TIMESTAMP2:
                return DT_DATE;

            case pqsql_TYPE_VARCHAR:
            case pqsql_TYPE_TINY_BLOB:
            case pqsql_TYPE_MEDIUM_BLOB:
            case pqsql_TYPE_LONG_BLOB:
            case pqsql_TYPE_BLOB:
            case pqsql_TYPE_VAR_STRING:
            case pqsql_TYPE_STRING:
                return DT_TEXT;
            case pqsql_TYPE_NULL:
            default:
                return DT_UNKNOWN;
            }
        }
        //-------------------------------------------------
        bool is_null(void) const { return m_is_null(); }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�ַ���
        PStr as_string(const char* ConvFmt = NULL) const
        {
            if (m_is_null()) return NULL;
            return comm_as_string( ConvFmt);
        }
        field_t& to(char* buff, uint32_t max_size = 0)
        {
            if (max_size)
                rx::st::strcpy(buff, max_size, as_string());
            else
                rx::st::strcpy(buff, as_string());
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ������
        double as_double(double def_val = 0) const
        {
            if (m_is_null()) return def_val;
            return comm_as_double();
        }
        field_t& to(double &buff, double def_val = 0)
        {
            buff = as_double(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ��������
        int64_t as_long(int64_t def_val = 0) const 
        { 
            if (m_is_null()) return def_val;
            return comm_as_intlong(); 
        }
        field_t& to(int64_t &buff, int64_t def_val = 0)
        {
            buff = as_long(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����������
        int32_t as_int(int32_t def_val = 0) const
        {
            if (m_is_null()) return def_val;
            return comm_as_int();
        }
        field_t& to(int32_t &buff, int32_t def_val = 0)
        {
            buff = as_int(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�޷�������
        uint32_t as_uint(uint32_t def_val = 0) const
        {
            if (m_is_null()) return def_val;
            return comm_as_int(false);
        }
        field_t& to(uint32_t &buff, uint32_t def_val = 0)
        {
            buff = as_uint(def_val);
            return *this;
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����ʱ��
        datetime_t as_datetime(void) const
        {
            if (m_is_null()) return datetime_t();
            return comm_as_datetime();
        }
        field_t& to(datetime_t &buff)
        {
            buff = as_datetime();
            return *this;
        }
    };
}

#endif
