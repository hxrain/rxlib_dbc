#ifndef	_RX_DBC_PGSQL_FIELD_H_
#define	_RX_DBC_PGSQL_FIELD_H_

namespace pgsql
{
    /*
    begin;
    DECLARE cur CURSOR for select * from tmp_dbc;
    fetch 20 cur ;
    end
    */
    class query_t;
    //-----------------------------------------------------
    //结果集使用的字段访问对象
    class field_t:public col_base_t
    {
        friend class query_t;
        //-------------------------------------------------
        PGresult       *m_pg_res;
        uint32_t        m_cur_row;
        int             m_type_oid;

        //-------------------------------------------------
        //判断当前列是否为null空值
        virtual bool m_is_null() const
        {
            if (!m_pg_res) return true;
            return ::PQgetisnull(m_pg_res, m_cur_row, m_idx)!=0;
        }
        //-------------------------------------------------
        //获取当前列数据内容与长度
        virtual const char* m_value() const
        {
            return m_pg_res!=NULL? ::PQgetvalue(m_pg_res, m_cur_row, m_idx) : NULL;
        }
    protected:
        //-------------------------------------------------
        //绑定当前字段到一个确切的结果集上
        void bind(PGresult *res, int idx)
        {
            m_cur_row = 0;
            m_pg_res = res;
            m_type_oid = ::PQftype(m_pg_res, idx);
            col_base_t::bind(idx, ::PQfname(m_pg_res, idx),&m_type_oid);
        }
        //-------------------------------------------------
        //调整当前操作的结果集行索引
        void adj_row_idx(uint32_t row_idx)
        {
            m_cur_row = row_idx;
        }
        //-------------------------------------------------
        //清空当前的绑定状态
        void reset()
        {
            col_base_t::reset();
            m_cur_row = 0;
            m_type_oid = 0;
            m_pg_res = NULL;
        }
    public:
        field_t() { reset(); }
        virtual ~field_t() {}
    };
}

#endif
