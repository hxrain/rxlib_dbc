#ifndef	_RX_DBC_ORA_QUERY_H_
#define	_RX_DBC_ORA_QUERY_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //��stmt_t�༯��,�����˽����������ȡ������
    //-----------------------------------------------------
    class query_t:public stmt_t
    {
        friend class field_t;

        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t   m_fields;                           //�ֶ�����
        ub2  			m_bat_fetch_eof;			        //��ǰ��¼��������ȡ�Ƿ����(������ȡδ�ؽ���)
        ub2				m_bat_fetch_size;	                //ÿ���λ�ȡ�ļ�¼����
        ub4				m_bat_fetch_total;                  //�Ѿ���ȡ����������¼����
        ub4				m_cur_fetch_idx;                    //��ǰ����ļ�¼��ȫ��������е��к�
        ub4             m_cur_field_idx;                    //ʹ��>>�����ʱ�����ĵ�ǰ�ֶ����
        //-------------------------------------------------
        //����ֹ�Ĳ���
        query_t (const query_t&);
        query_t& operator = (const query_t&);
        //-------------------------------------------------
        //����������ʹ�õ�״̬�������Դ
        void m_clear (bool reset_only=false)
        {
            m_cur_field_idx = 0;
            m_bat_fetch_total = 0;
            m_cur_fetch_idx = 0;
            m_bat_fetch_eof = false;
            for (uint32_t i = 0; i < m_fields.capacity(); ++i)
                m_fields[i].reset();
            m_fields.clear(reset_only);
        }

        //-------------------------------------------------
        //�����ֶ�������Ϣ�������ֶζ�������,������
        ub4 m_make_fields ()
        {
            m_clear(true);                                  //״̬����,�Ȳ��ͷ��ֶ�����

            //��ȡ��ǰ��������ֶ�����
            ub4			count=0;
            sword result = OCIAttrGet(m_stmt_handle, OCI_HTYPE_STMT, &count, NULL, OCI_ATTR_PARAM_COUNT, m_conn.m_handle_err);
            if (result != OCI_SUCCESS)
                throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));

            if (count == 0)
                return 0;

            //��̬�����ֶζ�������
            rx_assert(m_fields.size()==0);
            if (!m_fields.make_ex(count,true))              //�����ֶ�����
                throw (error_info_t (type_t::DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL.c_str()));

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
                result = OCIParamGet (m_stmt_handle,OCI_HTYPE_STMT,m_conn.m_handle_err,reinterpret_cast <void **> (&param_handle),i + 1);	// first is 1

                if (result == OCI_SUCCESS)                          //���ݲ�������õ��ֶ�����
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&param_name,&name_len,OCI_ATTR_NAME,m_conn.m_handle_err);

                if (result == OCI_SUCCESS)                          //���ݲ�������õ�ORACLE��������
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&oci_data_type,NULL,OCI_ATTR_DATA_TYPE,m_conn.m_handle_err);

                if (result == OCI_SUCCESS)                          //���ݲ�������õ����ֶε��������ߴ�
                    result = OCIAttrGet (param_handle,OCI_DTYPE_PARAM,&size,NULL,OCI_ATTR_DATA_SIZE,m_conn.m_handle_err);

                if (param_handle)                                   //�ͷ�OCIParamGet���ɵĲ������
                    OCIDescriptorFree (param_handle,OCI_DTYPE_PARAM);

                if (result != OCI_SUCCESS)
                    throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));

                rx::st::strncpy(Tmp,(char*)param_name,name_len);    //ת���ֶ���,���ֶζ����������ֽ��й���
                Tmp[name_len]=0;

                rx_assert(m_fields.size()==i);
                //�󶨲���ʼ���ֶζ���
                field_t	&Field = m_fields[i];
                m_fields.bind(i,rx::st::strlwr(Tmp));

                //����OCI�������͵Ĺ�һ�����������Ӧ�Ļ�����
                oci_data_type=Field.bind_data_type (this,reinterpret_cast <const char *> (param_name),name_len,oci_data_type,size,m_bat_fetch_size);

                //�����ֶλ������İ�,���������������m_bat_fetch_size,������OCIStmtFetch2��ʱ����ж���������ȡ
                OCIDefine *field_handle=NULL;
                result = OCIDefineByPos(m_stmt_handle, &field_handle, m_conn.m_handle_err,
                    i+1,                                //����0,rowid��
                    Field.m_col_databuff.ptr(),         //���ݻ�����ָ��
                    Field.m_max_data_size,			    //ÿ���ֶ����ݵ����ߴ�
                    oci_data_type,                      //�ֶ�����
                    Field.m_col_dataempty.ptr(),        //�����Ƿ�Ϊ�յ�ָʾ����ָ��
                    Field.m_col_datasize.ptr<ub2>(),	//�ı��ֶ�ÿ�����ݵ�ʵ�ʳߴ�����,���ı�Ӧ��ΪNULL
                    NULL,				                // ptr to array of field_t-level return codes
                    OCI_DEFAULT);
                if (result != OCI_SUCCESS)
                    throw (error_info_t(result, m_conn.m_handle_err, __FILE__, __LINE__, Field.m_name.c_str()));
            }

            return count;
        }

        //-------------------------------------------------
        //������ȡ��¼,�ȴ�����
        void m_bat_fetch (void)
        {
            sword	result;
            //��¼֮ǰ�Ѿ���ȡ���Ľ����
            ub4		old_fetch_total = m_bat_fetch_total;
            //����������ȡһ�ν����;����ʹ������ʾ��������ȡ��ʽ,�Ͳ���ʹ������OCI_ATTR_PREFETCH_ROWSԤȡģʽ.
#if RX_DBC_ORA_USE_OLD_STMT
            result = OCIStmtFetch(m_stmt_handle,m_conn.m_handle_err,m_bat_fetch_size,OCI_FETCH_NEXT,OCI_DEFAULT);
#else
            result = OCIStmtFetch2(m_stmt_handle, m_conn.m_handle_err, m_bat_fetch_size, OCI_FETCH_NEXT,0, OCI_DEFAULT);
#endif
            if (result == OCI_SUCCESS || result == OCI_NO_DATA || result == OCI_SUCCESS_WITH_INFO)
            {//���������,�������������,��ȡ��ʵ����ȡ�����;����OCI_NO_DATA�������ν�����,�������ڵľ�������������Ҫ��������.
                result = OCIAttrGet (m_stmt_handle,OCI_HTYPE_STMT,&m_bat_fetch_total,NULL,OCI_ATTR_ROW_COUNT,m_conn.m_handle_err);
                if (result != OCI_SUCCESS)
                    throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
                //���������ȡ�Ľ��������ǰ����ȡ�����Ĳ�С��Ҫ����ȡ������,��˵�������������β����.
                if (m_bat_fetch_total - old_fetch_total != (ub4)m_bat_fetch_size)
                    m_bat_fetch_eof = true;       //��ǽ����������ȡ����
            }
            else
                throw (error_info_t (result, m_conn.m_handle_err, __FILE__, __LINE__, m_SQL.c_str()));
        }
        //-------------------------------------------------
        //������ŷ����ֶ�
        field_t& field(const ub4 field_idx)
        {
            rx_assert(field_idx<m_fields.size());
            if (field_idx >= m_fields.capacity())
                throw (error_info_t(type_t::DBEC_IDX_OVERSTEP, __FILE__, __LINE__));
            return m_fields[field_idx];
        }
        //-------------------------------------------------
        //�����ֶ�������
        field_t& field(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name, Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity())
                throw (error_info_t(type_t::DBEC_FIELD_NOT_FOUND, __FILE__, __LINE__, name));
            return m_fields[field_idx];
        }

    public:
        //-------------------------------------------------
        query_t(conn_t &Conn) :stmt_t(Conn), m_fields(Conn.m_mem) { m_clear(); set_fetch_bats(); }
        ~query_t (){close();}
        //-------------------------------------------------
        //���ý����������ȡ��
        void set_fetch_bats(ub2 bats= BAT_FETCH_SIZE) { m_bat_fetch_size = rx::Max(bats,(ub2)1); }
        //-------------------------------------------------
        //ִ�н������sql���,�����Եõ����(���Ϊÿ�λ�ȡ����������,Ĭ��0Ϊ������)
        //ִ�к����û���쳣,�Ϳ��Գ��Է��ʽ������
        query_t& exec(ub2 BulkCount=0)
        {
            stmt_t::exec(BulkCount);
            if (m_make_fields())
                m_bat_fetch();
            return *this;
        }
        //-------------------------------------------------
        //ֱ��ִ��һ��sql���,�м�û�а󶨲����Ļ�����
        query_t& exec(const char *sql, va_list arg)
        {
            prepare(sql,arg);
            return exec();
        }
        //-------------------------------------------------
        //ֱ��ִ��һ��sql���,�м�û�а󶨲����Ļ�����
        query_t& exec(const char *sql, ...)
        {
            va_list arg;
            va_start(arg, sql);
            exec(sql, arg);
            va_end(arg);
            return *this;
        }
        //-------------------------------------------------
        //�رյ�ǰ��Query����,�ͷ�ȫ����Դ
        void close()
        {
            m_clear();
            stmt_t::close();
        }
        //-------------------------------------------------
        //�жϵ�ǰ�Ƿ�Ϊ����������Ľ���״̬(exec/eof/next�����˽��������ԭ��)
        bool eof(void) const { return (m_cur_fetch_idx >= m_bat_fetch_total && m_bat_fetch_eof); }
        //-------------------------------------------------
        //��ת����һ��,����ֵΪfalse˵������β��
        //����ֵ:�Ƿ��м�¼
        bool next(void)
        {
            rx_assert_if(m_cur_field_idx,m_cur_field_idx==m_fields.size());
            m_cur_field_idx=0;

            ++m_cur_fetch_idx;

            if (m_cur_fetch_idx >= m_bat_fetch_total)
            {
                if (m_bat_fetch_eof)
                    return false;
                m_bat_fetch();
            }
            if (m_cur_fetch_idx >= m_bat_fetch_total)
                return false;

            return true;
        }
        //-------------------------------------------------
        uint32_t fetched() { return m_bat_fetch_total ? m_cur_fetch_idx + 1 : 0; }
        //-------------------------------------------------
        //�õ���ǰ��������ֶε�����
        ub4 fields() { return m_fields.size(); }
        //-------------------------------------------------
        //�����ֶ������һ��ж��ֶ��Ƿ����
        //����ֵ:�ֶ�ָ���NULL(�����׳��쳣)
        field_t* exists(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity()) return NULL;
            return &m_fields[field_idx];
        }
        //-------------------------------------------------
        //��ѯָ�����и��������µļ�¼����(��������������Ƽ�,��Ҫʹ�ð󶨱���ģʽ)
        int32_t query_records(const char* tblname, const char* cond = NULL)
        {
            if (is_empty(cond))
                exec("select count(1) as c from %s", tblname);
            else
                exec("select count(1) as c from %s where %s", tblname, cond);

            if (eof()) return 0;
            return field("c").as_long();
        }
        //-------------------------------------------------
        //���������,�����ֶ�
        field_t& operator[](const ub4 field_idx) { return field(field_idx); }
        field_t& operator[](const char *name) { return field(name); }
        //-------------------------------------------------
        //˳����ȡ�ֶε�ֵ
        template<typename DT>
        query_t& operator >> (DT &value)
        {
            rx_assert(m_cur_field_idx<m_fields.size());
            m_fields[m_cur_field_idx++].to(value);
            return *this;
        }

    };

    //-----------------------------------------------------
    //����field_t��Ҫ����query_t�е���Ϣ,���Խ����������
    inline ub2 field_t::bulk_row_idx()const
    {
        rx_assert (m_query!=NULL);
        return static_cast <ub2> (m_query->m_cur_fetch_idx % m_query->m_bat_fetch_size);
    }
}

#endif
