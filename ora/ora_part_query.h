#ifndef	_RX_DBC_ORA_QUERY_H_
#define	_RX_DBC_ORA_QUERY_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //��stmt_t�༯��,�����˽����������ȡ������
    //-----------------------------------------------------
    class query_t:protected stmt_t
    {
        friend class field_t;

        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t   m_fields;                           //�ֶ�����
        ub2				m_bat_fetch_count;	                //ÿ���λ�ȡ�ļ�¼����
        ub4				m_fetched_count;	                //�Ѿ���ȡ���ļ�¼����
        ub4				m_cur_row_idx;	                    //��ǰ����ļ�¼��ȫ��������е��к�
        ub2  			m_is_eof;			                //��ǰ��¼���Ƿ��Ѿ���ȫ��ȡ���

        //-------------------------------------------------
        //����ֹ�Ĳ���
        query_t (const query_t&);
        query_t& operator = (const query_t&);
        //-------------------------------------------------
        //����������ʹ�õ�״̬�������Դ
        void m_clear (void)
        {
            m_bat_fetch_count = 0;
            m_fetched_count = 0;
            m_cur_row_idx = 0;
            m_is_eof = false;
            m_fields.clear();
        }

        //-------------------------------------------------
        //�õ��ֶ��б��������Ϣ
        void m_make_fields (ub2 fetch_size)
        {
            m_bat_fetch_count = fetch_size;
            //��ȡ��ǰ��������ֶ�����
            ub4			count;
            sword result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &count, NULL, OCI_ATTR_PARAM_COUNT, m_conn.m_ErrHandle);
            if (result != OCI_SUCCESS)
                throw (rx_dbc_ora::error_info_t(result, m_conn.m_ErrHandle, __FILE__, __LINE__));

            //��̬�����ֶζ�������
            rx_assert(m_fields.capacity()==0);
            if (!m_fields.make_ex(count))
                throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));

            //ѭ����ȡ�ֶ�������Ϣ
            char Tmp[200];
            for (ub4 i=0; i<count; i++)
            {
                OCIParam	*param_handle = NULL;
                text		*param_name = NULL;
                ub4			name_len = 0;
                ub2			oci_data_type = 0;
                ub4			size = 0;

                //�ӽ�����л�ȡ�������
                result = OCIParamGet (m_stmt_handle,OCI_HTYPE_STMT,m_conn.m_ErrHandle,reinterpret_cast <void **> (&param_handle),i + 1);	// first is 1

                if (result == OCI_SUCCESS)                          //���ݲ�������õ��ֶ�����
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&param_name,&name_len,OCI_ATTR_NAME,m_conn.m_ErrHandle);

                if (result == OCI_SUCCESS)                          //���ݲ�������õ�ORACLE��������
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&oci_data_type,NULL,OCI_ATTR_DATA_TYPE,m_conn.m_ErrHandle);

                if (result == OCI_SUCCESS)                          //���ݲ�������õ����ֶε��������ߴ�
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&size,NULL,OCI_ATTR_DATA_SIZE,m_conn.m_ErrHandle);

                if (param_handle)                                   //�ͷŲ������
                    OCIDescriptorFree (param_handle,OCI_DTYPE_PARAM);

                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t (result, m_conn.m_ErrHandle, __FILE__, __LINE__));
                    
                rx::st::strncpy(Tmp,(char*)param_name,name_len);    //ת���ֶ���,���ֶζ����������ֽ��й���
                Tmp[name_len]=0;

                rx_assert(m_fields.size()==i);
                //�󶨲���ʼ���ֶζ���
                field_t	&Field = m_fields[i];
                m_fields.bind(i,rx::st::strlwr(Tmp));
                Field.bind_data_type (this,reinterpret_cast <const char *> (param_name),name_len,oci_data_type,size,m_bat_fetch_count);
            }

            //ѭ�������ֶ�ֵ�������İ�
            ub4 position = 1;
            for (ub4 i = 0; i<m_fields.size(); i++)
            {
                field_t& Field = m_fields[i];
                result = OCIDefineByPos(m_stmt_handle, &(Field.m_field_handle), m_conn.m_ErrHandle,
                    position++,
                    Field.m_fields_databuff.array(),
                    Field.m_max_data_size,			    // fetch m_max_data_size for a single row (NOT for several)
                    Field.m_oci_data_type,
                    Field.m_fields_is_empty.array(),
                    Field.m_fields_datasize.array(),	// will be NULL for non-text columns
                    NULL,				                // ptr to array of field_t-level return codes
                    OCI_DEFAULT);

                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn.m_ErrHandle, __FILE__, __LINE__, Field.m_FieldName.c_str()));
            }
        }

        //-------------------------------------------------
        //������ȡ��¼,�ȴ�����
        void m_bat_fetch (void)
        {
            sword	result;
            ub4		old_rows_count = m_fetched_count;

            result = OCIStmtFetch(m_stmt_handle,m_conn.m_ErrHandle,m_bat_fetch_count,OCI_FETCH_NEXT,OCI_DEFAULT);
            if (result == OCI_SUCCESS || result == OCI_NO_DATA || result == OCI_SUCCESS_WITH_INFO)
            {
                result = OCIAttrGet (m_stmt_handle,OCI_HTYPE_STMT,&m_fetched_count,NULL,OCI_ATTR_ROW_COUNT,m_conn.m_ErrHandle);
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t (result, m_conn.m_ErrHandle, __FILE__, __LINE__));
                
                if (m_fetched_count - old_rows_count != (ub4)m_bat_fetch_count)
                    m_is_eof = true;
            }
            else
                throw (rx_dbc_ora::error_info_t (result, m_conn.m_ErrHandle, __FILE__, __LINE__));
        }

    public:
        //-------------------------------------------------
        query_t(conn_t &Conn) :m_fields(Conn.m_MemPool),stmt_t(Conn) { m_clear(); }
        ~query_t (){close();}
        //-------------------------------------------------
        conn_t& conn(){return stmt_t::conn();}
        PStr SQL(){return stmt_t::SQL();}
        //-------------------------------------------------
        //Ԥ����һ��SQL���,�õ���Ҫ����Ϣ
        void prepare(const char *SQL, int Len = -1) { stmt_t::prepare(SQL, Len); }
        //-------------------------------------------------
        //Ԥ����֮����Խ��в�����
        sql_param_t& bind(const char *name, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES) { return stmt_t::bind(name,type, MaxStringSize); }
        //-------------------------------------------------
        //��ȡ�󶨵Ĳ�������
        sql_param_t& param(const char* Name) { return stmt_t::param(Name); }
        //-------------------------------------------------
        //ִ�н������SQL���,�����Եõ����(���Ϊÿ�λ�ȡ����������)
        //ִ�к����û���쳣,�Ϳ��Գ��Է��ʽ������
        void exec(ub2 fetch_size=BAT_FETCH_SIZE)
        {
            m_clear();
            stmt_t::exec();
            m_make_fields(fetch_size);
            m_bat_fetch();
        }
        //-------------------------------------------------
        //ֱ��ִ��һ��SQL���,�м�û�а󶨲����Ļ�����
        void exec(const char *SQL,int Len = -1,ub2 fetch_size=BAT_FETCH_SIZE)
        {
            prepare(SQL,Len);
            exec(fetch_size);
        }
        //-------------------------------------------------
        //�رյ�ǰ��Query����,�ͷ�ȫ����Դ
        void close()
        {
            m_clear();
            stmt_t::close();
        }
        //-------------------------------------------------
        //�жϵ�ǰ���Ƿ���������(exec/eof/next�����˽��������ԭ��)
        bool eof(void) const { return (m_cur_row_idx >= m_fetched_count && m_is_eof); }
        //-------------------------------------------------
        //��ת����һ��,����ֵΪfalse˵������β��
        //����ֵ:�Ƿ��м�¼
        bool next(void)
        {
            m_cur_row_idx++;
            if (m_cur_row_idx >= m_fetched_count)
            {
                if (m_is_eof) return false;
                m_bat_fetch();
            }
            if (m_cur_row_idx >= m_fetched_count)
                return false;

            return true;
        }
        //-------------------------------------------------
        //�õ���ǰ��������ֶε�����
        ub4 fields()
        {
            rx_assert(m_fields.size()==m_fields.capacity());
            return m_fields.size();
        }
        //-------------------------------------------------
        //������ŷ����ֶ�
        field_t& operator [] (const ub4 field_idx)
        {
            rx_assert(field_idx<m_fields.size());
            return m_fields[field_idx];
        }
        //-------------------------------------------------
        //�����ֶ������һ��ж��ֶ��Ƿ����
        field_t* field(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity()) return NULL;
            return &m_fields[field_idx];
        }
        //-------------------------------------------------
        //�����ֶ�������
        field_t& operator [] (const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity())
                throw (rx_dbc_ora::error_info_t (EC_COLUMN_NOT_FOUND, __FILE__, __LINE__, name));
            return m_fields[field_idx];
        }
    };

    //-----------------------------------------------------
    //����field_t��Ҫ����query_t�е���Ϣ,���Խ����������
    inline ub2 field_t::rel_row_idx()const
    {
        rx_assert (m_query!=NULL);
        return static_cast <ub2> (m_query->m_cur_row_idx % m_query->m_bat_fetch_count);
    }
}

#endif
