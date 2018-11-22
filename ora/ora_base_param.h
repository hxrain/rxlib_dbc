#ifndef	_RX_DBC_ORA_PARAMETER_H_
#define	_RX_DBC_ORA_PARAMETER_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //sql语句绑定参数,即可用于输入,也可用于输出
    //对于批量模式,参数对象对应着一个列的多行值
    class sql_param_t
    {
        friend class stmt_t;
        typedef rx::array_t<ub2> array_datasize_t;
        typedef rx::array_t<ub1> array_databuff_t;
        typedef rx::array_t<sb2> array_dataempty_t;

        data_type_t	        m_dbc_data_type;		        //期待的数据类型
        ub2				    m_oci_data_type;	            //OCI底层数据类型,二者搭配进行自动转换
        ub2				    m_max_data_size;	            //数据的最大尺寸
        ub4                 m_max_bulk_count;               //最大的批量数
        array_datasize_t    m_bulks_datasize;		        //数据的长度指示
        array_databuff_t    m_bulks_databuff;	            //模拟二维数组访问的数据缓冲区
        array_dataempty_t   m_bulks_is_empty;	            //标记数值是否为Oracle的null值 0 - ok; -1 为 null
                                                            
        static const int    m_TmpStrBufSize = 64;           
        char                m_TmpStrBuf[m_TmpStrBufSize];   //临时存放转换数字到字符串的缓冲区

        conn_t		        *m_conn;		                //关联的数据库连接对象
        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //释放全部的资源
        void clear(void)
        {
            m_bulks_is_empty.clear();
            m_bulks_datasize.clear();
            m_bulks_databuff.clear();
            m_max_bulk_count = -1;
        }

        //-------------------------------------------------
        //进行数据类型确认并进行数据初始化
        void m_init_data_type(const char *param_name, data_type_t type, int StringMaxSize, ub4 BulkCount)
        {
            rx_assert(!is_empty(param_name));
            rx_assert(m_max_bulk_count == (ub4)-1);
            rx_assert(BulkCount >= 1);

            char NamePreDateTypeChar = ' ';                   //默认前缀无效
            if (param_name[0] == ':')
                NamePreDateTypeChar = param_name[1];          //以':'为前导的参数命名才进行前缀类型解析

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
                //该类型当前还不能处理
                throw (rx_dbc_ora::error_info_t(EC_BAD_PARAM_PREFIX, __FILE__, __LINE__, param_name));

            m_max_bulk_count = BulkCount;

            //分配参数数据内存,并初始清零
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
        //初始化绑定到对应的语句句柄上
        void bind(conn_t &conn, OCIStmt* StmtHandle, const char *name, data_type_t type, int StringMaxSize, int BulkCount)
        {
            rx_assert(!is_empty(name));
            clear();
            try
            {
                m_conn = &conn;

                //可以根据参数名前缀额外处理参数数据类型,如果没有明确设置参数类型的话
                m_init_data_type(name, type, StringMaxSize, BulkCount);

                //OCI绑定句柄,无需释放
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
        //绑定数字值
        sql_param_t& set_long(int32_t value, ub4 bulk_idx, bool is_signed)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
                sword result = OCINumberFromInt(m_conn->m_ErrHandle, &value, sizeof(int32_t), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED, reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty.at(bulk_idx) = 0;               //标记参数非空了
                m_bulks_datasize.at(bulk_idx) = m_max_data_size; //记录数据的实际长度
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
        //绑定大数字与浮点数
        sql_param_t& set_double(double value, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
                sword result = OCINumberFromReal(m_conn->m_ErrHandle, &value, sizeof(double), reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty.at(bulk_idx) = 0;                  //标记参数非空了
                m_bulks_datasize.at(bulk_idx) = m_max_data_size;    //记录数据的实际长度
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
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
                sword result = OCINumberFromReal(m_conn->m_ErrHandle, &value, sizeof(value), reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty.at(bulk_idx) = 0;                  //标记参数非空了
                m_bulks_datasize.at(bulk_idx) = m_max_data_size;    //记录数据的实际长度
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
        //绑定日期数据
        sql_param_t& set_datetime(const datetime_t& d, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_DATE)
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
                d.to(*reinterpret_cast <OCIDate*> (DataBuf));
                m_bulks_is_empty.at(bulk_idx) = 0;               //标记参数非空了
                m_bulks_datasize.at(bulk_idx) = m_max_data_size; //记录数据的实际长度
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
        //绑定文本串
        sql_param_t& set_string(PStr text, ub4 bulk_idx = 0)
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (is_empty(text))
            {//不管什么类型,空串都让其变为空值
                m_bulks_is_empty.at(bulk_idx) = -1;
                return *this;
            }
            else if (m_dbc_data_type == DT_TEXT)
            {//当前实际数据类型是文本串,赋值的也是文本,那么就进行拷贝赋值吧
                ub2 DataLen = static_cast <ub2> (strlen(text));             //得到数据实际长度
                ub1 *DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
                if (DataLen > m_max_data_size)                              //输入数据太长了,进行截断吧
                {
                    rx_alert("输入数据过大,请在绑定时调整缓冲区尺寸");
                    DataLen = static_cast <ub2> ((m_max_data_size - 2) & ~1);
                }

                memcpy(DataBuf, text, DataLen);             //将输入数据拷贝到此元素对应的空间
                *((char *)DataBuf + DataLen++) = '\0';      //给该空间的串尾设置结束符

                m_bulks_is_empty.at(bulk_idx) = 0;               //标记参数非空了
                m_bulks_datasize.at(bulk_idx) = DataLen;         //记录该元素的实际长度    
            }
            else if (m_dbc_data_type == DT_DATE)
            {//当前实际数据类型是日期,而给赋值的是文本串,那么就进行转换后处理吧;默认只处理"yyyy-mm-dd hh:mi:ss"的格式
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))             //转换日期格式
                    throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
                D.set(ST);
                return set_datetime(D, bulk_idx);                //交给实际的功能函数
            }
            else if (m_dbc_data_type == DT_NUMBER)
            {//当前实际数据类型是数字,而给赋值的时候是文本串,那么就进行转换后处理吧
                double Value = rx::st::atof(text);
                return set_double(Value, bulk_idx);              //交给实际的功能函数
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
        //让当前参数设置为空值
        void set_null(ub4 bulk_idx = 0) { rx_assert(bulk_idx < m_max_bulk_count); m_bulks_is_empty.at(bulk_idx) = -1; }
        bool is_null(uint32_t bulk_idx = 0) const { rx_assert(bulk_idx < m_max_bulk_count); return (m_bulks_is_empty.at(bulk_idx) == -1); }
        //-------------------------------------------------
        //给参数的指定数组批量元素赋值:字符串值
        sql_param_t& operator()(PStr text, ub4 bulk_idx = 0) { return set_string(text,bulk_idx); }
        sql_param_t& operator = (PStr text) { return set_string(text, 0);}
        //-------------------------------------------------
        //设置参数中指定序号的批量元素为浮点数
        sql_param_t& operator () (double value, ub4 bulk_idx = 0) { return set_double(value,bulk_idx); }
        sql_param_t& operator = (double value) { return set_double(value,0); }
        sql_param_t& operator () (long double value, ub4 bulk_idx = 0) { return set_real(value, bulk_idx); }
        sql_param_t& operator = (long double value) { return set_real(value, 0); }
        //-------------------------------------------------
        //设置参数为大整数值(带符号)
        sql_param_t& operator()(int64_t value, ub4 bulk_idx = 0) { return set_real((long double)value, bulk_idx); }
        sql_param_t& operator = (int64_t value) { return set_real((long double)value, 0); }
        //-------------------------------------------------
        //设置参数为整数值(带符号)
        sql_param_t& operator()(int32_t value, ub4 bulk_idx = 0) { return set_long(value,bulk_idx,true); }
        sql_param_t& operator = (int32_t value) { return set_long(value, 0, true); }
        //-------------------------------------------------
        //设置参数为整数值(无符号)
        sql_param_t& operator()(uint32_t value, ub4 bulk_idx = 0) { return set_long(value, bulk_idx, false); }
        sql_param_t& operator = (uint32_t value) { return set_long(value, 0, false); }
        //-------------------------------------------------
        //设置参数为日期时间值
        sql_param_t& operator()(const datetime_t& d, ub4 bulk_idx = 0) { return set_datetime(d,bulk_idx); }
        sql_param_t& operator = (const datetime_t& d) { return set_datetime(d, 0); }

        //-------------------------------------------------
        //从当前参数中得到字符串值
        PStr as_string(ub4 bulk_idx = 0, const char* ConvFmt = NULL) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return NULL;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区            
            return comm_as_string(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type, (char*)m_TmpStrBuf, m_TmpStrBufSize, ConvFmt);
        }
        //-------------------------------------------------
        //从当前参数中得到浮点数值
        double as_double(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
            return comm_as_double<double>(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        long double as_real(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
            return comm_as_double<long double>(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        //获取大整数
        int64_t as_int(ub4 bulk_idx = 0) const { return int64_t(as_real(bulk_idx)); }
        //-------------------------------------------------
        //从当前参数中得到整型数值
        int32_t as_long(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
            return comm_as_long(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        uint32_t as_ulong(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
            return comm_as_long(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type,false);
        }
        //-------------------------------------------------
        //从当前参数中得到时间数值
        datetime_t as_datetime(ub4 bulk_idx = 0) const
        {
            rx_assert_msg(bulk_idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty.at(bulk_idx) == -1) return datetime_t();
            ub1* DataBuf = &m_bulks_databuff.at(bulk_idx*m_max_data_size);   //得到可用缓冲区
            return comm_as_datetime(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
    };
}

#endif
