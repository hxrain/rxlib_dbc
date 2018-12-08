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
        ub4                 m_max_bulk_deep;               //����������
        conn_t		        *m_conn;		                //���������ݿ����Ӷ���
        ub2                 m_bulk_idx;                     //��ǰ�������п����

        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //�ͷ�ȫ������Դ
        void m_clear(void)
        {
            col_base_t::reset();
            m_max_bulk_deep = 0;
            m_bulk_idx = 0;
        }
        //-------------------------------------------------
        //����������ָ����ȵ����ݳߴ�
        void m_set_data_size(ub2 size, ub2 idx=0)
        {
            if (!size)
            {
                m_col_dataempty.at<sb2>(idx) = -1;
                m_col_datasize.at<ub2>(idx) = 0;
            }
            else
            {
                m_col_dataempty.at<sb2>(idx) = 0;
                m_col_datasize.at<ub2>(idx) = size;
            }
        }

        //-------------------------------------------------
        //������������ȷ�ϲ��������ݳ�ʼ��
        //����ֵ:��һ�����OCI��������
        ub2 m_bind_data_type(const char *param_name,ub4 name_size, type_t::data_type_t type, int StringMaxSize, ub4 BulkCount)
        {
            rx_assert(!is_empty(param_name));
            rx_assert(m_max_bulk_deep == (ub4)0);
            rx_assert(BulkCount >= 1);
            m_max_bulk_deep = BulkCount;

            ub2 oci_data_type;
            type_t::data_type_t	dbc_data_type;
            int max_data_size;

            char NamePreDateTypeChar = type_t::DT_UNKNOWN;          //ǰ׺����Ĭ��Ϊ��Ч
            if (param_name[0] == ':')
                NamePreDateTypeChar = param_name[1];        //��':'Ϊǰ���Ĳ��������Ž���ǰ׺���ͽ���

            //���������֪�İ���������,�����ڲ���������ת��
            if (type == type_t::DT_NUMBER || (type == type_t::DT_UNKNOWN && NamePreDateTypeChar == type_t::DT_NUMBER))
            {
                dbc_data_type = type_t::DT_NUMBER;
                oci_data_type = SQLT_VNU;
                max_data_size = sizeof(OCINumber);
            }
            else if (type == type_t::DT_DATE || (type == type_t::DT_UNKNOWN && NamePreDateTypeChar == type_t::DT_DATE))
            {
                dbc_data_type = type_t::DT_DATE;
                oci_data_type = SQLT_ODT;
                max_data_size = sizeof(OCIDate);
            }
            else if (type == type_t::DT_TEXT || (type == type_t::DT_UNKNOWN && NamePreDateTypeChar == type_t::DT_TEXT))
            {
                dbc_data_type = type_t::DT_TEXT;
                oci_data_type = SQLT_STR;
                max_data_size = StringMaxSize;
            }
            else
                //�����͵�ǰ�����ܴ���
                throw (error_info_t(type_t::DBEC_BAD_TYPEPREFIX, __FILE__, __LINE__, "param(%s)",param_name));


            //������������ڴ�,����ʼ����
            col_base_t::make(param_name, name_size, dbc_data_type, max_data_size, BulkCount, true);

            for (ub4 i = 0; i < BulkCount; i++)
                m_set_data_size(0,i);

            return oci_data_type;
        }

        //-------------------------------------------------
        //��ʼ���󶨵���Ӧ���������
        void bind_param(conn_t &conn, OCIStmt* StmtHandle, const char *name, type_t::data_type_t dbc_data_type, int StringMaxSize, int BulkCount)
        {
            rx_assert(!is_empty(name));
            m_clear();
            try
            {
                m_conn = &conn;
                ub4 name_size = (ub4)rx::st::strlen(name);
                //�����������Ͳ����й�һ������,�������ʹ�õĻ�����
                ub2 oci_data_type= m_bind_data_type(name, name_size, dbc_data_type, StringMaxSize, BulkCount);

                //OCI�󶨾��,�����ͷ�
                OCIBind	*bind_handle=NULL;                     

                //���в��������뻺����ָ���Լ��������͵İ�
                sword result = OCIBindByName(StmtHandle, &bind_handle, conn.m_handle_err, (text *)name, name_size,
                    m_col_databuff.ptr(), m_max_data_size,
                    oci_data_type, m_col_dataempty.ptr<sb2>(), m_col_datasize.ptr<ub2>(),
                    NULL,	// pointer conn array of field_t-level return codes
                    0,		// maximum possible number of elements of type m_nType
                    NULL,	// a pointer conn the actual number of elements (PL/sql binds)
                    OCI_DEFAULT);

                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, conn.m_handle_err, __FILE__, __LINE__,"param(%s)", name));
            }
            catch (...)
            {
                m_clear();
                throw;
            }
        }
        //-------------------------------------------------
        //������ֵ
        sql_param_t& set_long(int32_t value, bool is_signed)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                OCINumber* data_buff = m_col_databuff.ptr<OCINumber>(m_bulk_idx);   //�õ����û�����
                sword result = OCINumberFromInt(m_conn->m_handle_err, &value, sizeof(int32_t), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED,data_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)",m_name.c_str()));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case type_t::DT_TEXT:
            {
                char tmp_buff[50];
                if (is_signed)
                    sprintf(tmp_buff, "%d", value);
                else
                    sprintf(tmp_buff, "%u", value);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            }
        }
        //-------------------------------------------------
        //�󶨴������븡����
        sql_param_t& set_double(double value)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                OCINumber* data_buff = m_col_databuff.ptr<OCINumber>(m_bulk_idx);
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(value), data_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case type_t::DT_TEXT:
            {
                char tmp_buff[50];
                sprintf(tmp_buff, "%f", value);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            }
        }
        sql_param_t& set_real(long double value)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                OCINumber* data_buff = m_col_databuff.ptr<OCINumber>(m_bulk_idx);
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(value), data_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case type_t::DT_TEXT:
            {//��real����ת��Ϊ�ı�,ʹ��OCI����
                OCINumber num;
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(value), &num);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)", m_name.c_str()));

                char tmp_buff[50];
                ub4 buff_size = sizeof(tmp_buff);
                result = OCINumberToText(m_conn->m_handle_err, &num, (oratext*)NUMBER_FRM_FMT, NUMBER_FRM_FMT_LEN, NULL, 0, &buff_size, (oratext*)tmp_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            }
            
        }
        //-------------------------------------------------
        //����������
        sql_param_t& set_datetime(const datetime_t& d)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_DATE:
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                d.to(m_col_databuff.at<OCIDate>(m_bulk_idx));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case type_t::DT_TEXT:
            {
                char tmp_buff[21];
                d.to(tmp_buff);
                return set_string(tmp_buff);
            }
            default:
                throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            }
        }
        //-------------------------------------------------
        //���ı���
        sql_param_t& set_string(PStr text)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (is_empty(text))
            {//����ʲô����,�մ��������Ϊ��ֵ
                m_set_data_size(0, m_bulk_idx);   //��¼���ݵ�ʵ�ʳ���
                return *this;
            }

            switch (m_dbc_data_type)
            {
            case type_t::DT_TEXT:
            {//��ǰʵ�������������ı���,��ֵ��Ҳ���ı�,��ô�ͽ��п�����ֵ��
                ub2 data_len = ub2(strlen(text));           //�õ�����ʵ�ʳ���
                ub1 *data_buff = m_col_databuff.ptr(m_bulk_idx*m_max_data_size);   //�õ����û�����
                if (data_len > m_max_data_size)             //��������̫����,���нضϰ�
                {
                    rx_alert("�������ݹ���,���ڰ�ʱ�����������ߴ�");
                    data_len = ub2((m_max_data_size - 2) & ~1);
                }

                memcpy(data_buff, text, data_len);          //���������ݿ�������Ԫ�ض�Ӧ�Ŀռ�
                *((char *)data_buff + data_len++) = '\0';   //���ÿռ�Ĵ�β���ý�����

                m_set_data_size(data_len, m_bulk_idx);      //��¼���ݵ�ʵ�ʳ���
                return (*this);
            }
            case type_t::DT_DATE:
            {//��ǰʵ����������������,������ֵ�����ı���,��ô�ͽ���ת�������;Ĭ��ֻ����"yyyy-mm-dd hh:mi:ss"�ĸ�ʽ
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))             //ת�����ڸ�ʽ
                    throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                D.set(ST);
                return set_datetime(D);                     //����ʵ�ʵĹ��ܺ���
            }
            case type_t::DT_NUMBER:
            {//��ǰʵ����������������,������ֵ��ʱ�����ı���,��ô�ͽ���ת�������
                double Value = rx::st::atof(text);
                return set_double(Value);                   //����ʵ�ʵĹ��ܺ���
            }
            default:
                throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            }
        }
        //-------------------------------------------------
        //�õ�������
        OCIError* oci_err_handle() const { return m_conn->m_handle_err; }
        //�õ���ǰ�ķ����к�
        ub2 bulk_row_idx() const { return m_bulk_idx; }
        //-------------------------------------------------
        //���ÿ�������
        void bulk(ub2 idx)
        {
            if (idx >= m_max_bulk_deep)
                throw (error_info_t(type_t::DBEC_IDX_OVERSTEP, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            m_bulk_idx = idx;
        }
        //��ȡ�������
        ub2 bulks() { return m_max_bulk_deep; }
    public:
        //-------------------------------------------------
        sql_param_t(rx::mem_allotter_i &ma) :col_base_t(ma) { m_clear(); }
        ~sql_param_t() { m_clear(); }
        //-------------------------------------------------
        //�õ�ǰ��������Ϊ��ֵ
        void set_null() { rx_assert(bulk_row_idx() < m_max_bulk_deep); m_set_data_size(0, bulk_row_idx()); }
        bool is_null() const { rx_assert(bulk_row_idx() < m_max_bulk_deep); return col_base_t::m_is_null(bulk_row_idx()); }
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
