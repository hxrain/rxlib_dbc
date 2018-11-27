#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"
#include "../rx_dbc_ora.h"
#include "rx_datetime_ex.h"

using namespace rx_dbc_ora;

//---------------------------------------------------------
//测试使用的对象容器
typedef struct ut_ora
{
    conn_param_t conn_param;
    conn_t conn;

    ut_ora()
    {
        strcpy(conn_param.host, "20.0.2.106");
        strcpy(conn_param.user, "system");
        strcpy(conn_param.pwd, "sysdba");
        strcpy(conn_param.db, "oradb");
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
//简单查询
inline bool ut_ora_base_query_1(rx_tdd_t &rt, ut_ora &dbc)
{
    try {
        query_t q(dbc.conn);

        for (q.exec("select * from tmp_dbc"); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
                q["id"].as_long(), q["int"].as_long(), q["uint"].as_ulong(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_long());
        }

        for (q.exec("select * from tmp_dbc"); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
                q["id"].as_long(), q["int"].as_long(), q["uint"].as_ulong(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_long());
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

        q.exec("delete from tmp_dbc where str!='str'").conn().trans_commit();

        q.prepare("select * from tmp_dbc where str=:sSTR");
        q(":sSTR","2");

        for (q.exec(); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
                q["id"].as_long(), q["int"].as_long(), q["uint"].as_ulong(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_long());
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
//绑定参数插入(使用query_t)
inline bool ut_ora_base_insert_1(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        query_t q(dbc.conn);

        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        q(":nID", 2)(":nINT", -155905152)(":nUINT",(uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.exec();
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
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
//参数绑定插入示例(使用stmt_t)
inline bool ut_ora_base_insert_2(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定单条参数
        q(":nID", 2)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //执行语句
        q.exec();
        //提交
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
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
//参数绑定插入示例(使用stmt_t)
inline bool ut_ora_base_insert_2b(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定参数
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //绑定数据
        q << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        //执行语句
        q.exec();
        //提交
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
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
        q.bulk(0)(":nID", 2)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.bulk(1)(":nID", 3)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //执行本次批量操作
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //必须对默认事务进行提交
        dbc.conn.trans_commit();

        //继续进行批量数据的绑定
        q.bulk(0)(":nID", 4)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
//进行自动参数绑定的插入示例
inline bool ut_ora_base_insert_4(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);

        //预处理解析并进行参数的自动绑定
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind();
        q << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;   //顺序给参数进行数据赋值
        q.exec().conn().trans_commit();                     //执行语句并提交

        rt.tdd_assert(q.rows() == 1);
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
//进行自动参数绑定的批量插入示例
inline bool ut_ora_base_insert_5(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //解析带有参数绑定的语句,同时告知最大批量块深度
        q.prepare(2, "insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind();

        //给每个块深度对应的参数进行赋值
        q.bulk(0) << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.bulk(1) << 3 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32769;
        q.exec().conn().trans_commit();                     //执行本次批量操作,并进行提交
        rt.tdd_assert(q.rows() == 2);

        //继续进行批量数据的绑定
        q.bulk(0) << 4 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.exec(1).conn().trans_commit();                    //告知真正绑定的数据深度并执行操作,并进行提交
        rt.tdd_assert(q.rows() == 1);

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
//sql绑定参数解析示例
inline void ut_ora_base_sql_parse_1(rx_tdd_t &rt)
{
    sql_param_parse_t<> sp;
    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;") == NULL);
    rt.tdd_assert(sp.count == 2);
    rt.tdd_assert(strncmp(sp.segs[0].name, ":nID", sp.segs[0].name_len) == 0);
    rt.tdd_assert(sp.segs[0].name_len == 4);
    rt.tdd_assert(strncmp(sp.segs[1].name, ":nUINT", sp.segs[1].name_len) == 0);
    rt.tdd_assert(sp.segs[1].name_len == 6);

    rt.tdd_assert(sp.ora_sql("select id,:se'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'b':se,':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and (UINT=:nUINT)") != NULL);
    rt.tdd_assert(sp.ora_sql("select id,'b' :se,':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and (UINT=:nUINT)") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se"",\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se\"\",\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT:se from tmp_dbc where id=:nID and UINT=:nUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT:se from tmp_dbc where id=:nID and UINT=: nUINT;") != NULL);
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT :se from tmp_dbc where id=:nID and UINT=:nUINT;") == NULL);   //解析通过,但不符合sql语法
}
//---------------------------------------------------------
//进行数据库基础动作测试
inline void ut_ora_base_1(rx_tdd_t &rt)
{
    ut_ora ora;
    if (ut_ora_base_conn(rt, ora))
    {
        rt.tdd_assert(ut_ora_base_insert_1(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_2(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_2b(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_3(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_4(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_5(rt, ora));

        for (int i = 0; i < 10; ++i)
            rt.tdd_assert(ut_ora_base_query_1(rt, ora));

        rt.tdd_assert(ut_ora_base_query_2(rt, ora));
    }
}
//---------------------------------------------------------
rx_tdd(ut_dtl_array)
{
    ut_ora_base_sql_parse_1(*this);
    ut_ora_base_1(*this);
    for(int i=0;i<10;++i)
        ut_ora_base_1(*this);
}

#endif
