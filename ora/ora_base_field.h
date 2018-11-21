#ifndef	_RX_DBC_ORA_FIELD_H_
#define	_RX_DBC_ORA_FIELD_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //结果集使用的字段访问对象
    class field_t
    {
        friend class query_t;

        String		    m_FieldName;		                // 字段名字
        data_type_t	    m_dbc_data_type;		            // 期待的数据类型
        ub2				m_oci_data_type;		            // Oracle实际数据类型,二者用于自动转换
        int				m_max_data_size;			        // 每个字段数据最大尺寸

        static const int m_TmpStrBufSize = 64;
        char            m_TmpStrBuf[m_TmpStrBufSize];       // 临时存放转换数字到字符串的缓冲区

        sb2				*m_fields_is_empty;	                // 标记该字段的值是否为空的状态数组: 0 - ok; -1 - null
        ub2				*m_fields_datasize;		            // 文本字段没行的长度数组
        ub1			    *m_fields_databuff;	                // 记录该字段的每行的实际数据的数组

        OCIDefine		*m_field_handle;	                // 字段定义句柄
        query_t		    *m_query;	                        // 该字段的实际拥有者Query对象指针

        //-------------------------------------------------
        //字段构造函数,只能被记录集类使用
        void bind_data_type(query_t *rs, const char *name, ub4 name_len, ub2 oci_data_type, ub4 max_data_size, int fetch_size = FETCH_SIZE)
        {
            rx_assert(rs && !is_empty(name));
            reset();

            m_FieldName = name;
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
            conn_t &conn = reinterpret_cast<command_t*>(m_query)->m_Conn;

            m_fields_is_empty = (sb2*)conn.m_MemPool.alloc(sizeof(sb2)*fetch_size);

            if (m_dbc_data_type == DT_TEXT)
                m_fields_datasize = (ub2*)conn.m_MemPool.alloc(sizeof(ub2)*fetch_size);
            else
                m_fields_datasize = NULL;

            m_fields_databuff = (ub1*)conn.m_MemPool.alloc(sizeof(ub1)*m_max_data_size * fetch_size);

            m_field_handle = NULL;

            if (!m_fields_is_empty || !m_fields_databuff)
            {
                reset();
                throw (rx_dbc_ora::error_info_t(EC_NO_MEMORY, __FILE__, __LINE__));
            }
        }
        //-------------------------------------------------
        //得到错误句柄
        OCIError *oci_err_handle() const
        {
            conn_t &conn = reinterpret_cast<command_t*>(m_query)->m_Conn;
            return conn.m_ErrHandle;
        }
        //-------------------------------------------------
        void reset()
        {
            if (!m_query) return;
            conn_t &conn = reinterpret_cast<command_t*>(m_query)->m_Conn;
            if (m_fields_is_empty)
            {
                conn.m_MemPool.free(m_fields_is_empty);
                m_fields_is_empty = NULL;
            }
            if (m_fields_datasize)
            {
                conn.m_MemPool.free(m_fields_datasize);
                m_fields_datasize = NULL;
            }
            if (m_fields_databuff)
            {
                conn.m_MemPool.free(m_fields_databuff);
                m_fields_databuff = NULL;
            }
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
        field_t()
        {
            m_dbc_data_type = DT_UNKNOWN;
            m_oci_data_type = 0;
            m_max_data_size = 0;
            m_fields_is_empty = NULL;
            m_fields_datasize = NULL;
            m_fields_databuff = NULL;
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
        bool is_null(void) const { return (m_fields_is_empty[rel_row_idx()] == -1); }
        //-------------------------------------------------
        PStr as_string(const char* ConvFmt = NULL) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty[row_no] == -1) return NULL;
            ub1 *DataBuf = CommFieldRowDataOffset(m_fields_databuff, m_dbc_data_type, row_no, m_max_data_size);
            return CommAsString(oci_err_handle(), DataBuf, m_dbc_data_type, (char*)m_TmpStrBuf, m_TmpStrBufSize, ConvFmt);
        }
        //-------------------------------------------------
        double as_double(double DefValue = 0) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty[row_no] == -1) return DefValue;
            ub1 *DataBuf = CommFieldRowDataOffset(m_fields_databuff, m_dbc_data_type, row_no, m_max_data_size);
            return CommAsDouble(oci_err_handle(), DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        long as_long(long DefValue = 0) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty[row_no] == -1) return DefValue;
            ub1 *DataBuf = CommFieldRowDataOffset(m_fields_databuff, m_dbc_data_type, row_no, m_max_data_size);
            return CommAsLong(oci_err_handle(), DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        datetime_t as_datetime(void) const
        {
            ub2 row_no = rel_row_idx();
            if (m_fields_is_empty[row_no] == -1) return datetime_t();
            ub1 *DataBuf = CommFieldRowDataOffset(m_fields_databuff, m_dbc_data_type, row_no, m_max_data_size);
            return CommAsDateTime(oci_err_handle(), DataBuf, m_dbc_data_type);
        }
    };


}

#endif
