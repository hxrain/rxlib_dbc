#ifndef	_RX_DBC_ORA_PARAMETER_H_
#define	_RX_DBC_ORA_PARAMETER_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //sql语句绑定参数,即可用于输入,也可用于输出
    //对于批量模式,参数对象对应着一个列的多行值
    class sql_param_t
    {
        friend class command_t;

        String		    m_Name;		                        //参数的名字
        data_type_t	    m_dbc_data_type;		            //期待的数据类型
        ub2				m_oci_data_type;	                //OCI底层数据类型,二者搭配进行自动转换
        ub2				m_max_data_size;	                //数据的最大尺寸
        ub4             m_max_bulk_count;                   //最大的批量数
        ub2				*m_bulks_datasize;		            //数据的长度指示
        ub1			    *m_bulks_databuff;	                //模拟二维数组访问的数据缓冲区
        sb2				*m_bulks_is_empty;	                //标记数值是否为Oracle的null值 0 - ok; -1 为 null

        static const int m_TmpStrBufSize = 64;
        char            m_TmpStrBuf[m_TmpStrBufSize];       //临时存放转换数字到字符串的缓冲区

        conn_t		    *m_conn;		                    //关联的数据库连接对象
        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //释放全部的资源
        void clear(void)
        {
            if (m_bulks_is_empty) m_conn->m_MemPool.free(m_bulks_is_empty), m_bulks_is_empty = NULL;
            if (m_bulks_datasize) m_conn->m_MemPool.free(m_bulks_datasize), m_bulks_datasize = NULL;
            if (m_bulks_databuff) m_conn->m_MemPool.free(m_bulks_databuff), m_bulks_databuff = NULL;
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
        //初始化绑定到对应的语句句柄上
        void bind(conn_t &conn, OCIStmt* StmtHandle, const char *name, data_type_t type, int StringMaxSize, int BulkCount)
        {
            rx_assert(!is_empty(name));
            clear();
            try
            {
                m_conn = &conn;
                m_Name = name;

                //可以根据参数名前缀额外处理参数数据类型,如果没有明确设置参数类型的话
                m_init_data_type(name, type, StringMaxSize, BulkCount);

                //OCI绑定句柄,无需释放
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
        //让当前参数设置为空值
        void set_null(ub4 Idx = 0) { rx_assert(Idx < m_max_bulk_count); m_bulks_is_empty[Idx] = -1; }
        //-------------------------------------------------
        //判断当前参数是否为空值
        bool is_null(ub4 Idx = 0) const { rx_assert(Idx < m_max_bulk_count); return (m_bulks_is_empty[Idx] == -1); }
        //-------------------------------------------------
        //给参数的指定数组批量元素赋值:字符串值
        sql_param_t& operator()(PStr text, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (is_empty(text))
            {//不管什么类型,空串都让其变为空值
                m_bulks_is_empty[Idx] = -1;
                return *this;
            }
            else if (m_dbc_data_type == DT_TEXT)
            {//当前实际数据类型是文本串,赋值的也是文本,那么就进行拷贝赋值吧
                ub2 DataLen = static_cast <ub2> (strlen(text));        //得到数据实际长度
                ub1 *DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
                if (DataLen > m_max_data_size)                             //输入数据太长了,进行截断吧
                {
                    rx_alert("输入数据过大,请在绑定时调整缓冲区尺寸");
                    DataLen = static_cast <ub2> ((m_max_data_size - 2) & ~1);
                }

                memcpy(DataBuf, text, DataLen);            //将输入数据拷贝到此元素对应的空间
                *((char *)DataBuf + DataLen++) = '\0';     //给该空间的串尾设置结束符

                m_bulks_is_empty[Idx] = 0;             //标记参数非空了
                m_bulks_datasize[Idx] = DataLen;           //记录该元素的实际长度    
            }
            else if (m_dbc_data_type == DT_DATE)
            {//当前实际数据类型是日期,而给赋值的是文本串,那么就进行转换后处理吧;默认只处理"yyyy-mm-dd hh:mi:ss"的格式
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))              //转换日期格式
                    throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
                D.set(ST);
                return operator()(D, Idx);                   //交给实际的功能函数
            }
            else if (m_dbc_data_type == DT_NUMBER)
            {//当前实际数据类型是数字,而给赋值的时候是文本串,那么就进行转换后处理吧
                double Value = rx::st::atof(text);
                return operator()(Value, Idx);               //交给实际的功能函数
            }
            else
                throw (rx_dbc_ora::error_info_t(EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
            return (*this);
        }
        //-------------------------------------------------
        //给参数设置字符串值
        sql_param_t& operator = (PStr text) { return operator()(text); }
        //-------------------------------------------------
        //设置参数中指定序号的批量元素为浮点数
        sql_param_t& operator () (double value, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
                sword result = OCINumberFromReal(m_conn->m_ErrHandle, &value, sizeof(double), reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty[Idx] = 0;                  //标记参数非空了
                m_bulks_datasize[Idx] = m_max_data_size;    //记录数据的实际长度
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
        //设置参数为浮点数
        sql_param_t& operator = (double value) { return operator()(value); }
        //-------------------------------------------------
        //设置参数为整数值
        sql_param_t& operator()(long value, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_NUMBER)
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
                sword result = OCINumberFromInt(m_conn->m_ErrHandle, &value, sizeof(long), OCI_NUMBER_SIGNED, reinterpret_cast <OCINumber *> (DataBuf));
                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t(result, m_conn->m_ErrHandle, __FILE__, __LINE__));
                m_bulks_is_empty[Idx] = 0;             //标记参数非空了
                m_bulks_datasize[Idx] = m_max_data_size;      //记录数据的实际长度
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
        //设置参数为整型数
        sql_param_t& operator = (long value) { return operator()(value); }
        //-------------------------------------------------
        sql_param_t& operator()(const datetime_t& d, ub4 Idx = 0)
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_dbc_data_type == DT_DATE)
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
                d.to(*reinterpret_cast <OCIDate*> (DataBuf));
                m_bulks_is_empty[Idx] = 0;                  //标记参数非空了
                m_bulks_datasize[Idx] = m_max_data_size;    //记录数据的实际长度
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
        //设置参数为日期时间值
        sql_param_t& operator = (const datetime_t& d) { return operator()(d); }

        //-------------------------------------------------
        //从当前参数中得到字符串值
        PStr as_string(ub4 Idx = 0, const char* ConvFmt = NULL) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty[Idx] == -1) return NULL;
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区            
            return CommAsString(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type, (char*)m_TmpStrBuf, m_TmpStrBufSize, ConvFmt);
        }
        //-------------------------------------------------
        //从当前参数中得到浮点数值
        double as_double(ub4 Idx = 0) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty[Idx] == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
            return CommAsDouble(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        //从当前参数中得到整型数值
        long as_long(ub4 Idx = 0) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty[Idx] == -1) return 0;
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
            return CommAsLong(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
        //-------------------------------------------------
        //从当前参数中得到时间数值
        datetime_t as_datetime(ub4 Idx = 0) const
        {
            rx_assert_msg(Idx < m_max_bulk_count, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (m_bulks_is_empty[Idx] == -1) return datetime_t();
            ub1* DataBuf = &m_bulks_databuff[Idx*m_max_data_size];   //得到可用缓冲区
            return CommAsDateTime(m_conn->m_ErrHandle, DataBuf, m_dbc_data_type);
        }
    };
}

#endif
