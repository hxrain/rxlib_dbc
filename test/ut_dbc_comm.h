#ifndef _UT_RX_DBC_ORA_COMM_H_
#define _UT_RX_DBC_ORA_COMM_H_

#include "rx_tdd.h"

#define DB_ORA      1
#define DB_MYSQL    2

#ifndef UT_DB
    #define UT_DB DB_ORA
#endif

#if UT_DB==DB_ORA
    #include "../rx_dbc_ora.h"
    #include "../rx_dbc_util.h"

    typedef rx_dbc::dbc_conn_t<rx_dbc::ora::type_t> dbc_conn_t;
    typedef rx_dbc::dbc_t<rx_dbc::ora::type_t> dbc_t;
    typedef rx_dbc::tiny_dbc_t<rx_dbc::ora::type_t> tiny_dbc_t;
    using namespace rx_dbc::ora;
    /*
    CREATE TABLE SCOTT.TMP_DBC (
	    ID NUMBER NOT NULL ENABLE,
	    INTN NUMBER (*, 0),
	    UINT NUMBER,
	    STR VARCHAR2 (255),
	    MDATE DATE,
	    SHORT NUMBER (*, 0),
	    PRIMARY KEY (ID) USING INDEX PCTFREE 10 INITRANS 2 MAXTRANS 255 COMPUTE STATISTICS STORAGE (
		    INITIAL 65536 NEXT 1048576 MINEXTENTS 1 MAXEXTENTS 2147483645 PCTINCREASE 0 FREELISTS 1 FREELIST GROUPS 1 BUFFER_POOL DEFAULT FLASH_CACHE DEFAULT CELL_FLASH_CACHE DEFAULT
	    ) TABLESPACE "USERS" ENABLE
    ) SEGMENT CREATION IMMEDIATE PCTFREE 10 PCTUSED 40 INITRANS 1 MAXTRANS 255 NOCOMPRESS LOGGING STORAGE (
	    INITIAL 65536 NEXT 1048576 MINEXTENTS 1 MAXEXTENTS 2147483645 PCTINCREASE 0 FREELISTS 1 FREELIST GROUPS 1 BUFFER_POOL DEFAULT FLASH_CACHE DEFAULT CELL_FLASH_CACHE DEFAULT
    ) TABLESPACE "USERS";

    CREATE UNIQUE INDEX SCOTT.TMP_DBC_IDX_ID ON SCOTT.TMP_DBC (ID) PCTFREE 10 INITRANS 2 MAXTRANS 255 COMPUTE STATISTICS STORAGE (
	    INITIAL 65536 NEXT 1048576 MINEXTENTS 1 MAXEXTENTS 2147483645 PCTINCREASE 0 FREELISTS 1 FREELIST GROUPS 1 BUFFER_POOL DEFAULT FLASH_CACHE DEFAULT CELL_FLASH_CACHE DEFAULT
    ) TABLESPACE "USERS";
    */
#endif

#if UT_DB==DB_MYSQL
    #include "../rx_dbc_mysql.h"
    #include "../rx_dbc_util.h"

    typedef rx_dbc::dbc_conn_t<rx_dbc::mysql::type_t> dbc_conn_t;
    typedef rx_dbc::dbc_t<rx_dbc::mysql::type_t> dbc_t;
    typedef rx_dbc::tiny_dbc_t<rx_dbc::mysql::type_t> tiny_dbc_t;
    using namespace rx_dbc::mysql;
    /*
    CREATE TABLE tmp_dbc (
      ID bigint(20) unsigned NOT NULL,
      INTN int(10) DEFAULT NULL,
      UINT int(10) unsigned DEFAULT NULL,
      STR varchar(255) DEFAULT NULL,
      MDATE datetime DEFAULT NULL,
      SHORT smallint(6) DEFAULT NULL,
      PRIMARY KEY (ID)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    */
#endif

    //---------------------------------------------------------
    //基于底层功能对象进行连接与数据库操作测试
    //---------------------------------------------------------
    //测试使用的对象容器
    typedef struct ut_conn
    {
        rx_dbc::conn_param_t conn_param;
        conn_t conn;

        ut_conn()
        {
#if UT_DB==DB_ORA
            strcpy(conn_param.host, "20.0.2.106");
            strcpy(conn_param.user, "system");
            strcpy(conn_param.pwd, "sysdba");
            strcpy(conn_param.db, "oradb");
            conn_param.port = 1521;
#elif UT_DB==DB_MYSQL
            strcpy(conn_param.host, "20.0.3.130");
            strcpy(conn_param.user, "root");
            strcpy(conn_param.pwd, "root");
            strcpy(conn_param.db, "mysql");
            conn_param.port = 3306;

            rx_assert(get_sql_type("SELECT") == rx_dbc::ST_SELECT);
            rx_assert(get_sql_type("UPDATE") == rx_dbc::ST_UPDATE);
            rx_assert(get_sql_type("UPSERT") == rx_dbc::ST_UPDATE);
            rx_assert(get_sql_type("DELETE") == rx_dbc::ST_DELETE);
            rx_assert(get_sql_type("CREATE") == rx_dbc::ST_CREATE);
            rx_assert(get_sql_type("DROP")   == rx_dbc::ST_DROP);
            rx_assert(get_sql_type("ALTER")  == rx_dbc::ST_ALTER);
            rx_assert(get_sql_type("BEGIN")  == rx_dbc::ST_BEGIN);
            rx_assert(get_sql_type("SET ")   == rx_dbc::ST_SET);
            rx_assert(get_sql_type("INSERT") == rx_dbc::ST_INSERT);
#endif
        }
        bool check_conn()
        {
            try {
                if (!conn.is_valid())
                {
                    conn.open(conn_param);
#if UT_DB==DB_ORA
                    conn.schema_to("SCOTT");
#endif
                }
                return true;
            }
            catch (error_info_t &e)
            {
                printf(e.c_str(conn_param));
                printf("\n");
                return false;
            }
        }

        int records()
        {
            try {
                if (!check_conn())
                    return -2;
                return query_t(conn).query_records("tmp_dbc");
            }
            catch (error_info_t &e)
            {
                printf(e.c_str(conn_param));
                printf("\n");
                return -1;
            }
        }
        int exec(const char* sql)
        {
            try {
                if (!check_conn())
                    return -2;
                conn.exec(sql);
                conn.trans_commit();
                return 1;
            }
            catch (error_info_t &e)
            {
                conn.trans_rollback();
                printf(e.c_str(conn_param));
                printf("\n");
                return -1;
            }
        }
    }ut_conn;

#endif
