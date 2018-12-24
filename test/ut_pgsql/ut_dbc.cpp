
#include "../ut_dbc_comm.h"
#include "../ut_dbc_base.h"
#include "../ut_dbc_util.h"

#include "../../rx_dbc_pgsql.h"
#include "rx_tdd.h"

#if RX_CC==RX_CC_VC
    #include <crtdbg.h>
#endif

using namespace rx_dbc::pgsql;

//---------------------------------------------------------
void ut_pgsql_base_conn_0(rx_tdd_t &rt, conn_t &conn, rx_dbc::conn_param_t &conn_param)
{
    strcpy(conn_param.host, "10.110.38.201");
    strcpy(conn_param.user, "postgres");
    strcpy(conn_param.pwd, "postgres");
    strcpy(conn_param.db, "postgres");
    conn_param.port = 5432;

    try {
        conn.open(conn_param);
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(conn_param));
        printf("\n");
    }
}
//---------------------------------------------------------
//参数绑定插入示例(使用stmt_t)
inline bool ut_conn_base_insert_2(rx_tdd_t &rt, conn_t &conn, rx_dbc::conn_param_t &conn_param)
{

    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //绑定单条参数
        q(":uvID", 21)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //执行语句
        q.exec();
        //提交
        conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);

        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values($1::int,$2::int,$3::int8,$4::text,$5::timestamp,$6::int2)");
        //绑定单条参数
        q("$1::int", 22)("$2::int", -155905152)("$3::int8", (uint32_t)2155905152u)("$4::text", "2")("$5::timestamp", cur_time_str)("$6::int2", 32767);
        //执行语句
        q.exec();
        //提交
        conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        conn.trans_rollback();
        printf(e.c_str(conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
void ut_pgsql_base_conn_1(rx_tdd_t &rt, conn_t &conn, rx_dbc::conn_param_t &conn_param)
{
    try {
        //conn.tmp_exec("SELECT  NOW()::date,NOW()::time,NOW()::abstime,NOW()::timestamp,NOW()::timestamptz");
        rt.tdd_assert(conn.ping());
        conn.exec("insert into tmp_dbc(id,intn) values(3,0)");
    }
    catch (error_info_t &e)
    {
        conn.trans_rollback();
        printf(e.c_str(conn_param));
        printf("\n");
    }

    try {
        conn.trans_begin();
        conn.exec("insert into tmp_dbc(id,intn) values(4,0)");
        conn.trans_commit();

        conn.trans_begin();
        conn.exec("insert into tmp_dbc(id,intn) values(5,0)");
        conn.exec("insert into tmp_dbc(id,intn) values(6,0)");
        conn.trans_commit();
    }
    catch (error_info_t &e)
    {
        conn.trans_rollback();
        printf(e.c_str(conn_param));
        printf("\n");
    }
}

//---------------------------------------------------------
rx_tdd(ut_conn_base)
{
    rx_dbc::conn_param_t conn_param;
    conn_t conn;

    ut_pgsql_base_conn_0(*this, conn, conn_param);
    ut_pgsql_base_conn_1(*this, conn, conn_param);

    ut_conn_base_insert_2(*this, conn, conn_param);
}
//---------------------------------------------------------

int main()
{
    rx_tdd_run();

    getchar();

    return 0;
}

