#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"
#include "../rx_dbc_ora.h"
#include "rx_datetime_ex.h"

using namespace rx_dbc_ora;

//---------------------------------------------------------
typedef struct ut_ora
{
    conn_param_t conn_param;
    conn_t conn;

    ut_ora()
    {
        strcpy(conn_param.host, "20.0.2.106");
        strcpy(conn_param.user, "system");
        strcpy(conn_param.pwd, "sysdba");
        strcpy(conn_param.sid, "oradb");
    }
}ut_ora;

//---------------------------------------------------------
//数据库连接
inline bool ut_ora_base_conn(rx_tdd_t &rt, ut_ora &dbc)
{
    try {
        dbc.conn.open(dbc.conn_param);
        dbc.conn.schema_to("HYTPDTBILLDB");
        return true;
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(dbc.conn_param));
        return false;
    }
}

//---------------------------------------------------------
//数据库查询
inline bool ut_ora_base_query_1(rx_tdd_t &rt, ut_ora &dbc)
{
    try {
        query_t q(dbc.conn);

        for (q.exec("select * from tmp_dbc"); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
                q.field("id").as_long(), q.field("int").as_long(), q.field("uint").as_ulong(), 
                q.field("str").as_string(), q.field("mdate").as_string(), q.field("short").as_long());
        }
        return true;
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//参数绑定的查询
inline bool ut_ora_base_query_2(rx_tdd_t &rt, ut_ora &dbc)
{
    try {
        query_t q(dbc.conn);
        q.prepare("select * from tmp_dbc where str=:sSTR");
        q(":sSTR","2");
        
        for (q.exec(); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
                q.field("id").as_long(), q.field("int").as_long(), q.field("uint").as_ulong(),
                q.field("str").as_string(), q.field("mdate").as_string(), q.field("short").as_long());
        }
        return true;
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//绑定参数插入
inline bool ut_ora_base_insert_1(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        query_t q(dbc.conn);

        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        q(":nID", 2)(":nINT", -155905152)(":nUINT",(uint32_t)2155905152)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.exec();
        dbc.conn.trans_commit();
        return true;
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
inline bool ut_ora_base_insert_2(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定单条参数
        q(":nID", 2)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //执行语句
        q.exec();
        //提交
        dbc.conn.trans_commit();
        return true;
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//批量插入示例
inline bool ut_ora_base_insert_3(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //解析带有参数绑定的语句,同时告知最大批量块深度
        q.prepare(2,"insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");

        //给每个块深度对应的参数进行绑定与赋值
        q.bulk(0)(":nID", 2)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.bulk(1)(":nID", 3)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152)(":sSTR", "3")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //执行本次批量操作
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //必须对默认事务进行提交
        dbc.conn.trans_commit();

        //继续进行批量数据的绑定
        q.bulk(0)(":nID", 4)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //告知真正绑定的数据深度并执行操作
        q.exec(1);
        rt.tdd_assert(q.rows() == 1);
        //必须对默认事务进行提交
        dbc.conn.trans_commit();
        return true;
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
rx_tdd(ut_dtl_array)
{
    ut_ora ora;
    if (ut_ora_base_conn(*this, ora))
    {
        tdd_assert(ut_ora_base_insert_1(*this, ora));
        tdd_assert(ut_ora_base_insert_2(*this, ora));
        tdd_assert(ut_ora_base_insert_3(*this, ora));

        for (int i = 0; i < 10; ++i)
            tdd_assert(ut_ora_base_query_1(*this,ora));

        tdd_assert(ut_ora_base_query_2(*this, ora));
    }
}

#endif