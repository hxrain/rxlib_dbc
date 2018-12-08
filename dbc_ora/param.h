#ifndef	_RX_DBC_ORA_PARAMETER_H_
#define	_RX_DBC_ORA_PARAMETER_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //sql语句绑定参数,即可用于输入,也可用于输出
    //对于批量模式,参数对象对应着一个列的多行值,通过use_bulk()进行调整
    class sql_param_t:public col_base_t
    {
        friend class stmt_t;
        ub4                 m_max_bulk_deep;               //最大的批量数
        conn_t		        *m_conn;		                //关联的数据库连接对象
        ub2                 m_bulk_idx;                     //当前操作的行块序号

        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //释放全部的资源
        void m_clear(void)
        {
            col_base_t::reset();
            m_max_bulk_deep = 0;
            m_bulk_idx = 0;
        }
        //-------------------------------------------------
        //设置批量块指定深度的数据尺寸
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
        //进行数据类型确认并进行数据初始化
        //返回值:归一化后的OCI数据类型
        ub2 m_bind_data_type(const char *param_name,ub4 name_size, type_t::data_type_t type, int StringMaxSize, ub4 BulkCount)
        {
            rx_assert(!is_empty(param_name));
            rx_assert(m_max_bulk_deep == (ub4)0);
            rx_assert(BulkCount >= 1);
            m_max_bulk_deep = BulkCount;

            ub2 oci_data_type;
            type_t::data_type_t	dbc_data_type;
            int max_data_size;

            char NamePreDateTypeChar = type_t::DT_UNKNOWN;          //前缀类型默认为无效
            if (param_name[0] == ':')
                NamePreDateTypeChar = param_name[1];        //以':'为前导的参数命名才进行前缀类型解析

            //根据外面告知的绑定数据类型,进行内部数据类型转换
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
                //该类型当前还不能处理
                throw (error_info_t(type_t::DBEC_BAD_TYPEPREFIX, __FILE__, __LINE__, "param(%s)",param_name));


            //分配参数数据内存,并初始清零
            col_base_t::make(param_name, name_size, dbc_data_type, max_data_size, BulkCount, true);

            for (ub4 i = 0; i < BulkCount; i++)
                m_set_data_size(0,i);

            return oci_data_type;
        }

        //-------------------------------------------------
        //初始化绑定到对应的语句句柄上
        void bind_param(conn_t &conn, OCIStmt* StmtHandle, const char *name, type_t::data_type_t dbc_data_type, int StringMaxSize, int BulkCount)
        {
            rx_assert(!is_empty(name));
            m_clear();
            try
            {
                m_conn = &conn;
                ub4 name_size = (ub4)rx::st::strlen(name);
                //设置数据类型并进行归一化处理,分配参数使用的缓冲区
                ub2 oci_data_type= m_bind_data_type(name, name_size, dbc_data_type, StringMaxSize, BulkCount);

                //OCI绑定句柄,无需释放
                OCIBind	*bind_handle=NULL;                     

                //进行参数名字与缓冲区指针以及数据类型的绑定
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
        //绑定数字值
        sql_param_t& set_long(int32_t value, bool is_signed)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                OCINumber* data_buff = m_col_databuff.ptr<OCINumber>(m_bulk_idx);   //得到可用缓冲区
                sword result = OCINumberFromInt(m_conn->m_handle_err, &value, sizeof(int32_t), is_signed ? OCI_NUMBER_SIGNED : OCI_NUMBER_UNSIGNED,data_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)",m_name.c_str()));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //记录数据的实际长度
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
        //绑定大数字与浮点数
        sql_param_t& set_double(double value)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                OCINumber* data_buff = m_col_databuff.ptr<OCINumber>(m_bulk_idx);
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(value), data_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //记录数据的实际长度
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
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_NUMBER:
            {
                rx_assert(m_max_data_size == sizeof(OCINumber));
                OCINumber* data_buff = m_col_databuff.ptr<OCINumber>(m_bulk_idx);
                sword result = OCINumberFromReal(m_conn->m_handle_err, &value, sizeof(value), data_buff);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn->m_handle_err, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //记录数据的实际长度
                return (*this);
            }
            case type_t::DT_TEXT:
            {//将real数字转换为文本,使用OCI函数
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
        //绑定日期数据
        sql_param_t& set_datetime(const datetime_t& d)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            switch (m_dbc_data_type)
            {
            case type_t::DT_DATE:
            {
                rx_assert(m_max_data_size == sizeof(OCIDate));
                d.to(m_col_databuff.at<OCIDate>(m_bulk_idx));
                m_set_data_size(m_max_data_size, m_bulk_idx);   //记录数据的实际长度
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
        //绑定文本串
        sql_param_t& set_string(PStr text)
        {
            rx_assert_msg(m_bulk_idx < m_max_bulk_deep, "索引下标越界!已经使用bulk_bind_begin预先描述了吗?");
            if (is_empty(text))
            {//不管什么类型,空串都让其变为空值
                m_set_data_size(0, m_bulk_idx);   //记录数据的实际长度
                return *this;
            }

            switch (m_dbc_data_type)
            {
            case type_t::DT_TEXT:
            {//当前实际数据类型是文本串,赋值的也是文本,那么就进行拷贝赋值吧
                ub2 data_len = ub2(strlen(text));           //得到数据实际长度
                ub1 *data_buff = m_col_databuff.ptr(m_bulk_idx*m_max_data_size);   //得到可用缓冲区
                if (data_len > m_max_data_size)             //输入数据太长了,进行截断吧
                {
                    rx_alert("输入数据过大,请在绑定时调整缓冲区尺寸");
                    data_len = ub2((m_max_data_size - 2) & ~1);
                }

                memcpy(data_buff, text, data_len);          //将输入数据拷贝到此元素对应的空间
                *((char *)data_buff + data_len++) = '\0';   //给该空间的串尾设置结束符

                m_set_data_size(data_len, m_bulk_idx);      //记录数据的实际长度
                return (*this);
            }
            case type_t::DT_DATE:
            {//当前实际数据类型是日期,而给赋值的是文本串,那么就进行转换后处理吧;默认只处理"yyyy-mm-dd hh:mi:ss"的格式
                datetime_t D;
                struct tm ST;

                if (!rx_iso_datetime(text, ST))             //转换日期格式
                    throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
                D.set(ST);
                return set_datetime(D);                     //交给实际的功能函数
            }
            case type_t::DT_NUMBER:
            {//当前实际数据类型是数字,而给赋值的时候是文本串,那么就进行转换后处理吧
                double Value = rx::st::atof(text);
                return set_double(Value);                   //交给实际的功能函数
            }
            default:
                throw (error_info_t(type_t::DBEC_BAD_INPUT, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            }
        }
        //-------------------------------------------------
        //得到错误句柄
        OCIError* oci_err_handle() const { return m_conn->m_handle_err; }
        //得到当前的访问行号
        ub2 bulk_row_idx() const { return m_bulk_idx; }
        //-------------------------------------------------
        //设置块访问序号
        void bulk(ub2 idx)
        {
            if (idx >= m_max_bulk_deep)
                throw (error_info_t(type_t::DBEC_IDX_OVERSTEP, __FILE__, __LINE__, "param(%s)", m_name.c_str()));
            m_bulk_idx = idx;
        }
        //获取最大块深度
        ub2 bulks() { return m_max_bulk_deep; }
    public:
        //-------------------------------------------------
        sql_param_t(rx::mem_allotter_i &ma) :col_base_t(ma) { m_clear(); }
        ~sql_param_t() { m_clear(); }
        //-------------------------------------------------
        //让当前参数设置为空值
        void set_null() { rx_assert(bulk_row_idx() < m_max_bulk_deep); m_set_data_size(0, bulk_row_idx()); }
        bool is_null() const { rx_assert(bulk_row_idx() < m_max_bulk_deep); return col_base_t::m_is_null(bulk_row_idx()); }
        //-------------------------------------------------
        //给参数的指定数组批量元素赋值:字符串值
        sql_param_t& operator = (PStr text) { return set_string(text);}
        //-------------------------------------------------
        //设置参数中指定序号的批量元素为浮点数
        sql_param_t& operator = (double value) { return set_double(value); }
        sql_param_t& operator = (long double value) { return set_real(value); }
        //-------------------------------------------------
        //设置参数为大整数值(带符号)
        sql_param_t& operator = (int64_t value) { return set_real((long double)value); }
        //-------------------------------------------------
        //设置参数为整数值(带符号)
        sql_param_t& operator = (int32_t value) { return set_long(value, true); }
        //-------------------------------------------------
        //设置参数为整数值(无符号)
        sql_param_t& operator = (uint32_t value) { return set_long(value, false); }
        //-------------------------------------------------
        //设置参数为日期时间值
        sql_param_t& operator = (const datetime_t& d) { return set_datetime(d); }
    };
}

#endif
