#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"
#include "ut_dbc_comm.h"

//---------------------------------------------------------
//简单查询
inline bool ut_conn_base_query_1(rx_tdd_t &rt, ut_conn &dbc)
{
    try {
        query_t q(dbc.conn);

        for (q.exec("select * from tmp_dbc"); !q.eof(); q.next())
        {
            printf("fetch_count=%d:id(%d),intn(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",q.fetched(),
                q["id"].as_int(), q["intn"].as_int(), q["uint"].as_uint(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_int());
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
inline bool ut_conn_base_query_2(rx_tdd_t &rt, ut_conn &dbc)
{
    try {
        query_t q(dbc.conn);

        q.prepare("select * from tmp_dbc where str=:sSTR");
        q(":sSTR","2");

        for (q.exec(); !q.eof(); q.next())
        {
            printf("fc=%d:id(%d),intn(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",q.fetched(),
                q["id"].as_int(), q["intn"].as_int(), q["uint"].as_uint(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_int());
        }

        //q.exec("delete from tmp_dbc where str!='str'").conn().trans_commit();

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
inline bool ut_conn_base_insert_1(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        query_t q(dbc.conn);

        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        q(":uvID", 20)(":iINT", -155905152)(":uUINT",(uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        q.exec();
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//参数绑定插入示例(使用stmt_t)
inline bool ut_conn_base_insert_2(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //绑定单条参数
        q(":uvID", 21)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //执行语句
        q.exec();
        //提交
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//参数绑定插入示例(使用stmt_t)
inline bool ut_conn_base_insert_2b(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //绑定参数
        q(":uvID")(":iINT")(":uUINT")(":sSTR")(":dDATE")(":iSHORT");
        //绑定数据
        q << 24 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        //执行语句
        q.exec();
        //提交
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//参数绑定插入示例(使用stmt_t与显示事务)
inline bool ut_conn_base_insert_2c(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        rt.tdd_assert(dbc.conn.ping());

        dbc.conn.trans_begin();

        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //绑定参数
        q(":uvID")(":iINT")(":uUINT")(":sSTR")(":dDATE")(":iSHORT");
        //绑定数据
        q << 22 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        //执行语句
        q.exec();

        q << 23 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        //再次执行语句
        q.exec();
        //提交
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        int32_t ec = 0;
        dbc.conn.trans_rollback(&ec);
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//批量插入手动绑定示例
inline bool ut_conn_base_insert_3(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //解析带有参数绑定的语句,同时告知最大批量块深度
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)").manual_bind(2);
#if UT_DB==DB_ORA
        //给每个块深度对应的参数进行绑定与赋值
        q.bulk(0)(":uvID", 25)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        q.bulk(1)(":uvID", 35)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //执行本次批量操作
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //必须对默认事务进行提交
        dbc.conn.trans_commit();

        //继续进行批量数据的绑定
        q.bulk(0)(":uvID", 45)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //告知真正绑定的数据深度并执行操作
        q.exec(1);
        rt.tdd_assert(q.rows() == 1);
#else
        q(":uvID", 25)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767).exec();
        rt.tdd_assert(q.rows() == 1);

        q(":uvID", 35)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":iSHORT", 32767).exec();
        rt.tdd_assert(q.rows() == 1);
        //先对默认事务进行提交
        dbc.conn.trans_commit();

        //继续进行批量数据的绑定
        q(":uvID", 45)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767).exec();
        rt.tdd_assert(q.rows() == 1);

#endif
        //再对默认事务进行提交
        dbc.conn.trans_commit();
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }

    return true;
}
//---------------------------------------------------------
//进行自动参数绑定的插入示例
inline bool ut_conn_base_insert_4(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);

        //预处理解析并进行参数的自动绑定
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)").auto_bind();
        q << 26 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;   //顺序给参数进行数据赋值
        q.exec().conn().trans_commit();                     //执行语句并提交

        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//进行自动参数绑定的批量插入示例
inline bool ut_conn_base_insert_5(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //解析带有参数绑定的语句,同时告知最大批量块深度
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)").auto_bind(2);
#if UT_DB==DB_ORA

        //给每个块深度对应的参数进行赋值
        q.bulk(0) << 27 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.bulk(1) << 37 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32767;
        q.exec().conn().trans_commit();                     //执行本次批量操作,并进行提交
        rt.tdd_assert(q.rows() == 2);

        //继续进行批量数据的绑定
        q.bulk(0) << 47 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.exec(1, true);                                    //告知真正绑定的数据深度,执行并提交
        rt.tdd_assert(q.rows() == 1);
#else
        //给每个块深度对应的参数进行赋值
        q << 27 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.exec().conn().trans_commit();                     //执行本次批量操作,并进行提交
        rt.tdd_assert(q.rows() == 1);

        q << 37 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32767;
        q.exec().conn().trans_commit();                     //执行本次批量操作,并进行提交
        rt.tdd_assert(q.rows() == 1);

        //继续进行批量数据的绑定
        q<< 47 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.exec(true);                                    //告知真正绑定的数据深度,执行并提交
        rt.tdd_assert(q.rows() == 1);
#endif
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
    return true;
}

//---------------------------------------------------------
//sql绑定参数解析示例
//---------------------------------------------------------
inline void ut_conn_base_sql_parse_1(rx_tdd_t &rt)
{
    rx_dbc::sql_param_parse_t<> sp;
    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") == NULL);
    rt.tdd_assert(sp.count == 2);
    rt.tdd_assert(strncmp(sp.segs[0].name, ":uvID", sp.segs[0].length) == 0);
    rt.tdd_assert(sp.segs[0].length == 5);
    rt.tdd_assert(strncmp(sp.segs[1].name, ":uUINT", sp.segs[1].length) == 0);
    rt.tdd_assert(sp.segs[1].length == 6);

    rt.tdd_assert(sp.ora_sql("select id,:se'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'b':se,':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and (UINT=:uUINT)") != NULL);
    rt.tdd_assert(sp.ora_sql("select id,'b' :se,':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and (UINT=:uUINT)") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se"",\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se\"\",\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT:se from tmp_dbc where id=:uvID and UINT=:uUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT:se from tmp_dbc where id=:uvID and UINT=: nUINT;") != NULL);
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT :se from tmp_dbc where id=:uvID and UINT=:uUINT;") == NULL);   //解析通过,但不符合sql语法
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1") == NULL);
    rt.tdd_assert(sp.count==0);

    char tmp[1024];
    rt.tdd_assert(sp.ora2mysql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1",tmp,sizeof(tmp)) != sizeof(tmp));

    rt.tdd_assert(sp.ora2mysql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;",tmp,sizeof(tmp)) != sizeof(tmp));

    rt.tdd_assert(sp.pg_sql("select id,UINT from tmp_dbc where UINT=$1 or UINT=$2::int8") == NULL);
    rt.tdd_assert(sp.count == 2);
    rt.tdd_assert(rx::st::strcmp(sp.segs[1].name, "$2::int8") == 0);

}
//---------------------------------------------------------
//进行数据库基础动作测试
inline void ut_conn_base_1(rx_tdd_t &rt)
{
    ut_conn utdb;
    rt.tdd_assert(utdb.check_conn());
    if (utdb.check_conn())
    {
        rt.tdd_assert(ut_conn_base_query_1(rt, utdb));
        
        //先清空测试表
        rt.tdd_assert(utdb.exec("delete from tmp_dbc")==1);
        rt.tdd_assert(utdb.records() == 0);

        rt.tdd_assert(ut_conn_base_insert_1(rt, utdb));
        rt.tdd_assert(utdb.records() == 1);

        rt.tdd_assert(ut_conn_base_insert_2(rt, utdb));
        rt.tdd_assert(utdb.records() == 2);

        rt.tdd_assert(ut_conn_base_insert_2b(rt, utdb));
        rt.tdd_assert(utdb.records() == 3);

        rt.tdd_assert(ut_conn_base_insert_2c(rt, utdb));
        rt.tdd_assert(utdb.records() == 5);

        rt.tdd_assert(ut_conn_base_insert_3(rt, utdb));
        rt.tdd_assert(utdb.records() == 8);

        rt.tdd_assert(ut_conn_base_insert_4(rt, utdb));
        rt.tdd_assert(utdb.records() == 9);

        rt.tdd_assert(ut_conn_base_insert_5(rt, utdb));
        rt.tdd_assert(utdb.records() == 12);

        for (int i = 0; i < 10; ++i)
            rt.tdd_assert(ut_conn_base_query_1(rt, utdb));

        rt.tdd_assert(ut_conn_base_query_2(rt, utdb));
    }
}

//---------------------------------------------------------
//db测试用例的入口
//---------------------------------------------------------
rx_tdd(ut_conn_base)
{
    //进行sql参数解析的测试
    ut_conn_base_sql_parse_1(*this);

    //进行底层功能的测试
    ut_conn_base_1(*this);

    //循环进行底层功能的测试
    for(int i=0;i<10;++i)
        ut_conn_base_1(*this);
}


#endif
