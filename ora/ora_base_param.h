#ifndef	_RX_DBC_ORA_PARAMETER_H_
#define	_RX_DBC_ORA_PARAMETER_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //sql���󶨲���,������������,Ҳ���������
    //��������ģʽ,���������Ӧ��һ���еĶ���ֵ,ͨ��use_bulk()���е���
    class sql_param_t:public col_base_t
    {
        friend class stmt_t;
        ub4                 m_max_bulk_count;               //����������
        conn_t		        *m_conn;		                //���������ݿ����Ӷ���
        ub2                 m_bulk_idx;                     //��ǰ�������п����

        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //�ͷ�ȫ������Դ
        void clear(void)
        {
            col_base_t::reset();
            m_max_bulk_count = 0;
            m_bulk_idx = 0;
        }

        //-------------------------------------------------
        //������������ȷ�ϲ��������ݳ�ʼ��
        void m_init_data_type(const char *param_name, data_type_t type, int StringMaxSize, ub4 BulkCount)
        {
            rx_assert(!is_empty(param_name));
            rx_assert(m_max_bulk_count == (ub4)0);
            rx_assert(BulkCount >= 1);
            m_max_bulk_count = BulkCount;

            ub2 oci_data_type;
            data_type_t	dbc_data_type;
            int max_data_size;

            char NamePreDateTypeChar = ' ';                   //Ĭ��ǰ׺��Ч
            if (param_name[0] == ':')
                NamePreDateTypeChar = param_name[1];          //��':'Ϊǰ���Ĳ��������Ž���ǰ׺���ͽ���

            //���������֪�İ���������,�����ڲ���������ת��
            if (type == DT_NUMBER || (type == DT_UNKNOWN && NamePreDateTypeChar == PP_NUMERIC))
            {
                dbc_data_type = DT_NUMBER;
                oci_data_type = SQLT_VNU;
                max_data_size = sizeof(OCINumber);
            }
            else if (type == DT_DATE || (type == DT_UNKNOWN && NamePreDateTypeChar == PP_DATE))
            {
                dbc_data_type = DT_DATE;
                oci_data_type = SQLT_ODT;
                max_data_size = sizeof(OCIDate);
            }
            else if (type == DT_TEXT || (type == DT_UNKNOWN && NamePreDateTypeChar == PP_TEXT))
            {
                dbc_data_type = DT_TEXT;
                oci_data_type = SQLT_STR;
                max_data_size = StringMaxSize;
            }
            else
                //�����͵�ǰ�����ܴ���
                throw (error_info_t(DBEC_BAD_TYPEPREFIX, __FILE__, __LINE__, param_name));


            //������������ڴ�,����ʼ����
            col_base_t::make(param_name, strlen(param_name), oci_data_type, dbc_data_type, max_data_size, BulkCount, true);

            for (ub4 i = 0; i < BulkCount; i++)
            {
                m_col_dataempty.at(i) = -1;
                m_col_datasize.at(i) = 0;
            }
        }

        //-------------------------------------------------
        //��ʼ���󶨵���Ӧ���������
        void bind(conn_t &conn, OCIStmt* StmtHandle, const char *name, data_type_t type, int StringMaxSize, int BulkCount)
        {
            rx_assert(!is_empty(name));
            clear();
            try
            {
                m_conn = &conn;

                //���Ը��ݲ�����ǰ׺���⴦�������������,���û����ȷ���ò������͵Ļ�
                m_init_data_type(name, type, StringMaxSize, BulkCount);

                //OCI�󶨾��,�����ͷ�
                OCIBind	*bind_handle=NULL;                     

                sword result = OCIBindByName(StmtHandle, &bind_handle, conn.m_handle_err, (text *)name, (ub4)rx::st::strlen(name),
                    m_col_databuff.array(), m_max_data_size,
                    m_oci_data_type, m_col_dataempty.array(), m_col_datasize.array(),
                    NULL,	// pointer conn array of field_t-level return codes
                    0,		// maximum possible number of elements of type m_nType
                    NULL,	// a pointer conn the actual number of elements (PL/SQL binds)
                    OCI_DEFAULT);

                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, conn.m_handle_err, __FILE__, __LINE__, name));
            }
            catch (...)
            {
                clear();
                throw;
            }
        }
        //-------------------------------------------------
        //������ֵ
        sql_param_t& set_long(int32_t value, bool is_signed)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* data_buff = &m_col_databuff.at(m_bulk_idx*m_max_data_size);   //�õ����û�����
                sword result = OCINumberFromInt(m_conn->m_handle_err, &value, sizeof(int32_t), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED, reinterpret_cast <OCINumber *> (data_buff));
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__));
                m_col_dataempty.at(m_bulk_idx) = 0;              //��ǲ����ǿ���
                m_col_datasize.at(m_bulk_idx) = m_max_data_size; //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case DT_TEXT:
            {
                char tmp_buff[50];
                if (is_signed)
                    sprintf(tmp_buff, "%d", value);
                else
                    sprintf(tmp_buff, "%u", value);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(DBEC_BAD_INPUT, __FILE__, __LINE__));
            }
        }
        //-------------------------------------------------
        //�󶨴������븡����
        sql_param_t& set_double(double value)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* data_buff = &m_col_databuff.at(m_bulk_idx*m_max_data_size);   //�õ����û�����
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(double), reinterpret_cast <OCINumber *> (data_buff));
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__));
                m_col_dataempty.at(m_bulk_idx) = 0;                 //��ǲ����ǿ���
                m_col_datasize.at(m_bulk_idx) = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case DT_TEXT:
            {
                char tmp_buff[50];
                sprintf(tmp_buff, "%f", value);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(DBEC_BAD_INPUT, __FILE__, __LINE__));
            }
        }
        sql_param_t& set_real(long double value)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* data_buff = &m_col_databuff.at(m_bulk_idx*m_max_data_size);
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(value), reinterpret_cast <OCINumber *> (data_buff));
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__));
                m_col_dataempty.at(m_bulk_idx) = 0;                 //��ǲ����ǿ���
                m_col_datasize.at(m_bulk_idx) = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case DT_TEXT:
            {
                char tmp_buff[50];
                sprintf(tmp_buff, "%Lf", value);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(DBEC_BAD_INPUT, __FILE__, __LINE__));
            }
            
        }
        //-------------------------------------------------
        //����������
        sql_param_t& set_datetime(const datetime_t& d)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case DT_DATE:
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                ub1* data_buff = &m_col_databuff.at(m_bulk_idx*m_max_data_size);
                d.to(*reinterpret_cast <OCIDate*> (data_buff));
                m_col_dataempty.at(m_bulk_idx) = 0;                 //��ǲ����ǿ���
                m_col_datasize.at(m_bulk_idx) = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case DT_TEXT:
            {
                char tmp_buff[21];
                d.to(tmp_buff);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(DBEC_BAD_INPUT, __FILE__, __LINE__));
            }
        }
        //-------------------------------------------------
        //���ı���
        sql_param_t& set_string(PStr text)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (is_empty(text))
            {//����ʲô����,�մ��������Ϊ��ֵ
                m_col_dataempty.at(m_bulk_idx) = -1;
                return *this;
            }

            switch (m_dbc_data_type)
            {
            case DT_TEXT:
            {//��ǰʵ�������������ı���,��ֵ��Ҳ���ı�,��ô�ͽ��п�����ֵ��
                ub2 data_len = ub2(strlen(text));           //�õ�����ʵ�ʳ���
                ub1 *data_buff = &m_col_databuff.at(m_bulk_idx*m_max_data_size);   //�õ����û�����
                if (data_len > m_max_data_size)             //��������̫����,���нضϰ�
                {
                    rx_alert("�������ݹ���,���ڰ�ʱ�����������ߴ�");
                    data_len = ub2((m_max_data_size - 2) & ~1);
                }

                memcpy(data_buff, text, data_len);          //���������ݿ�������Ԫ�ض�Ӧ�Ŀռ�
                *((char *)data_buff + data_len++) = '\0';   //���ÿռ�Ĵ�β���ý�����

                m_col_dataempty.at(m_bulk_idx) = 0;         //��ǲ����ǿ���
                m_col_datasize.at(m_bulk_idx) = data_len;   //��¼��Ԫ�ص�ʵ�ʳ���  
                return (*this);
            }
            case DT_DATE:
            {//��ǰʵ����������������,������ֵ�����ı���,��ô�ͽ���ת�������;Ĭ��ֻ����"yyyy-mm-dd hh:mi:ss"�ĸ�ʽ
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))             //ת�����ڸ�ʽ
                    throw (error_info_t(DBEC_BAD_INPUT, __FILE__, __LINE__));
                D.set(ST);
                return set_datetime(D);                     //����ʵ�ʵĹ��ܺ���
            }
            case DT_NUMBER:
            {//��ǰʵ����������������,������ֵ��ʱ�����ı���,��ô�ͽ���ת�������
                double Value = rx::st::atof(text);
                return set_double(Value);                   //����ʵ�ʵĹ��ܺ���
            }
            default:
                throw (error_info_t(DBEC_BAD_INPUT, __FILE__, __LINE__));
            }
        }
        //-------------------------------------------------
        //�õ�������
        OCIError* oci_err_handle() const { return m_conn->m_handle_err; }
        //�õ���ǰ�ķ����к�
        ub2 bulk_row_idx() const { return m_bulk_idx; }
    public:
        //-------------------------------------------------
        sql_param_t(rx::mem_allotter_i &ma) :col_base_t(ma) { clear(); }
        //-------------------------------------------------
        ~sql_param_t() { clear(); }
        //-------------------------------------------------
        //���ÿ�������
        void bulk_use(ub2 idx) 
        {
            if (idx>= m_max_bulk_count) 
                throw (error_info_t(DBEC_METHOD_CALL, __FILE__, __LINE__));
            m_bulk_idx = idx;
        }
        //��ȡ�ɷ��ʿ�����
        ub2 bulk_count() { return m_max_bulk_count; }
        //-------------------------------------------------
        //�õ�ǰ��������Ϊ��ֵ
        void set_null() { rx_assert(bulk_row_idx() < m_max_bulk_count); m_col_dataempty.at(bulk_row_idx()) = -1; }
        bool is_null() const { rx_assert(bulk_row_idx() < m_max_bulk_count); return (m_col_dataempty.at(bulk_row_idx()) == -1); }
        //-------------------------------------------------
        //��������ָ����������Ԫ�ظ�ֵ:�ַ���ֵ
        sql_param_t& operator = (PStr text) { return set_string(text);}
        //-------------------------------------------------
        //���ò�����ָ����ŵ�����Ԫ��Ϊ������
        sql_param_t& operator = (double value) { return set_double(value); }
        sql_param_t& operator = (long double value) { return set_real(value); }
        //-------------------------------------------------
        //���ò���Ϊ������ֵ(������)
        sql_param_t& operator = (int64_t value) { return set_real((long double)value); }
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
