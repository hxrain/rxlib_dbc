#ifndef	_RX_DBC_MYSQL_PARAMETER_H_
#define	_RX_DBC_MYSQL_PARAMETER_H_

namespace mysql
{
    //-----------------------------------------------------
    //sql语句绑定参数,即可用于输入,也可用于输出
    //对于批量模式,参数对象对应着一个列的多行值,通过use_bulk()进行调整
    class sql_param_t:public field_t
    {
        friend class stmt_t;

        //-------------------------------------------------
        sql_param_t(const sql_param_t&);
        sql_param_t& operator = (const sql_param_t&);

        //-------------------------------------------------
        //绑定数字值
        sql_param_t& set_long(int32_t value, bool is_signed)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_LONG;
            m_metainfo->is_unsigned = !is_signed;
            m_metainfo->length_value = sizeof(value);
            *((int32_t*)m_metainfo->buffer) = value;
            return *this;
        }
        //-------------------------------------------------
        //绑定大数字与浮点数
        sql_param_t& set_double(double value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_DOUBLE;
            m_metainfo->is_unsigned = 0;
            m_metainfo->length_value = sizeof(value);
            *((double*)m_metainfo->buffer) = value;
            return *this;
        }

        //-------------------------------------------------
        sql_param_t& set_longlong(int64_t value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_LONGLONG;
            m_metainfo->is_unsigned = 1;
            m_metainfo->length_value = sizeof(value);
            *((int64_t*)m_metainfo->buffer) = value;
            return *this;
        }
        //-------------------------------------------------
        //绑定日期数据
        sql_param_t& set_datetime(const datetime_t& value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_DATETIME;
            m_metainfo->length_value = sizeof(value);
            value.to(*((MYSQL_TIME*)m_metainfo->buffer));
            return *this;
        }
        //-------------------------------------------------
        //绑定文本串
        sql_param_t& set_string(PStr value)
        {
            m_metainfo->buffer_type = MYSQL_TYPE_STRING;
            m_metainfo->length_value = rx::st::strcpy2((char*)m_metainfo->buffer, m_metainfo->buffer_length, value);
            return *this;
        }

    public:
        //-------------------------------------------------
        sql_param_t() :field_t() { }
        ~sql_param_t() { }
        //-------------------------------------------------
        //让当前参数设置为空值
        void set_null() { rx_assert(m_metainfo!=NULL); m_metainfo->is_null_value=1; }
        //-------------------------------------------------
        //给参数的指定数组批量元素赋值:字符串值
        sql_param_t& operator = (PStr text) { return set_string(text);}
        //-------------------------------------------------
        //设置参数中指定序号的批量元素为浮点数
        sql_param_t& operator = (double value) { return set_double(value); }
        //-------------------------------------------------
        //设置参数为大整数值(带符号)
        sql_param_t& operator = (int64_t value) { return set_longlong(value); }
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
