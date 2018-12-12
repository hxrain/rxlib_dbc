
//#include "../ut_dbc_comm.h"
//#include "../ut_dbc_base.h"
//#include "../ut_dbc_util.h"

#include "../../rx_dbc_pgsql.h"
#include "rx_tdd.h"

#if RX_CC==RX_CC_VC
    #include <crtdbg.h>
#endif

using namespace rx_dbc::pgsql;

//---------------------------------------------------------
void ut_pgsql_base_conn_0(rx_tdd_t &rt, conn_t &conn, rx_dbc::conn_param_t &conn_param)
{
    strcpy(conn_param.host, "10.110.38.208");
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
void ut_pgsql_base_conn_1(rx_tdd_t &rt, conn_t &conn, rx_dbc::conn_param_t &conn_param)
{
    try {
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
}


int main()
{
    rx_tdd_run();

    getchar();

    return 0;
}

