#ifndef	_RX_DBC_PGSQL_PARAMETER_H_
#define	_RX_DBC_PGSQL_PARAMETER_H_

namespace pgsql
{
    //-----------------------------------------------------
    //sql���󶨲���,������������,Ҳ���������
    class param_t:public col_base_t
    {
        friend class stmt_t;
        //-------------------------------------------------
        param_t(const param_t&);
        param_t& operator = (const param_t&);

        char            m_buff[MAX_TEXT_BYTES];
        char          **m_values;
        //-------------------------------------------------
        //�жϵ�ǰ���Ƿ�Ϊnull��ֵ
        virtual bool m_is_null() const { return m_values==NULL|| m_values[m_idx]==NULL; }
        //-------------------------------------------------
        //��ȡ��ǰ����������
        virtual const char* m_value() const { return m_buff; }
        //-------------------------------------------------
        //������ֵ
        param_t& set_long(int32_t value, bool is_signed)
        {
            switch (pg_data_type())
            {
                case PG_DATA_TYPE_DATE       :
                case PG_DATA_TYPE_TIME       :
                case PG_DATA_TYPE_ABSTIME    :
                case PG_DATA_TYPE_TIMESTAMP  :
                case PG_DATA_TYPE_TIMESTAMPTZ:
                    throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "param(%s:%d)", m_name.c_str(),m_idx));
                    break;
                default                      :
                    if (is_signed)
                        rx::st::itoa(value, m_buff);
                    else
                        rx::st::ultoa(value, m_buff);
                    break;
            }

