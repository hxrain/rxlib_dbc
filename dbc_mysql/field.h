#ifndef	_RX_DBC_MYSQL_FIELD_H_
#define	_RX_DBC_MYSQL_FIELD_H_

namespace rx_dbc_mysql
{
    class query_t;
    //-----------------------------------------------------
    //�����ʹ�õ��ֶη��ʶ���
    //-----------------------------------------------------
    //�ֶ���󶨲���ʹ�õĻ���,�������ݻ������Ĺ������ֶ����ݵķ���
    class field_t
    {
    protected:
        typedef rx::tiny_string_t<char, FIELD_NAME_LENGTH> col_name_t;

        col_name_t          m_name;		                    // ��������
        data_type_t	        m_dbc_data_type;		        // �ڴ�����������

        MYSQL_BIND         *m_metainfo;                     // ָ��mysql����Ҫ��Ԫ��Ϣ�ṹ
        uint8_t             m_buff[MAX_TEXT_BYTES];         // ���ݻ�����

        char                m_working_buff[64];             // ��ʱ���ת���ַ����Ļ�����
        //-------------------------------------------------
        //�ֶι��캯��,ֻ�ܱ���¼����ʹ��
        void make(const char *name, uint32_t name_len, MYSQL_BIND *bind,bool is_param=true)
        {
            rx_assert(!is_empty(name));
            reset();

            m_name.assign(name, name_len);
            
            m_metainfo=bind;
            if (is_param)
            {
                memset(m_metainfo,0,sizeof(*m_metainfo));
                m_metainfo->buffer = m_buff;
                m_metainfo->buffer_length = sizeof(m_buff);
                m_metainfo->buffer_type;
            }

            switch(m_metainfo->buffer_type)
            {
            case MYSQL_TYPE_TINY:
            case MYSQL_TYPE_SHORT:
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_LONGLONG:
            case MYSQL_TYPE_FLOAT:
            case MYSQL_TYPE_DOUBLE:
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                m_dbc_data_type=DT_NUMBER;
                break;
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_NEWDATE:
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_TIME2:
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_DATETIME2:
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_TIMESTAMP2:
                m_dbc_data_type=DT_DATE;
                break;
            case MYSQL_TYPE_VARCHAR:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_STRING:
                m_dbc_data_type=DT_TEXT;
                break;
            case MYSQL_TYPE_NULL:
            default:
                m_dbc_data_type=DT_UNKNOWN;
                break;

            }
        }
        //-------------------------------------------------
        void reset()
        {
            m_dbc_data_type = DT_UNKNOWN;
            m_metainfo = NULL;
            m_buff[0] = 0;
            m_name.assign();
        }
        
