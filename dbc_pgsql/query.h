#ifndef	_RX_DBC_PGSQL_QUERY_H_
#define	_RX_DBC_PGSQL_QUERY_H_

namespace pgsql
{
    //-----------------------------------------------------
    //��stmt_t�༯��,�����˽����������ȡ������
    //-----------------------------------------------------
    class query_t:public stmt_t
    {
        friend class field_t;
        const uint16_t NOT_BAT_FETCH = 0xFFFF;
        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t           m_fields;                   //�ֶ�����
        uint16_t		        m_fetch_bat_size;	        //ÿ���λ�ȡ�ļ�¼����,-1ʱ��������ȡȫ�������.
        uint32_t		        m_fetched_count;	        //�Ѿ���ȡ���ļ�¼����
        uint32_t                m_cur_field_idx;            //��ȡ�ֶε�ʱ������˳��������
        bool    	            m_is_eof;			        //��ǰ��¼���Ƿ��Ѿ���ȫ��ȡ���

        //-------------------------------------------------
        //����ֹ�Ĳ���
        query_t (const query_t&);
        query_t& operator = (const query_t&);
        //-------------------------------------------------
        //����������ʹ�õ�״̬�������Դ
        void m_clear (bool reset_only=false)
        {
            m_fetched_count = 0;
            m_cur_field_idx = 0;
            m_is_eof = false;
            for (uint32_t i = 0; i < m_fields.capacity(); ++i)
                m_fields[i].reset();
            m_fields.clear(reset_only);
        }

        //-------------------------------------------------
        //�����ֶ�������Ϣ�������ֶζ�������,������
        uint32_t m_make_fields ()
        {
            m_clear(true);                                  //״̬����,�Ȳ��ͷ��ֶ�����
            rx_assert(m_raw_stmt.res() != NULL);
            
            uint32_t fields = ::PQnfields(m_raw_stmt.res());
            uint32_t rows = ::PQntuples(m_raw_stmt.res());
            if ( rows == 0|| fields == 0)                   //û�н��,ֱ�ӷ���
            {
                m_is_eof = true;
                return 0;
            }

            //�����ֶζ���������Ԫ��Ϣ����
            if (!m_fields.make(fields,true))
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            //���ֶζ�����Ԫ��Ϣ
            for (uint32_t i = 0; i < fields; ++i)
            {
                char Tmp[200];
                rx::st::strlwr(::PQfname(m_raw_stmt.res(), i), Tmp);
                m_fields.bind(i, Tmp);                      //���ֶΰ󶨱���

                field_t &col = m_fields[i];
                col.bind(m_raw_stmt.res(), i, Tmp);         //���ֶΰ󶨽����Ԫ��Ϣ
            }

            return fields;
        }

        //-------------------------------------------------
        //������ȡ��¼,�ȴ�����
        void m_bat_fetch (void)
        {
            rx_assert(m_raw_stmt.res() != NULL);
            rx_assert(!m_is_eof);
            if (m_fetch_bat_size == NOT_BAT_FETCH)
            {//δ������ȡģʽ
                if ((int)m_fetched_count < ::PQntuples(m_raw_stmt.res()))
                {
                    for (uint32_t i = 0; i < m_fields.size(); ++i)
                        m_fields[i].adj_row_idx(m_fetched_count);
                    ++m_fetched_count;
                }
                else
                    m_is_eof = true;
            }
            else
            {//������ȡģʽ

            }
        }
        //-------------------------------------------------
        //������ŷ����ֶ�
        field_t& field(const uint32_t field_idx)
        {
            rx_assert(field_idx<m_fields.size());
            if (field_idx >= m_fields.capacity())
                throw (error_info_t(DBEC_IDX_OVERSTEP, __FILE__, __LINE__));
            return m_fields[field_idx];
        }
        //-------------------------------------------------
        //�����ֶ�������
        field_t& field(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name, Tmp);
            uint32_t field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity())
                throw (error_info_t(DBEC_FIELD_NOT_FOUND, __FILE__, __LINE__, name));
            return m_fields[field_idx];
        }

    public:
        //-------------------------------------------------
        query_t(conn_t &Conn) :stmt_t(Conn), m_fields(Conn.m_mem) { m_clear(); set_fetch_bats(NOT_BAT_FETCH); }
        ~query_t (){close();}
        //-------------------------------------------------
        //���ý����������ȡ��
        void set_fetch_bats(uint16_t bats= BAT_FETCH_SIZE) { m_fetch_bat_size = rx::Max(bats,(uint16_t)1); }
        //-------------------------------------------------
        //ִ�н������sql���,�����Եõ����
        //ִ�к����û���쳣,�Ϳ��Գ��Է��ʽ������
        query_t& exec(uint32_t dummy=-1)
        {
            stmt_t::exec();
            if (m_make_fields())
                m_bat_fetch();
            return *this;
        }
        //-------------------------------------------------
        //ֱ��ִ��һ��sql���,�м�û�а󶨲����Ļ�����
        query_t& exec(const char *sql, va_list arg)
        {
            stmt_t::exec(sql, arg);
            if (m_make_fields())
                m_bat_fetch();
            return *this;
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
        bool eof(void) const { return m_is_eof; }
        //-------------------------------------------------
        //��ת����һ��,����ֵΪfalse˵������β��
        //����ֵ:�Ƿ��м�¼
        bool next(void)
        {
            if (m_is_eof) return false;
            m_bat_fetch();
            return !m_is_eof;
        }
        //-------------------------------------------------
        uint32_t fetched() { return m_fetched_count; }
        //-------------------------------------------------
        //�õ���ǰ��������ֶε�����
        uint32_t fields() { return m_fields.size(); }
        //-------------------------------------------------
        //�����ֶ������һ��ж��ֶ��Ƿ����
        //����ֵ:�ֶ�ָ���NULL(�����׳��쳣)
        field_t* exists(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            uint32_t field_idx = m_fields.index(Tmp);
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
            return field("c").as_int();
        }
        //-------------------------------------------------
        //���������,�����ֶ�
        field_t& operator[](const uint32_t field_idx) { return field(field_idx); }
        field_t& operator[](const char *name) { return field(name); }
        //-------------------------------------------------
        //˳����ȡ�ֶε�ֵ
        template<typename DT>
        query_t& operator >> (DT &value)
        {
            rx_assert(m_cur_field_idx<m_fields.size());
            m_fields[field_idx].to(value);
            return *this;
        }
    };
}

#endif
