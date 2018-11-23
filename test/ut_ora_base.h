#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"
#include "../ora/rx_dbc_ora.h"

using namespace rx_dbc_ora;

inline void ut_ora_base_1(rx_tdd_t &rt)
{
    conn_param_t conn_param;
    strcpy(conn_param.host, "20.0.2.106");
    strcpy(conn_param.user, "system");
    strcpy(conn_param.pwd, "sysdba");
    strcpy(conn_param.sid, "oradb");

    conn_t conn;

    try {
        conn.open(conn_param);
        conn.schema_to("HYTPDTBILLDB");

        query_t q(conn);

        for (q.exec("select * from tmp_dbc"); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),date(%s),short(%d)\n",
                q.field("id").as_long(), q.field("int").as_long(), q.field("uint").as_ulong(), 
                q.field("str").as_string(), q.field("date").as_string(), q.field("short").as_long());
        }
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(conn_param));
    }

    getchar();
}

rx_tdd(ut_dtl_array)
{
    ut_ora_base_1(*this);
}

#endif