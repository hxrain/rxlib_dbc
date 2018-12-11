#ifndef	_RX_DBC_MYSQL_QUERY_H_
#define	_RX_DBC_MYSQL_QUERY_H_

namespace mysql
{
    //-----------------------------------------------------
    //从stmt_t类集成,增加了结果集遍历获取的能力
    //-----------------------------------------------------
    class query_t:public stmt_t
    {
        friend class field_t;

        typedef rx::alias_array_t<field_t, FIELD_NAME_LENGTH> field_array_t;
        field_array_t           m_fields;                   //字段数组
        uint16_t		        m_fetch_bat_size;	        //每批次获取的记录数量
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
            rx_assert(m_stmt_handle != NULL);

            uint32_t count = mysql_stmt_field_count(m_stmt_handle);
            if (!count)
                return 0;                                   //获取结果集字段数量

            //生成字段对象数组与元信息数组
            if (!m_fields.make(count,true)||!m_metainfos.make(count, true))
                throw (error_info_t(DBEC_NO_MEMORY, __FILE__, __LINE__, m_SQL));

            //获取字段信息
            MYSQL_RES *cols = mysql_stmt_result_metadata(m_stmt_handle);
            if (!cols)
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));

            //绑定字段对象与元信息
            rx_assert(cols->field_count == count);
            for (uint32_t i = 0; i < count; ++i)
            {
                MYSQL_BIND  &mi = m_metainfos.at(i);        //字段绑定的元信息
                MYSQL_FIELD &mf = cols->fields[i];          //字段元信息

                char Tmp[200];
                rx::st::strlwr(mf.name, Tmp);

                m_fields.bind(i, Tmp);                      //进行字段对象的名字绑定

                field_t &col = m_fields[i];                 //得到字段对象
                col.make(mf.name, &m_metainfos.at(i));      //字段对象初始化
                mi.is_unsigned = (mf.flags&NUM_FLAG) && (mf.flags&UNSIGNED_FLAG);
                mi.buffer_type = mf.type;                   //修正字段绑定元信息中的字段类型
            }

            mysql_free_result(cols);                        //释放mysql_stmt_result_metadata返回的结果集元信息

            //绑定元信息数组
            if (mysql_stmt_bind_result(m_stmt_handle, m_metainfos.array()))
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));

            //设置预取数量
            uint32_t v = m_fetch_bat_size;
            if (mysql_stmt_attr_set(m_stmt_handle, STMT_ATTR_PREFETCH_ROWS,&v))
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));

            return count;
        }

        //-------------------------------------------------
        //批量提取记录,等待访问
        void m_bat_fetch (void)
        {
            rx_assert(m_stmt_handle != NULL);
            int rc = mysql_stmt_fetch(m_stmt_handle);
            switch (rc)
            {
            case 0:
                ++m_fetched_count;
                m_cur_field_idx = 0;
                return;
            case MYSQL_NO_DATA:
                m_is_eof = true;
                break;
            case MYSQL_DATA_TRUNCATED:
                //需要处理,看哪个字段的数据被截断了
                for (uint32_t i = 0; i < m_fields.size(); ++i)
                    rx_assert_msg(m_metainfos.at(i).error_value == 0, m_fields[i].name());

                ++m_fetched_count;
                m_cur_field_idx = 0;
                return;
            case 1:
            default:
                throw (error_info_t(m_stmt_handle, __FILE__, __LINE__, m_SQL));
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
        query_t(conn_t &Conn) :stmt_t(Conn), m_fields(Conn.m_mem) { m_clear(); set_fetch_bats(); }
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
            prepare(sql,arg);
            return exec();
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
            m_fields[m_cur_field_idx++].to(value);
            return *this;
        }
    };
}

#endif
