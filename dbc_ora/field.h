#ifndef	_RX_DBC_ORA_FIELD_H_
#define	_RX_DBC_ORA_FIELD_H_

namespace ora
{
    class query_t;
    //-----------------------------------------------------
    //�����ʹ�õ��ֶη��ʶ���
    class field_t:public col_base_t
    {
        friend class query_t;

        query_t		    *m_query;	                        // ���ֶε�ʵ��ӵ����Query����ָ��

        //-------------------------------------------------
        //�ֶι��캯��,ֻ�ܱ���¼����ʹ��
        //����ֵ:��һ��֮���OCI��������
        ub2 bind_data_type(query_t *rs, const char *name, ub4 name_len, ub2 oci_data_type, ub4 max_data_size, int fetch_size)
        {
            rx_assert(rs && !is_empty(name));
            data_type_t dbc_data_type = DT_UNKNOWN;
            m_query = rs;

            //����oci�������͵Ĺ�һ������,�õ�dbc�ڲ�����
            switch (oci_data_type)
            {
            case	SQLT_UIN:	// unsigned int
                oci_data_type = SQLT_VNU;
                dbc_data_type = DT_UINT;
                max_data_size = sizeof(OCINumber);
                break;

            case	SQLT_INT:	// integer
            case	SQLT_LNG:	// long
                oci_data_type = SQLT_VNU;
                dbc_data_type = DT_INT;
                max_data_size = sizeof(OCINumber);
                break;

            case	SQLT_NUM:	// numeric
                oci_data_type = SQLT_VNU;
                dbc_data_type = DT_LONG;
                max_data_size = sizeof(OCINumber);
                break;
            case	SQLT_FLT:	// float
            case	SQLT_VNU:	// numeric with length
            case	SQLT_PDN:	// packed decimal
                oci_data_type = SQLT_VNU;
                dbc_data_type = DT_FLOAT;
                max_data_size = sizeof(OCINumber);
                break;

            case	SQLT_DAT:	// date
            case	SQLT_ODT:	// oci date - should not appear?
                oci_data_type = SQLT_ODT;
                dbc_data_type = DT_DATE;
                max_data_size = sizeof(OCIDate);
                break;

            case	SQLT_CHR:	// character string
            case	SQLT_STR:	// zero-terminated string
            case	SQLT_VCS:	// variable-character string
            case	SQLT_AFC:	// ansi fixed char
            case	SQLT_AVC:	// ansi var char
            case	SQLT_VST:	// oci string type
                oci_data_type = SQLT_STR;
                dbc_data_type = DT_TEXT;
                max_data_size = (max_data_size + 1) * CHAR_SIZE; // + 1 for terminating zero!
                break;

            default:
                throw (error_info_t(DBEC_UNSUP_TYPE, __FILE__, __LINE__, name));
            }

            //�������յ����ݻ�����
            col_base_t::make(name, name_len, dbc_data_type, max_data_size, fetch_size, dbc_data_type == DT_TEXT);
            return oci_data_type;
        }
        //-------------------------------------------------
        //�õ�������
        OCIError *oci_err_handle() const
        {
            conn_t &conn = reinterpret_cast<stmt_t*>(m_query)->m_conn;
            return conn.m_handle_err;
        }
        //-------------------------------------------------
        void reset()
        {
            if (!m_query) return;
            col_base_t::reset();
            m_dbc_data_type = DT_UNKNOWN;
            m_max_data_size = 0;
            m_query = NULL;
        }
        //-------------------------------------------------
        //�õ���ǰ������к�
        ub2 bulk_row_idx()const;
        //-------------------------------------------------
        field_t(const field_t&);
        field_t& operator = (const field_t&);
    public:
        field_t(rx::mem_allotter_i &ma) :col_base_t(ma) { reset(); }
        //-------------------------------------------------
        ~field_t() { reset(); }
        //-------------------------------------------------
    };
}

#endif