            m_values[m_idx] = m_buff;
            return *this;
        }
        //-------------------------------------------------
        //�󶨴������븡����
        param_t& set_double(double value)
        {
            switch (pg_data_type())
            {
                case PG_DATA_TYPE_DATE       :
                case PG_DATA_TYPE_TIME       :
                case PG_DATA_TYPE_ABSTIME    :
                case PG_DATA_TYPE_TIMESTAMP  :
                case PG_DATA_TYPE_TIMESTAMPTZ:
                    throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "param(%s:%d)", m_name.c_str(), m_idx));
                    break;
                case PG_DATA_TYPE_INT2       :
                case PG_DATA_TYPE_INT4       :
                case PG_DATA_TYPE_INT8       :
                case PG_DATA_TYPE_OID        :
                    rx::st::itoa64((int64_t)value, m_buff);
                    break;
                case PG_DATA_TYPE_FLOAT4     :
                case PG_DATA_TYPE_FLOAT8     :
                case PG_DATA_TYPE_NUMERIC    :
                case PG_DATA_TYPE_BYTEA      :
                case PG_DATA_TYPE_CHAR       :
                case PG_DATA_TYPE_NAME       :
                case PG_DATA_TYPE_TEXT       :
                case PG_DATA_TYPE_BPCHAR     :
                case PG_DATA_TYPE_VARCHAR    :
                default                      :
                    rx::st::ftoa(value, m_buff);
                    break;
            }

            m_values[m_idx] = m_buff;
            return *this;
        }
        //-------------------------------------------------
        param_t& set_longlong(int64_t value)
        {
            switch (pg_data_type())
            {
                case PG_DATA_TYPE_INT2       :
                case PG_DATA_TYPE_INT4       :
                case PG_DATA_TYPE_INT8       :
                case PG_DATA_TYPE_OID        :
                case PG_DATA_TYPE_FLOAT4     :
                case PG_DATA_TYPE_FLOAT8     :
                case PG_DATA_TYPE_NUMERIC    :
                case PG_DATA_TYPE_BYTEA      :
                case PG_DATA_TYPE_CHAR       :
                case PG_DATA_TYPE_NAME       :
                case PG_DATA_TYPE_TEXT       :
                case PG_DATA_TYPE_BPCHAR     :
                case PG_DATA_TYPE_VARCHAR    :
                    rx::st::itoa64(value, m_buff);
                    break;
                case PG_DATA_TYPE_DATE       :
                case PG_DATA_TYPE_TIME       :
                case PG_DATA_TYPE_ABSTIME    :
                case PG_DATA_TYPE_TIMESTAMP  :
                case PG_DATA_TYPE_TIMESTAMPTZ:
                default                      :
                    throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "param(%s:%d)", m_name.c_str(), m_idx));
            }
            return *this;
        }
        //-------------------------------------------------
        //����������
        param_t& set_datetime(const datetime_t& value)
        {
            switch (pg_data_type())
            {
                case PG_DATA_TYPE_INT2       :
                case PG_DATA_TYPE_INT4       :
                case PG_DATA_TYPE_INT8       :
                case PG_DATA_TYPE_OID        :
                case PG_DATA_TYPE_FLOAT4     :
                case PG_DATA_TYPE_FLOAT8     :
                case PG_DATA_TYPE_NUMERIC    :
                    throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, "param(%s:%d)", m_name.c_str(), m_idx));
                    break;
                case PG_DATA_TYPE_BYTEA      :
                case PG_DATA_TYPE_CHAR       :
                case PG_DATA_TYPE_NAME       :
                case PG_DATA_TYPE_TEXT       :
                case PG_DATA_TYPE_BPCHAR     :
                case PG_DATA_TYPE_VARCHAR    :
                case PG_DATA_TYPE_DATE       :
                case PG_DATA_TYPE_TIME       :
                case PG_DATA_TYPE_ABSTIME    :
                case PG_DATA_TYPE_TIMESTAMP  :
                case PG_DATA_TYPE_TIMESTAMPTZ:
                default                      :
                    value.to(m_buff);
                    break;
            }
            m_values[m_idx] = m_buff;
            return *this;
        }
        //-------------------------------------------------
        //���ı���
        param_t& set_string(PStr value)
        {
            rx::st::strcpy2(m_buff,sizeof(m_buff),value);
            m_values[m_idx] = m_buff;
            return *this;
        }
    protected:
        //-------------------------------------------------
        //����ǰ��������󶨶�Ӧ��Ԫ��Ϣ
        void bind(char **values,int idx,int *type_oid,const char* name, int type)
        {
            m_values = values;
            col_base_t::bind(idx,name,type_oid);
            set_null();                                     //Ĭ�ϲ���ֵΪnull��
            rx_assert(m_values != NULL);
            rx_assert(m_idx != -1);
            rx_assert(m_type_oid != NULL);
            *m_type_oid = type;
        }
        //-------------------------------------------------
        //����Ϣ��λ
        void reset()
        {
            m_values = NULL;
            m_buff[0] = 0;
            col_base_t::reset();
        }
    public:
        //-------------------------------------------------
        param_t() :col_base_t() { reset(); }
        ~param_t() { }
        //-------------------------------------------------
        //�õ�ǰ��������Ϊ��ֵ
        void set_null() { rx_assert(m_values != NULL && m_idx!=-1); m_buff[0] = 0; if (m_values) m_values[m_idx] = NULL; }
        //-------------------------------------------------
        //��������ָ����������Ԫ�ظ�ֵ:�ַ���ֵ
        param_t& operator = (PStr text) { return set_string(text);}
        //-------------------------------------------------
        //���ò�����ָ����ŵ�����Ԫ��Ϊ������
        param_t& operator = (double value) { return set_double(value); }
        //-------------------------------------------------
        //���ò���Ϊ������ֵ(������)
        param_t& operator = (int64_t value) { return set_longlong(value); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ(������)
        param_t& operator = (int32_t value) { return set_long(value, true); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ(�޷���)
        param_t& operator = (uint32_t value) { return set_long(value, false); }
        //-------------------------------------------------
        //���ò���Ϊ����ʱ��ֵ
        param_t& operator = (const datetime_t& d) { return set_datetime(d); }
    };
}

#endif
