#ifndef	_RX_DBC_ORA_PARAMETER_H_
#define	_RX_DBC_ORA_PARAMETER_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //sql���󶨲���,������������,Ҳ���������
    //��������ģʽ,���������Ӧ��һ���еĶ���ֵ
    class sql_param_t
    {
        friend class command_t;

        String		    m_Name;		                        //����������
        data_type_t	    m_dbc_data_type;		            //�ڴ�����������
        ub2				m_oci_data_type;	                //OCI�ײ���������,���ߴ�������Զ�ת��
        ub2				m_max_data_size;	                //���ݵ����ߴ�
        ub4             m_max_bulk_count;                   //����������
        ub2				*m_bulks_datasize;		            //���ݵĳ���ָʾ
        ub1			    *m_bulks_databuff;	                //ģ���ά������ʵ����ݻ�����
        sb2				*m_bulks_is_empty;	                //�����ֵ�Ƿ�ΪOracle��nullֵ 0 - ok; -1 Ϊ null

        static const int m_TmpStrBufSize = 64;
        char            m_TmpStrBuf[m_TmpStrBufSize];       //��ʱ���ת�����ֵ��ַ����Ļ�����

        conn_t		    *m_conn;		                    //���������ݿ����Ӷ���
        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //�ͷ�ȫ������Դ
        void clear(void)
        {
            if (m_bulks_is_empty) m_conn->m_MemPool.free(m_bulks_is_empty), m_bulks_is_empty = NULL;
            if (m_bulks_datasize) m_conn->m_MemPool.free(m_bulks_datasize), m_bulks_datasize = NULL;
            if (m_bulks_databuff) m_conn->m_MemPool.free(m_bulks_databuff), m_bulks_databuff = NULL;
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
            m_bulks_databuff = (ub1*)m_conn->m_MemPool.alloc(m_max_data_size*BulkCount);
            m_bulks_datasize = (ub2*)m_conn->m_MemPool.alloc(sizeof(ub2)*BulkCount);
            m_bulks_is_empty = (sb2*)m_conn->m_MemPool.alloc(sizeof(sb2)*BulkCount);

            if (!m_bulks_is_empty || !m_bulks_datasize || !m_bulks_is_empty)
            {
                clear();
                throw (rx_dbc_ora::error_info_t(EC_NO_MEMORY, __FILE__, __LINE__));
            }

            for (ub4 i = 0; i < BulkCount; i++)
            {
                m_bulks_is_empty[i] = -1;
                m_bulks_datasize[i] = 0;
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
                m_Name = name;

                //���Ը��ݲ�����ǰ׺���⴦�������������,���û����ȷ���ò������͵Ļ�
                m_init_data_type(name, type, StringMaxSize, BulkCount);

                //OCI�󶨾��,�����ͷ�
                OCIBind	*bind_handle=NULL;                     

                sword result = OCIBindByName(StmtHandle, &bind_handle, conn.m_ErrHandle, (text *)m_Name.c_str(), (ub4)m_Name.size(),
                    m_bulks_databuff, m_max_data_size, 
                    m_oci_data_type, m_bulks_is_empty, m_bulks_datasize,
                    NULL,	// pointer conn array of field_t-level return codes
                    0,		// maximum possible number of elements of type m_nType
                    NULL,	// a pointer conn the actual number of elements (PL/SQL binds)
                    OCI_DEFAULT);

                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, conn.m_ErrHandle, __FILE__, __LINE__, m_Name.c_str()));
            }
            catch (...)
            {
                clear();
                throw;
            }
        }
    public:
        //-------------------------------------------------
        sql_param_t()
        {
            m_max_bulk_count = -1;
            m_bulks_is_empty = NULL;
            m_bulks_datasize = NULL;
            m_bulks_databuff = NULL;
        }
        //-------------------------------------------------
        ~sql_param_t() { clear(); }
        //-------------------------------------------------
        //�õ�ǰ��������Ϊ��ֵ
        void set_null(ub4 Idx = 0) { rx_assert(Idx < m_max_bulk_count); m_bulks_is_empty[Idx] = -1; }
        //-------------------------------------------------
        //�жϵ�ǰ�����Ƿ�Ϊ��ֵ
        bool is_null(ub4 Idx = 0) const { rx_assert(Idx < m_max_bulk_count); return (m_bulks_is_empty[Idx] == -1); }
        //-------------------------------------------------
        //��������ָ����������Ԫ�ظ�ֵ:�ַ���ֵ
        sql_param_t& operator()(PStr text, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (is_empty(text))
            {//����ʲô����,�մ��������Ϊ��ֵ
                m_bulks_is_empty[Idx] = -1;
                return *this;
            }
            else if (m_dbc_data_type == DT_TEXT)
            {//��ǰʵ�������������ı���,��ֵ��Ҳ���ı�,��ô�ͽ��п�����ֵ��
                ub2 DataLen = static_cast <ub2> (strlen(text));        //�õ�����ʵ�ʳ���
                ub1 *DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
                if (DataLen > m_max_data_size)                             //��������̫����,���нضϰ�
                {
                    rx_alert("�������ݹ���,���ڰ�ʱ�����������ߴ�");
                    DataLen = static_cast <ub2> ((m_max_data_size - 2) & ~1);
                }

                memcpy(DataBuf, text, DataLen);            //���������ݿ�������Ԫ�ض�Ӧ�Ŀռ�
                *((char *)DataBuf + DataLen++) = '\0';     //���ÿռ�Ĵ�β���ý�����

                m_bulks_is_empty[Idx] = 0;             //��ǲ����ǿ���
                m_bulks_datasize[Idx] = DataLen;           //��¼��Ԫ�ص�ʵ�ʳ���    
            }
            else if (m_dbc_data_type == DT_DATE)
            {//��ǰʵ����������������,������ֵ�����ı���,��ô�ͽ���ת�������;Ĭ��ֻ����"yyyy-mm-dd hh:mi:ss"�ĸ�ʽ
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))              //ת�����ڸ�ʽ
                    throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
                D.set(ST);
                return operator()(D, Idx);                   //����ʵ�ʵĹ��ܺ���
            }
            else if (m_dbc_data_type == DT_NUMBER)
            {//��ǰʵ����������������,������ֵ��ʱ�����ı���,��ô�ͽ���ת�������
                double Value = rx::st::atof(text);
                return operator()(Value, Idx);               //����ʵ�ʵĹ��ܺ���
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //�����������ַ���ֵ
        sql_param_t& operator = (PStr text) { return operator()(text); }
        //-------------------------------------------------
        //���ò�����ָ����ŵ�����Ԫ��Ϊ������
        sql_param_t& operator () (double value, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
                sword result = OCINumberFromReal(m_conn->m_ErrHandle, &value, sizeof(double), reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty[Idx] = 0;                  //��ǲ����ǿ���
                m_bulks_datasize[Idx] = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[50];
                sprintf(TmpBuf, "%.02f", value);
                return operator()(TmpBuf, Idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //���ò���Ϊ������
        sql_param_t& operator = (double value) { return operator()(value); }
        //-------------------------------------------------
        //���ò���Ϊ����ֵ
        sql_param_t& operator()(long value, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
                sword result = OCINumberFromInt(m_conn->m_ErrHandle, &value, sizeof(long), OCI_NUMBER_SIGNED, reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty[Idx] = 0;             //��ǲ����ǿ���
                m_bulks_datasize[Idx] = m_max_data_size;      //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[50];
                sprintf(TmpBuf, "%d", value);
                return operator()(TmpBuf, Idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //���ò���Ϊ������
        sql_param_t& operator = (long value) { return operator()(value); }
        //-------------------------------------------------
        sql_param_t& operator()(const datetime_t& d, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_dbc_data_type == DT_DATE)
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
                d.to(*reinterpret_cast <OCIDate*> (DataBuf));
                m_bulks_is_empty[Idx] = 0;                  //��ǲ����ǿ���
                m_bulks_datasize[Idx] = m_max_data_size;    //��¼���ݵ�ʵ�ʳ���
            }
            else if (m_dbc_data_type == DT_TEXT)
            {
                char TmpBuf[21];
                d.to(TmpBuf);
                return operator()(TmpBuf, Idx);
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //���ò���Ϊ����ʱ��ֵ
        sql_param_t& operator = (const datetime_t& d) { return operator()(d); }

        //-------------------------------------------------
        //�ӵ�ǰ�����еõ��ַ���ֵ
        PStr as_string(ub4 Idx = 0, const char* ConvFmt = NULL) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty[Idx] == -1) return NULL;
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����            
            return CommAsString(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type, (char*)m_TmpStrBuf, m_TmpStrBufSize, ConvFmt);
        }
        //-------------------------------------------------
        //�ӵ�ǰ�����еõ�������ֵ
        double as_double(ub4 Idx = 0) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty[Idx] == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
            return CommAsDouble(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        //�ӵ�ǰ�����еõ�������ֵ
        long as_long(ub4 Idx = 0) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty[Idx] == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
            return CommAsLong(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        //�ӵ�ǰ�����еõ�ʱ����ֵ
        datetime_t as_datetime(ub4 Idx = 0) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "�����±�Խ��!�Ѿ�ʹ��bulk_bind_beginԤ����������?");
            if (m_bulks_is_empty[Idx] == -1) return datetime_t();
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //�õ����û�����
            return CommAsDateTime(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
    };
}

#endif
