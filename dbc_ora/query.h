#ifndef	_RX_DBC_ORA_QUERY_H_
#define	_RX_DBC_ORA_QUERY_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //从stmt_t类集成,增加了结果集遍历获取的能力
    //-----------------------------------------------------
    class query_t:public stmt_t
    {
        friend class field_t;

        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t   m_fields;                           //字段数组
        ub2				m_fetch_bat_size;	                //每批次获取的记录数量
        ub4				m_fetched_count;	                //已经获取过的记录数量
        ub4				m_cur_row_idx;	                    //当前处理的记录在全部结果集中的行号
        ub2  			m_is_eof;			                //当前记录集是否已经完全获取完成

        //-------------------------------------------------
        //被禁止的操作
        query_t (const query_t&);
        query_t& operator = (const query_t&);
        //-------------------------------------------------
        //清理本子类中使用的状态与相关资源
        void m_clear (bool reset_only=false)
        {
            m_fetched_count = 0;
            m_cur_row_idx = 0;
            m_is_eof = false;
            for (uint32_t i = 0; i < m_fields.capacity(); ++i)
                m_fields[i].reset();
            m_fields.clear(reset_only);
        }

        //-------------------------------------------------
        //根据字段描述信息表生成字段对象数组,绑定名称
        ub4 m_make_fields ()
        {
            m_clear(true);                                  //状态归零,先不释放字段数组

            //获取当前结果集的字段数量
            ub4			count=0;
            sword result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &count, NULL, OCI_ATTR_PARAM_COUNT, m_conn.m_handle_err);
            if (result != OCI_SUCCESS)
                throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));

            if (count == 0)
                return 0;

            //动态生成字段对象数组
            rx_assert(m_fields.size()==0);
            if (!m_fields.make_ex(count,true))              //分配字段数组
                throw (error_info_t (DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL.c_str()));

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
                result = OCIParamGet (m_stmt_handle,OCI_HTYPE_STMT,m_conn.m_handle_err,reinterpret_cast <void **> (&param_handle),i + 1);	// first is 1

                if (result == OCI_SUCCESS)                          //根据参数句柄得到字段名字
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&param_name,&name_len,OCI_ATTR_NAME,m_conn.m_handle_err);

                if (result == OCI_SUCCESS)                          //根据参数句柄得到ORACLE数据类型
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&oci_data_type,NULL,OCI_ATTR_DATA_TYPE,m_conn.m_handle_err);

                if (result == OCI_SUCCESS)                          //根据参数句柄得到该字段的数据最大尺寸
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&size,NULL,OCI_ATTR_DATA_SIZE,m_conn.m_handle_err);

                if (param_handle)                                   //释放OCIParamGet生成的参数句柄
                    OCIDescriptorFree (param_handle,OCI_DTYPE_PARAM);

                if (result != OCI_SUCCESS)
                    throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));

                rx::st::strncpy(Tmp,(char*)param_name,name_len);    //转换字段名,将字段对象与其名字进行关联
                Tmp[name_len]=0;

                rx_assert(m_fields.size()==i);
                //绑定并初始化字段对象
                field_t	&Field = m_fields[i];
                m_fields.bind(i,rx::st::strlwr(Tmp));

                //进行OCI数据类型的归一化处理并构造对应的缓冲区
                oci_data_type=Field.bind_data_type (this,reinterpret_cast <const char *> (param_name),name_len,oci_data_type,size,m_fetch_bat_size);

                //进行字段缓冲区的绑定,缓冲区的行深度是m_fetch_bat_size,便于在OCIStmtFetch2的时候进行多行批量获取
                OCIDefine *field_handle=NULL;
                result = OCIDefineByPos(m_stmt_handle, &field_handle, m_conn.m_handle_err,
                    i+1,                                //跳过0,rowid列
                    Field.m_col_databuff.ptr(),         //数据缓冲区指针
                    Field.m_max_data_size,			    //每行字段数据的最大尺寸
                    oci_data_type,                      //字段类型
                    Field.m_col_dataempty.ptr(),        //数据是否为空的指示数组指针
                    Field.m_col_datasize.ptr<ub2>(),	//文本字段每行数据的实际尺寸数组,非文本应该为NULL
                    NULL,				                // ptr to array of field_t-level return codes
                    OCI_DEFAULT);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, Field.m_name.c_str()));
            }

            return count;
        }

        //-------------------------------------------------
        //批量提取记录,等待访问
        void m_bat_fetch (void)
        {
            sword	result;
            //记录之前已经提取过的结果数
            ub4		old_rows_count = m_fetched_count;
            //尝试批量获取一次结果集;我们使用了显示的批量提取方式,就不能使用内置OCI_ATTR_PREFETCH_ROWS预取模式.
#if RX_DBC_ORA_USE_OLD_STMT
            result = OCIStmtFetch(m_stmt_handle,m_conn.m_handle_err,m_fetch_bat_size,OCI_FETCH_NEXT,OCI_DEFAULT);
#else
            result = OCIStmtFetch2(m_stmt_handle, m_conn.m_handle_err, m_fetch_bat_size, OCI_FETCH_NEXT,0, OCI_DEFAULT);
#endif
            if (result == OCI_SUCCESS || result == OCI_NO_DATA || result == OCI_SUCCESS_WITH_INFO)
            {//正常完成了,或有条件完成了,则取出实际提取结果数;返回OCI_NO_DATA代表本批次结束了,但批次内的具体结果数量仍需要正常处理.
                result = OCIAttrGet (m_stmt_handle,OCI_HTYPE_STMT,&m_fetched_count,NULL,OCI_ATTR_ROW_COUNT,m_conn.m_handle_err);
                if (result != OCI_SUCCESS)
                    throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
                //如果本次提取的结果数量与前次提取数量的差小于要求提取的数量,则说明遇到结果集的尾部了.
                if (m_fetched_count - old_rows_count != (ub4)m_fetch_bat_size)
                    m_is_eof = true;                        //标记结果集提取结束
            }
            else
                throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
        }
        //-------------------------------------------------
        //根据序号访问字段
        field_t& field(const ub4 field_idx)
        {
            rx_assert(field_idx<m_fields.size());
            if (field_idx >= m_fields.capacity())
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__));
            return m_fields[field_idx];
        }
        //-------------------------------------------------
        //根据字段名访问
        field_t& field(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name, Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity())
                throw (error_info_t(DBEC_FIELD_NOT_FOUND, __FILE__, __LINE__, name));
            return m_fields[field_idx];
        }

    public:
        //-------------------------------------------------
        query_t(conn_t &Conn) :stmt_t(Conn), m_fields(Conn.m_mem) { m_clear(); set_fetch_bats(); }
        ~query_t (){close();}
        //-------------------------------------------------
        //设置结果集批量提取数
        void set_fetch_bats(ub2 bats= BAT_FETCH_SIZE) { m_fetch_bat_size = rx::Max(bats,(ub2)1); }
        //-------------------------------------------------
        //执行解析后的sql语句,并尝试得到结果(入口为每次获取的批量数量,默认0为最大深度)
        //执行后如果没有异常,就可以尝试访问结果集了
        query_t& exec(ub2 BulkCount=0)
        {
            stmt_t::exec(BulkCount);
            if (m_make_fields())
                m_bat_fetch();
            return *this;
        }
        //-------------------------------------------------
        //直接执行一条sql语句,中间没有绑定参数的机会了
        query_t& exec(const char *sql,...)
        {
            va_list arg;
            va_start(arg, sql);
            prepare(sql,arg);
            va_end(arg);
            return exec();
        }
        //-------------------------------------------------
        //关闭当前的Query对象,释放全部资源
        void close()
        {
            m_clear();
            stmt_t::close();
        }
        //-------------------------------------------------
        //判断当前是否为结果集真正的结束状态(exec/eof/next构成了结果集遍历原语)
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
        ub4 fields() { return m_fields.size(); }
        //-------------------------------------------------
        //根据字段名查找或判断字段是否存在
        //返回值:字段指针或NULL(不会抛出异常)
        field_t* exists(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity()) return NULL;
            return &m_fields[field_idx];
        }
        //-------------------------------------------------
        //运算符重载,访问字段
        field_t& operator[](const ub4 field_idx) { return field(field_idx); }
        field_t& operator[](const char *name) { return field(name); }
    };

    //-----------------------------------------------------
    //由于field_t需要访问query_t中的信息,所以将其放在这里
    inline ub2 field_t::bulk_row_idx()const
    {
        rx_assert (m_query!=NULL);
        return static_cast <ub2> (m_query->m_cur_row_idx % m_query->m_fetch_bat_size);
    }
}

#endif
