#ifndef	_RX_DBC_PGSQL_PARAMETER_H_
#define	_RX_DBC_PGSQL_PARAMETER_H_

namespace pgsql
{
    //-----------------------------------------------------
    //sql语句绑定参数,即可用于输入,也可用于输出
    class param_t:public col_base_t
    {
        friend class stmt_t;
        //-------------------------------------------------
        param_t(const param_t&);
        param_t& operator = (const param_t&);

        char            m_buff[MAX_TEXT_BYTES];
        char          **m_values;
        //-------------------------------------------------
        //判断当前列是否为null空值
        virtual bool m_is_null() const { return m_values==NULL|| m_values[m_idx]==NULL; }
        //-------------------------------------------------
        //获取当前列数据内容
        virtual const char* m_value() const { return m_buff; }
        //-------------------------------------------------
        //绑定数字值
        param_t& set_long(int32_t value, bool is_signed)
        {
            if (is_signed)
            {
                rx::st::itoa(value, m_buff);
                m_set_type(PG_DATA_TYPE_INT4);
            }
            else
            {
                rx::st::ultoa(value, m_buff);
                m_set_type(PG_DATA_TYPE_INT8);
            }
            return *this;
        }
        //-------------------------------------------------
        //绑定大数字与浮点数
        param_t& set_double(double value)
        {
            rx::st::ftoa(value, m_buff);
            m_set_type(PG_DATA_TYPE_FLOAT8);
            return *this;
        }
        //-------------------------------------------------
        param_t& set_longlong(int64_t value)
        {
            rx::st::itoa64(value, m_buff);
            m_set_type(PG_DATA_TYPE_INT8);
            return *this;
        }
        //-------------------------------------------------
        //绑定日期数据
        param_t& set_datetime(const datetime_t& value)
        {
            value.to(m_buff);
            m_set_type(PG_DATA_TYPE_TIMESTAMP);
            return *this;
        }
        //-------------------------------------------------
        //绑定文本串
        param_t& set_string(PStr value)
        {
            rx::st::strcpy2(m_buff,sizeof(m_buff),value);
            m_set_type(PG_DATA_TYPE_TEXT);
            return *this;
        }
    protected:
        //-------------------------------------------------
        //给当前参数对象绑定对应的元信息
        void bind(char **values,int idx,int *type_oid,const char* name)
        {
            m_values = values;
            col_base_t::bind(idx,name,type_oid);
            set_null();                                     //默认参数值为null空
        }
        //-------------------------------------------------
        //绑定信息复位
        void reset()
        {
            m_values = NULL;
            m_buff[0] = 0;
            col_base_t::reset();
        }
        //-------------------------------------------------
        //设置绑定值的类型
        void m_set_type(int type) 
        { 
            rx_assert(m_values != NULL);
            rx_assert(m_idx != -1);
            rx_assert(m_type_oid != NULL);

            m_values[m_idx] = m_buff;                       //设置绑定参数为非空
            *m_type_oid = type;
        }
    public:
        //-------------------------------------------------
        param_t() :col_base_t() { reset(); }
        ~param_t() { }
        //-------------------------------------------------
        //让当前参数设置为空值
        void set_null() { rx_assert(m_values != NULL && m_idx!=-1); m_buff[0] = 0; if (m_values) m_values[m_idx] = NULL; }
        //-------------------------------------------------
        //给参数的指定数组批量元素赋值:字符串值
        param_t& operator = (PStr text) { return set_string(text);}
        //-------------------------------------------------
        //设置参数中指定序号的批量元素为浮点数
        param_t& operator = (double value) { return set_double(value); }
        //-------------------------------------------------
        //设置参数为大整数值(带符号)
        param_t& operator = (int64_t value) { return set_longlong(value); }
        //-------------------------------------------------
        //设置参数为整数值(带符号)
        param_t& operator = (int32_t value) { return set_long(value, true); }
        //-------------------------------------------------
        //设置参数为整数值(无符号)
        param_t& operator = (uint32_t value) { return set_long(value, false); }
        //-------------------------------------------------
        //设置参数为日期时间值
        param_t& operator = (const datetime_t& d) { return set_datetime(d); }
    };
}

#endif
