#ifndef	_RX_DBC_ORA_QUERY_H_
#define	_RX_DBC_ORA_QUERY_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //从stmt_t类集成,增加了结果集遍历获取的能力
    //-----------------------------------------------------
    class query_t:protected stmt_t
    {
        friend class field_t;

        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t   m_fields;                           //字段数组
        ub2				m_bat_fetch_count;	                //每批次获取的记录数量
        ub4				m_fetched_count;	                //已经获取过的记录数量
        ub4				m_cur_row_idx;	                    //当前处理的记录在全部结果集中的行号
        ub2  			m_is_eof;			                //当前记录集是否已经完全获取完成

        //-------------------------------------------------
        //被禁止的操作
        query_t (const query_t&);
        query_t& operator = (const query_t&);
        //-------------------------------------------------
        //清理本子类中使用的状态与相关资源
        void m_clear (void)
        {
            m_bat_fetch_count = 0;
            m_fetched_count = 0;
            m_cur_row_idx = 0;
            m_is_eof = false;
            m_fields.clear();
        }

        //-------------------------------------------------
        //得到字段列表的描述信息
        void m_make_fields (ub2 fetch_size)
        {
            m_bat_fetch_count = fetch_size;
            //获取当前结果集的字段数量
            ub4			count;
            sword result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &count, NULL, OCI_ATTR_PARAM_COUNT, m_conn.m_ErrHandle);
            if (result != OCI_SUCCESS)
                throw (rx_dbc_ora::error_info_t(result, m_conn.m_ErrHandle, __FILE__, __LINE__));

            //动态生成字段对象数组
            rx_assert(m_fields.capacity()==0);
            if (!m_fields.make_ex(count))
                throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));

            //循环获取字段属性信息
            char Tmp[200];
            for (ub4 i=0; i<count; i++)
            {
                OCIParam	*param_handle = NULL;
                text		*param_name = NULL;
                ub4			name_len = 0;
                ub2			oci_data_type = 0;
                ub4			size = 0;

                //从结果集中获取参数句柄
                result = OCIParamGet (m_stmt_handle,OCI_HTYPE_STMT,m_conn.m_ErrHandle,reinterpret_cast <void **> (&param_handle),i + 1);	// first is 1

                if (result == OCI_SUCCESS)                          //根据参数句柄得到字段名字
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&param_name,&name_len,OCI_ATTR_NAME,m_conn.m_ErrHandle);

                if (result == OCI_SUCCESS)                          //根据参数句柄得到ORACLE数据类型
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&oci_data_type,NULL,OCI_ATTR_DATA_TYPE,m_conn.m_ErrHandle);

                if (result == OCI_SUCCESS)                          //根据参数句柄得到该字段的数据最大尺寸
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&size,NULL,OCI_ATTR_DATA_SIZE,m_conn.m_ErrHandle);

                if (param_handle)                                   //释放参数句柄
                    OCIDescriptorFree (param_handle,OCI_DTYPE_PARAM);

                if (result != OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t (result, m_conn.m_ErrHandle, __FILE__, __LINE__));
                    
                rx::st::strncpy(Tmp,(char*)param_name,name_len);    //转换字段名,将字段对象与其名字进行关联
                Tmp[name_len]=0;

                rx_assert(m_fields.size()==i);
                //绑定并初始化字段对象
                field_t	&Field = m_fields[i];
                m_fields.bind(i,rx::st::strlwr(Tmp));
                Field.bind_data_type (this,reinterpret_cast <const char *> (param_name),name_len,oci_data_type,size,m_bat_fetch_count);
            }

            //循环进行字段值缓冲区的绑定
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
        //批量提取记录,等待访问
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
        //预解析一个SQL语句,得到必要的信息
        void prepare(const char *SQL, int Len = -1) { stmt_t::prepare(SQL, Len); }
        //-------------------------------------------------
        //预解析之后可以进行参数绑定
        sql_param_t& bind(const char *name, data_type_t type = DT_UNKNOWN, int MaxStringSize = MAX_TEXT_BYTES) { return stmt_t::bind(name,type, MaxStringSize); }
        //-------------------------------------------------
        //获取绑定的参数对象
        sql_param_t& param(const char* Name) { return stmt_t::param(Name); }
        //-------------------------------------------------
        //执行解析后的SQL语句,并尝试得到结果(入口为每次获取的批量数量)
        //执行后如果没有异常,就可以尝试访问结果集了
        void exec(ub2 fetch_size=BAT_FETCH_SIZE)
        {
            m_clear();
            stmt_t::exec();
            m_make_fields(fetch_size);
            m_bat_fetch();
        }
        //-------------------------------------------------
        //直接执行一条SQL语句,中间没有绑定参数的机会了
        void exec(const char *SQL,int Len = -1,ub2 fetch_size=BAT_FETCH_SIZE)
        {
            prepare(SQL,Len);
            exec(fetch_size);
        }
        //-------------------------------------------------
        //关闭当前的Query对象,释放全部资源
        void close()
        {
            m_clear();
            stmt_t::close();
        }
        //-------------------------------------------------
        //判断当前行是否结果集结束(exec/eof/next构成了结果集遍历原语)
        bool eof(void) const { return (m_cur_row_idx >= m_fetched_count && m_is_eof); }
        //-------------------------------------------------
        //跳转到下一行,返回值为false说明到结尾了
        //返回值:是否还有记录
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
        //得到当前结果集中字段的数量
        ub4 fields()
        {
            rx_assert(m_fields.size()==m_fields.capacity());
            return m_fields.size();
        }
        //-------------------------------------------------
        //根据序号访问字段
        field_t& operator [] (const ub4 field_idx)
        {
            rx_assert(field_idx<m_fields.size());
            return m_fields[field_idx];
        }
        //-------------------------------------------------
        //根据字段名查找或判断字段是否存在
        field_t* field(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity()) return NULL;
            return &m_fields[field_idx];
        }
        //-------------------------------------------------
        //根据字段名访问
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
    //由于field_t需要访问query_t中的信息,所以将其放在这里
    inline ub2 field_t::rel_row_idx()const
    {
        rx_assert (m_query!=NULL);
        return static_cast <ub2> (m_query->m_cur_row_idx % m_query->m_bat_fetch_count);
    }
}

#endif
