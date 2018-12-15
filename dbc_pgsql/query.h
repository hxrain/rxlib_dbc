#ifndef	_RX_DBC_PGSQL_QUERY_H_
#define	_RX_DBC_PGSQL_QUERY_H_

namespace pgsql
{
    //-----------------------------------------------------
    //从stmt_t类集成,增加了结果集遍历获取的能力
    //-----------------------------------------------------
    class query_t:public stmt_t
    {
        friend class field_t;
        const uint16_t NOT_BAT_FETCH = 0xFFFF;
        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t           m_fields;                   //字段数组
        uint16_t		        m_fetch_bat_size;	        //每批次获取的记录数量,-1时不分批获取全部结果集.
        uint32_t		        m_fetched_count;	        //已经获取过的记录数量
        uint32_t                m_cur_field_idx;            //提取字段的时候用于顺序处理的序号
        bool    	            m_is_eof;			        //当前记录集是否已经完全获取完成

        //-------------------------------------------------
        //被禁止的操作
        query_t (const query_t&);
        query_t& operator = (const query_t&);
        //-------------------------------------------------
        //清理本子类中使用的状态与相关资源
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
        //根据字段描述信息表生成字段对象数组,绑定名称
        uint32_t m_make_fields ()
        {
            m_clear(true);                                  //状态归零,先不释放字段数组
            rx_assert(m_raw_stmt.res() != NULL);
            
            uint32_t fields = ::PQnfields(m_raw_stmt.res());
            uint32_t rows = ::PQntuples(m_raw_stmt.res());
            if ( rows == 0|| fields == 0)                   //没有结果,直接返回
            {
                m_is_eof = true;
                return 0;
            }

            //生成字段对象数组与元信息数组
            if (!m_fields.make(fields,true))
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            //绑定字段对象与元信息
            for (uint32_t i = 0; i < fields; ++i)
            {
                char Tmp[200];
                rx::st::strlwr(::PQfname(m_raw_stmt.res(), i), Tmp);
                m_fields.bind(i, Tmp);                      //给字段绑定别名

                field_t &col = m_fields[i];
                col.bind(m_raw_stmt.res(), i, Tmp);         //给字段绑定结果集元信息
            }

            return fields;
        }

        //-------------------------------------------------
        //批量提取记录,等待访问
        void m_bat_fetch (void)
        {
            rx_assert(m_raw_stmt.res() != NULL);
            rx_assert(!m_is_eof);
            if (m_fetch_bat_size == NOT_BAT_FETCH)
            {//未分批提取模式
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
            {//分批提取模式

            }
        }
        //-------------------------------------------------
        //根据序号访问字段
        field_t& field(const uint32_t field_idx)
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
        //设置结果集批量提取数
        void set_fetch_bats(uint16_t bats= BAT_FETCH_SIZE) { m_fetch_bat_size = rx::Max(bats,(uint16_t)1); }
        //-------------------------------------------------
        //执行解析后的sql语句,并尝试得到结果
        //执行后如果没有异常,就可以尝试访问结果集了
        query_t& exec(uint32_t dummy=-1)
        {
            stmt_t::exec();
            if (m_make_fields())
                m_bat_fetch();
            return *this;
        }
        //-------------------------------------------------
        //直接执行一条sql语句,中间没有绑定参数的机会了
        query_t& exec(const char *sql, va_list arg)
        {
            stmt_t::exec(sql, arg);
            if (m_make_fields())
                m_bat_fetch();
            return *this;
        }
        //-------------------------------------------------
        //直接执行一条sql语句,中间没有绑定参数的机会了
        query_t& exec(const char *sql, ...)
        {
            va_list arg;
            va_start(arg, sql);
            exec(sql, arg);
            va_end(arg);
            return *this;
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
        bool eof(void) const { return m_is_eof; }
        //-------------------------------------------------
        //跳转到下一行,返回值为false说明到结尾了
        //返回值:是否还有记录
        bool next(void)
        {
            if (m_is_eof) return false;
            m_bat_fetch();
            return !m_is_eof;
        }
        //-------------------------------------------------
        uint32_t fetched() { return m_fetched_count; }
        //-------------------------------------------------
        //得到当前结果集中字段的数量
        uint32_t fields() { return m_fields.size(); }
        //-------------------------------------------------
        //根据字段名查找或判断字段是否存在
        //返回值:字段指针或NULL(不会抛出异常)
        field_t* exists(const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            uint32_t field_idx = m_fields.index(Tmp);
            if (field_idx == m_fields.capacity()) return NULL;
            return &m_fields[field_idx];
        }
        //-------------------------------------------------
        //查询指定表中给定条件下的记录数量(条件多变的情况不推荐,需要使用绑定变量模式)
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
        //运算符重载,访问字段
        field_t& operator[](const uint32_t field_idx) { return field(field_idx); }
        field_t& operator[](const char *name) { return field(name); }
        //-------------------------------------------------
        //顺序提取字段的值
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
