#ifndef	_RX_DBC_ORA_FIELD_H_
#define	_RX_DBC_ORA_FIELD_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //结果集使用的字段访问对象
    class field_t
    {
        friend class query_t;
        typedef rx::array_t<ub2> array_datasize_t;
        typedef rx::array_t<ub1> array_databuff_t;
        typedef rx::array_t<sb2> array_dataempty_t;
        typedef rx::tiny_string_t<char, FIELD_NAME_LENGTH> field_name_t;
        field_name_t    m_FieldName;		                // 字段名字
        data_type_t	    m_dbc_data_type;		            // 期待的数据类型
        ub2				m_oci_data_type;		            // Oracle实际数据类型,二者用于自动转换
        int				m_max_data_size;			        // 每个字段数据最大尺寸

        static const int m_TmpStrBufSize = 64;
        char            m_TmpStrBuf[m_TmpStrBufSize];       // 临时存放转换数字到字符串的缓冲区

        array_datasize_t m_fields_datasize;		            // 文本字段没行的长度数组
        array_databuff_t m_fields_databuff;	                // 记录该字段的每行的实际数据的数组
        array_dataempty_t m_fields_is_empty;	            // 标记该字段的值是否为空的状态数组: 0 - ok; -1 - null

        OCIDefine		*m_field_handle;	                // 字段定义句柄
        query_t		    *m_query;	                        // 该字段的实际拥有者Query对象指针

        //-------------------------------------------------
        //字段构造函数,只能被记录集类使用
        void bind_data_type(query_t *rs, const char *name, ub4 name_len, ub2 oci_data_type, ub4 max_data_size, int fetch_size = FETCH_SIZE)
        {
            rx_assert(rs && !is_empty(name));
            reset();

            m_FieldName.set(name);
            m_oci_data_type = oci_data_type;

            // decide the format we want oci to return data in (m_oci_data_type member)
            switch (oci_data_type)
            {
            case	SQLT_INT:	// integer
            case	SQLT_LNG:	// long
            case	SQLT_UIN:	// unsigned int

            case	SQLT_NUM:	// numeric
            case	SQLT_FLT:	// float
            case	SQLT_VNU:	// numeric with length
            case	SQLT_PDN:	// packed decimal
                m_oci_data_type = SQLT_VNU;
                m_dbc_data_type = DT_NUMBER;
                m_max_data_size = sizeof(OCINumber);
                break;

            case	SQLT_DAT:	// date
            case	SQLT_ODT:	// oci date - should not appear?
                m_oci_data_type = SQLT_ODT;
                m_dbc_data_type = DT_DATE;
                m_max_data_size = sizeof(OCIDate);
                break;

            case	SQLT_CHR:	// character string
            case	SQLT_STR:	// zero-terminated string
            case	SQLT_VCS:	// variable-character string
            case	SQLT_AFC:	// ansi fixed char
            case	SQLT_AVC:	// ansi var char
            case	SQLT_VST:	// oci string type
                m_oci_data_type = SQLT_STR;
                m_dbc_data_type = DT_TEXT;
                m_max_data_size = (max_data_size + 1) * CHAR_SIZE; // + 1 for terminating zero!
                break;

            default:
                throw (rx_dbc_ora::error_info_t(EC_UNSUP_ORA_TYPE, __FILE__, __LINE__, name));
            }

            m_query = rs;

            //只有文本串才需要字段长度数组
            if (m_dbc_data_type == DT_TEXT)
            {
                if (!m_fields_datasize.make(fetch_size))
                {
                    reset();
                    throw (rx_dbc_ora::error_info_t(EC_NO_MEMORY, __FILE__, __LINE__, name));
                }
            }

            //生成字段数据缓冲区
            m_fields_databuff.make(m_max_data_size * fetch_size);
            //生成字段是否为空的数组
            m_fields_is_empty.make(fetch_size);
            if (!m_fields_is_empty.array() || !m_fields_databuff.array())
            {//判断是否内存不足
                reset();
                throw (rx_dbc_ora::error_info_t(EC_NO_MEMORY, __FILE__, __LINE__, name));
            }
        }
        //-------------------------------------------------
        //得到错误句柄
        OCIError *oci_err_handle() const
        {
            conn_t &conn = reinterpret_cast<stmt_t*>(m_query)->m_conn;
            return conn.m_ErrHandle;
        }
        //-------------------------------------------------
        void reset()
        {
            if (!m_query) return;
            m_fields_datasize.clear();
            m_fields_databuff.clear();
            m_fields_is_empty.clear();
            m_field_handle = NULL;
            m_query = NULL;
        }
        //-------------------------------------------------
        //得到当前的相对行号
        ub2 rel_row_idx()const;
        //-------------------------------------------------
        field_t(const field_t&);
        field_t& operator = (const field_t&);

    public:
        field_t(rx::mem_allotter_i &ma):m_fields_datasize(ma), m_fields_databuff(ma), m_fields_is_empty(ma)
        {
            m_dbc_data_type = DT_UNKNOWN;
            m_oci_data_type = 0;
            m_max_data_size = 0;
            m_field_handle = NULL;
            m_query = NULL;
        }
        //-------------------------------------------------
        ~field_t() { reset(); }
        //-------------------------------------------------
        const char* Name()const { return m_FieldName.c_str(); }
        data_type_t dbc_data_type() { return m_dbc_data_type; }
        int max_data_size() { return m_max_data_size; }
        ub2 oci_data_type() { return m_oci_data_type; }
        //-------------------------------------------------
        bool is_null(void) const { return (m_fields_is_empty.at(rel_row_idx()) == -1); }
        //-------------------------------------------------
        PStr as_string(const char* ConvFmt = NULL) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty.at(row_no) == -1) return NULL;
            ub1 *DataBuf = comm_field_data_offset(m_fields_databuff.array(), m_dbc_data_type, row_no, m_max_data_size);
            return comm_as_string(oci_err_handle(), DataBuf, m_dbc_data_type, (char*)m_TmpStrBuf, m_TmpStrBufSize, ConvFmt);
        }
        //-------------------------------------------------
        double as_double(double DefValue = 0) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty.at(row_no) == -1) return DefValue;
            ub1 *DataBuf = comm_field_data_offset(m_fields_databuff.array(), m_dbc_data_type, row_no, m_max_data_size);
            return comm_as_double(oci_err_handle(), DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        long as_long(long DefValue = 0) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty.at(row_no) == -1) return DefValue;
            ub1 *DataBuf = comm_field_data_offset(m_fields_databuff.array(), m_dbc_data_type, row_no, m_max_data_size);
            return comm_as_long(oci_err_handle(), DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        datetime_t as_datetime(void) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty.at(row_no) == -1) return datetime_t();
            ub1 *DataBuf = comm_field_data_offset(m_fields_databuff.array(), m_dbc_data_type, row_no, m_max_data_size);
            return comm_as_datetime(oci_err_handle(), DataBuf, m_dbc_data_type);
        }
    };
}

#endif