        /*
                MYSQL_TYPE_NULL

                MYSQL_TYPE_TINY
                MYSQL_TYPE_SHORT
                MYSQL_TYPE_INT24
                MYSQL_TYPE_LONG
                MYSQL_TYPE_LONGLONG

                MYSQL_TYPE_FLOAT
                MYSQL_TYPE_DOUBLE

                MYSQL_TYPE_DECIMAL
                MYSQL_TYPE_NEWDECIMAL

                MYSQL_TYPE_DATE
                MYSQL_TYPE_NEWDATE
                MYSQL_TYPE_TIME
                MYSQL_TYPE_TIME2
                MYSQL_TYPE_DATETIME
                MYSQL_TYPE_DATETIME2
                MYSQL_TYPE_TIMESTAMP
                MYSQL_TYPE_TIMESTAMP2

                MYSQL_TYPE_VARCHAR
                MYSQL_TYPE_TINY_BLOB
                MYSQL_TYPE_MEDIUM_BLOB
                MYSQL_TYPE_LONG_BLOB
                MYSQL_TYPE_BLOB
                MYSQL_TYPE_VAR_STRING
                MYSQL_TYPE_STRING
        */
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ�ַ���:������;ԭʼ���ݻ�����;ԭʼ��������;��ʱ�ַ���������;��ʱ�������ߴ�;ת����ʽ
        PStr comm_as_string(const char* ConvFmt = NULL) const
        {
            switch(m_metainfo->buffer_type)
            {
            case MYSQL_TYPE_TINY:
                return m_metainfo->is_unsigned? rx::st::ultoa(*(uint8_t*)m_buff,(char*)m_working_buff) :rx::st::itoa(*(int8_t*)m_buff,(char*)m_working_buff);
            case MYSQL_TYPE_SHORT:
                return m_metainfo->is_unsigned? rx::st::ultoa(*(uint16_t*)m_buff,(char*)m_working_buff) :rx::st::itoa(*(int16_t*)m_buff,(char*)m_working_buff);
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONG:
                return m_metainfo->is_unsigned? rx::st::ultoa(*(uint32_t*)m_buff,(char*)m_working_buff) :rx::st::itoa(*(int32_t*)m_buff,(char*)m_working_buff);
            case MYSQL_TYPE_LONGLONG:
                return m_metainfo->is_unsigned? rx::st::utoa64(*(uint64_t*)m_buff,(char*)m_working_buff) :rx::st::utoa64(*(uint32_t*)m_buff,(char*)m_working_buff);
                break;
            case MYSQL_TYPE_FLOAT:
                return rx::st::ftoa(*(float*)m_buff,(char*)m_working_buff);
            case MYSQL_TYPE_DOUBLE:
                return rx::st::ftoa(*(double*)m_buff,(char*)m_working_buff);
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                return (char*)m_buff;
                break;
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_NEWDATE:
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_TIME2:
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_DATETIME2:
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_TIMESTAMP2:
                break;
            case MYSQL_TYPE_VARCHAR:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_STRING:
                break;            
            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "col(%s)", m_name.c_str()));
            }
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������:������;ԭʼ���ݻ�����;ԭʼ��������;
        double comm_as_double() const
        {
        }
        long double comm_as_real() const
        {
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ������������:������;ԭʼ���ݻ�����;ԭʼ��������;
        int32_t comm_as_long(bool is_signed = true) const
        {
        }
        int64_t comm_as_longlong() const
        {
        }
        //-----------------------------------------------------
        //ͳһ���ܺ���:��ָ����ԭʼ���͵�����ת��Ϊ����:������;ԭʼ���ݻ�����;ԭʼ��������;
        datetime_t comm_as_datetime() const
        {
        }
        //-------------------------------------------------
        bool m_is_null() const { return m_metainfo==NULL||m_metainfo->is_null_value; }
    public:
        field_t(){ reset(); }
        virtual ~field_t() { reset(); }
        //-------------------------------------------------
        const char* name()const { return m_name.c_str(); }
        data_type_t dbc_data_type() { return m_dbc_data_type; }
        //-------------------------------------------------
        bool is_null(void) const { return m_is_null(); }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�ַ���
        PStr as_string(const char* ConvFmt = NULL) const
        {
            if (m_is_null()) return NULL;
            return comm_as_string( ConvFmt);
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ������
        double as_double(double DefValue = 0) const
        {
            if (m_is_null()) return DefValue;
            return comm_as_double();
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�߾��ȸ�����
        long double as_real(long double DefValue = 0) const
        {
            if (m_is_null()) return DefValue;
            return comm_as_real();
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ��������
        int64_t as_bigint(int64_t DefValue = 0) const 
        { 
            if (m_is_null()) return DefValue;
            return comm_as_longlong(); 
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����������
        int32_t as_long(int32_t DefValue = 0) const
        {
            if (m_is_null()) return DefValue;
            return comm_as_long();
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ�޷�������
        uint32_t as_ulong(uint32_t DefValue = 0) const
        {
            if (m_is_null()) return DefValue;
            return comm_as_long(false);
        }
        //-------------------------------------------------
        //���Ի�ȡ�ڲ�����Ϊ����ʱ��
        datetime_t as_datetime(void) const
        {
            if (m_is_null()) return datetime_t();
            return comm_as_datetime();
        }
    };
}

#endif
