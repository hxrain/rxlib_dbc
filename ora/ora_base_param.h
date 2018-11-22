#ifndef	_RX_DBC_ORA_PARAMETER_H_
#define	_RX_DBC_ORA_PARAMETER_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //sql���󶨲���,������������,Ҳ���������
    //��������ģʽ,���������Ӧ��һ���еĶ���ֵ
    class sql_param_t
    {
        friend class stmt_t;
        typedef rx::array_t<ub2> array_datasize_t;
        typedef rx::array_t<ub1> array_databuff_t;
        typedef rx::array_t<sb2> array_dataempty_t;

        data_type_t	        m_dbc_data_type;		        //�ڴ�����������
        ub2				    m_oci_data_type;	            //OCI�ײ���������,���ߴ�������Զ�ת��
        ub2				    m_max_data_size;	            //���ݵ����ߴ�
        ub4                 m_max_bulk_count;               //����������
        array_datasize_t    m_bulks_datasize;		        //���ݵĳ���ָʾ
        array_databuff_t    m_bulks_databuff;	            //ģ���ά������ʵ����ݻ�����
        array_dataempty_t   m_bulks_is_empty;	            //�����ֵ�Ƿ�ΪOracle��nullֵ 0 - ok; -1 Ϊ null
                                                            
        static const int    m_TmpStrBufSize = 64;           
        char                m_TmpStrBuf[m_TmpStrBufSize];   //��ʱ���ת�����ֵ��ַ����Ļ�����

        conn_t		        *m_conn;		                //���������ݿ����Ӷ���
        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //�ͷ�ȫ������Դ
        void clear(void)
        {
            m_bulks_is_empty.clear();
            m_bulks_datasize.clear();
            m_bulks_databuff.clear();
            m_max_bulk_count = -1;
        }

        //-------------------------------------------------
        //������������ȷ�ϲ��������ݳ�ʼ��
        void m_init_data_type(const char *param_name, data_type_t type, int StringMaxSize, ub4 BulkCount)
        {
            rx_assert(!is_empty(param_name));
            rx_assert(m_max_bulk_count == (ub4)-1);
            rx_assert(BulkCount >= 1);

            char NamePreDateTypeChar = ' ';                   //Ĭ��ǰ׺��Ч
            if (param_name[0] == ':')
                NamePreDateTypeChar = param_name[1];          //��':'Ϊǰ���Ĳ��������Ž���ǰ׺���ͽ���

            if (type == DT_NUMBER || (type == DT_UNKNOWN && NamePreDateTypeChar == PP_NUMERIC))
            {
                m_dbc_data_type = DT_NUMBER;
                m_oci_data_type = SQLT_VNU;
                m_max_data_size = sizeof(OCINumber);
            }
            else if (type == DT_DATE || (type == DT_UNKNOWN && NamePreDateTypeChar == PP_DATE))
            {
                m_dbc_data_type = DT_DATE;
                m_oci_data_type = SQLT_ODT;
                m_max_data_size = sizeof(OCIDate);
            }
            else if (type == DT_TEXT || (type == DT_UNKNOWN && NamePreDateTypeChar == PP_TEXT))
            {
                m_dbc_data_type = DT_TEXT;
                m_oci_data_type = SQLT_STR;
                m_max_data_size = StringMaxSize;
            }
            else
                //�����͵�ǰ�����ܴ���
                throw (rx_dbc_ora::error_info_t(EC_BAD_PARAM_PREFIX, __FILE__, __LINE__, param_name));

            m_max_bulk_count = BulkCount;

            //������������ڴ�,����ʼ����
            m_bulks_databuff.make(m_max_data_size*BulkCount);
            m_bulks_datasize.make(BulkCount);
            m_bulks_is_empty.make(BulkCount);

            if (!m_bulks_is_empty.array() || !m_bulks_datasize.array() || !m_bulks_is_empty.array())
            {
                clear();
                throw (rx_dbc_ora::error_info_t(EC_NO_MEMORY, __FILE__, __LINE__));
            }

            for (ub4 i = 0; i < BulkCount; i++)
            {
                m_bulks_is_empty.at(i) = -1;
                m_bulks_datasize.at(i) = 0;
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

                sword result = OCIBindByName(StmtHandle, &bind_handle, conn.m_ErrHandle, (text *)name, (ub4)rx::st::strlen(name),
                    m_bulks_databuff.array(), m_max_data_size,
                    m_oci_data_type, m_bulks_is_empty.array(), m_bulks_datasize.array(),
                    NULL,	// pointer conn array of field_t-level return codes
                    0,		// maximum possible number of elements of type m_nType
                    NULL,	// a pointer conn the actual number of elements (PL/SQL binds)
                    OCI_DEFAULT);

                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, conn.m_ErrHandle, __FILE__, __LINE__, name));
            }
            catch (...)
            {
                clear();
                throw;
            }
        }

        //-------------------------------------------------
        //������ֵ
        sql_param_t& set_long(int32_t value, ub4 bulk_idx, bool is_signed)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
                sword result = OCINumberFromInt(m_conn->m_ErrHandle, &value, sizeof(int32_t), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED, reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty.at(bulk_idx) = 0;               //��ǲ����ǿ���
                m_bulks_datasize.at(bulk_idx) = m_max_data_size; //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[50];
                if (is_signed)
                    sprintf(TmpBuf, "%d", value);
                else
                    sprintf(TmpBuf, "%u", value);
                return set_string(TmpBuf, bulk_idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //�󶨴������븡����
        sql_param_t& set_double(double value, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
                sword result = OCINumberFromReal(m_conn->m_ErrHandle, &value, sizeof(double), reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty.at(bulk_idx) = 0;                  //��ǲ����ǿ���
                m_bulks_datasize.at(bulk_idx) = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[50];
                sprintf(TmpBuf, "%f", value);
                return set_string(TmpBuf, bulk_idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        sql_param_t& set_real(long double value, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
                sword result = OCINumberFromReal(m_conn->m_ErrHandle, &value, sizeof(value), reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty.at(bulk_idx) = 0;                  //��ǲ����ǿ���
                m_bulks_datasize.at(bulk_idx) = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[50];
                sprintf(TmpBuf, "%Lf", value);
                return set_string(TmpBuf, bulk_idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //����������
        sql_param_t& set_datetime(const datetime_t& d, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_DATE)
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
                d.to(*reinterpret_cast <OCIDate*> (DataBuf));
                m_bulks_is_empty.at(bulk_idx) = 0;               //��ǲ����ǿ���
                m_bulks_datasize.at(bulk_idx) = m_max_data_size; //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[21];
                d.to(TmpBuf);
                return set_string(TmpBuf, bulk_idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //���ı���
        sql_param_t& set_string(PStr text, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (is_empty(text))
            {//����ʲô����,�մ��������Ϊ��ֵ
                m_bulks_is_empty.at(bulk_idx) = -1;
                return *this;
            }
            else if (m_dbc_data_type == DT_TEXT)
            {//��ǰʵ�������������ı���,��ֵ��Ҳ���ı�,��ô�ͽ��п�����ֵ��
                ub2 DataLen = static_cast <ub2> (strlen(text));             //�õ�����ʵ�ʳ���
                ub1 *DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
                if (DataLen > m_max_data_size)                              //��������̫����,���нضϰ�
                {
                    rx_alert("�������ݹ���,���ڰ�ʱ�����������ߴ�");
                    DataLen = static_cast <ub2> ((m_max_data_size - 2) & ~1);
                }

                memcpy(DataBuf, text, DataLen);             //���������ݿ�������Ԫ�ض�Ӧ�Ŀռ�
                *((char *)DataBuf + DataLen++) = '\0';      //���ÿռ�Ĵ�β���ý�����

                m_bulks_is_empty.at(bulk_idx) = 0;               //��ǲ����ǿ���
                m_bulks_datasize.at(bulk_idx) = DataLen;         //��¼��Ԫ�ص�ʵ�ʳ���    
            }
            else if (m_dbc_data_type == DT_DATE)
            {//��ǰʵ����������������,������ֵ�����ı���,��ô�ͽ���ת�������;Ĭ��ֻ����"yyyy-mm-dd hh:mi:ss"�ĸ�ʽ
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))             //ת�����ڸ�ʽ
                    throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
                D.set(ST);
                return set_datetime(D, bulk_idx);                //����ʵ�ʵĹ��ܺ���
            }
            else if (m_dbc_data_type == DT_NUMBER)
            {//��ǰʵ����������������,������ֵ��ʱ�����ı���,��ô�ͽ���ת�������
                double Value = rx::st::atof(text);
                return set_double(Value, bulk_idx);              //����ʵ�ʵĹ��ܺ���
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
    public:
        //-------------------------------------------------
        sql_param_t(rx::mem_allotter_i &ma) :m_max_bulk_count(-1), m_bulks_datasize(ma), m_bulks_databuff(ma), m_bulks_is_empty(ma) {}
        //-------------------------------------------------
        ~sql_param_t() { clear(); }
        //-------------------------------------------------
        //�õ�ǰ��������Ϊ��ֵ
        void set_null(ub4 bulk_idx = 0) { rx_assert(bulk_idx < m_max_bulk_count); m_bulks_is_empty.at(bulk_idx) = -1; }
        bool is_null(uint32_t bulk_idx = 0) const { rx_assert(bulk_idx < m_max_bulk_count); return (m_bulks_is_empty.at(bulk_idx) == -1); }
        //-------------------------------------------------
        //��������ָ����������Ԫ�ظ�ֵ:�ַ���ֵ
        sql_param_t& operator()(PStr text, ub4 bulk_idx = 0) { return set_string(text,bulk_idx); }
        sql_param_t& operator = (PStr text) { return set_string(text, 0);}
        //-------------------------------------------------
        //���ò�����ָ����ŵ�����Ԫ��Ϊ������
        sql_param_t& operator () (double value, ub4 bulk_idx = 0) { return set_double(value,bulk_idx); }
        sql_param_t& operator = (double value) { return set_double(value,0); }
        sql_param_t& operator () (long double value, ub4 bulk_idx = 0) { return set_real(value, bulk_idx); }
        sql_param_t& operator = (long double value) { return set_real(value, 0); }
        //-------------------------------------------------
        //���ò���Ϊ������ֵ(������)
        sql_param_t& operator()(int64_t value, ub4 bulk_idx = 0) { return set_real((long double)value, bulk_idx); }
        sql_param_t& operator = (int64_t value) { return set_real((long double)value, 0); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ(������)
        sql_param_t& operator()(int32_t value, ub4 bulk_idx = 0) { return set_long(value,bulk_idx,true); }
        sql_param_t& operator = (int32_t value) { return set_long(value, 0, true); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ(�޷���)
        sql_param_t& operator()(uint32_t value, ub4 bulk_idx = 0) { return set_long(value, bulk_idx, false); }
        sql_param_t& operator = (uint32_t value) { return set_long(value, 0, false); }
        //-------------------------------------------------
        //���ò���Ϊ����ʱ��ֵ
        sql_param_t& operator()(const datetime_t& d, ub4 bulk_idx = 0) { return set_datetime(d,bulk_idx); }
        sql_param_t& operator = (const datetime_t& d) { return set_datetime(d, 0); }

        //-------------------------------------------------
        //�ӵ�ǰ�����еõ��ַ���ֵ
        PStr as_string(ub4 bulk_idx = 0, const char* ConvFmt = NULL) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return NULL;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����            
            return comm_as_string(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type, (char*)m_TmpStrBuf, m_TmpStrBufSize, ConvFmt);
        }
        //-------------------------------------------------
        //�ӵ�ǰ�����еõ�������ֵ
        double as_double(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
            return comm_as_double<double>(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        long double as_real(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
            return comm_as_double<long double>(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        //��ȡ������
        int64_t as_int(ub4 bulk_idx = 0) const { return int64_t(as_real(bulk_idx)); }
        //-------------------------------------------------
        //�ӵ�ǰ�����еõ�������ֵ
        int32_t as_long(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
            return comm_as_long(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        uint32_t as_ulong(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
            return comm_as_long(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type,false);
        }
        //-------------------------------------------------
        //�ӵ�ǰ�����еõ�ʱ����ֵ
        datetime_t as_datetime(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return datetime_t();
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //�õ����û�����
            return comm_as_datetime(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
    };
}

#endif
