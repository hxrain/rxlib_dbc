#ifndef	_RX_DBC_MYSQL_PARAMETER_H_
#define	_RX_DBC_MYSQL_PARAMETER_H_

namespace mysql
{
    //-----------------------------------------------------
    //sql���󶨲���,������������,Ҳ���������
    //��������ģʽ,���������Ӧ��һ���еĶ���ֵ,ͨ��use_bulk()���е���
    class sql_param_t:public field_t
    {
        friend class stmt_t;

        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //������ֵ
        sql_param_t& set_long(int32_t value, bool is_signed)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_LONG;
            m_metainfo->is_unsigned = !is_signed;
            m_metainfo->length_value = sizeof(value);
            *((int32_t*)m_metainfo->buffer) = value;
            return *this;
        }
        //-------------------------------------------------
        //�󶨴������븡����
        sql_param_t& set_double(double value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_DOUBLE;
            m_metainfo->is_unsigned = 0;
            m_metainfo->length_value = sizeof(value);
            *((double*)m_metainfo->buffer) = value;
            return *this;
        }

        //-------------------------------------------------
        sql_param_t& set_longlong(int64_t value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_LONGLONG;
            m_metainfo->is_unsigned = 1;
            m_metainfo->length_value = sizeof(value);
            *((int64_t*)m_metainfo->buffer) = value;
            return *this;
        }
        //-------------------------------------------------
        //����������
        sql_param_t& set_datetime(const datetime_t& value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_DATETIME;
            m_metainfo->length_value = sizeof(value);
            value.to(*((MYSQL_TIME*)m_metainfo->buffer));
            return *this;
        }
        //-------------------------------------------------
        //���ı���
        sql_param_t& set_string(PStr value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_STRING;
            m_metainfo->length_value = rx::st::strcpy2((char*)m_metainfo->buffer, m_metainfo->buffer_length, value);
            return *this;
        }

    public:
        //-------------------------------------------------
        sql_param_t() :field_t() { }
        ~sql_param_t() { }
        //-------------------------------------------------
        //�õ�ǰ��������Ϊ��ֵ
        void set_null() { rx_assert(m_metainfo!=NULL); m_metainfo->is_null_value=1; }
        //-------------------------------------------------
        //��������ָ����������Ԫ�ظ�ֵ:�ַ���ֵ
        sql_param_t& operator = (PStr text) { return set_string(text);}
        //-------------------------------------------------
        //���ò�����ָ����ŵ�����Ԫ��Ϊ������
        sql_param_t& operator = (double value) { return set_double(value); }
        //-------------------------------------------------
        //���ò���Ϊ������ֵ(������)
        sql_param_t& operator = (int64_t value) { return set_longlong(value); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ(������)
        sql_param_t& operator = (int32_t value) { return set_long(value, true); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ(�޷���)
        sql_param_t& operator = (uint32_t value) { return set_long(value, false); }
        //-------------------------------------------------
        //���ò���Ϊ����ʱ��ֵ
        sql_param_t& operator = (const datetime_t& d) { return set_datetime(d); }
    };
}

#endif
