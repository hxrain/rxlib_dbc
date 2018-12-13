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
    //-----------------------------------------------------
    //字段与绑定参数使用的基类,进行数据缓冲区的管理与字段数据的访问
    class field_t:public col_base_t
    {
    public:
        field_t(){ }
        virtual ~field_t() { }
    };
}

#endif
