#ifndef _UT_RX_DBC_ORA_COMM_H_
#define _UT_RX_DBC_ORA_COMM_H_

#include "rx_tdd.h"

#define DB_ORA      1
#define DB_MYSQL    2
#define DB_PGSQL    3

#ifndef UT_DB
    #define UT_DB DB_ORA
#endif

#if UT_DB==DB_ORA
    #include "../rx_dbc_ora.h"
    #include "../rx_dbc_util.h"

    typedef rx_dbc::dbc_conn_t<rx_dbc::ora::type_t> dbc_conn_t;
    typedef rx_dbc::dbc_ext_t<rx_dbc::ora::type_t> dbc_ext_t;
    typedef rx_dbc::dbc_tiny_t<rx_dbc::ora::type_t> dbc_tiny_t;
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

    */
#endif

#if UT_DB==DB_MYSQL
    #include "../rx_dbc_mysql.h"
    #include "../rx_dbc_util.h"

    typedef rx_dbc::dbc_conn_t<rx_dbc::mysql::type_t> dbc_conn_t;
    typedef rx_dbc::dbc_ext_t<rx_dbc::mysql::type_t> dbc_ext_t;
    typedef rx_dbc::dbc_tiny_t<rx_dbc::mysql::type_t> dbc_tiny_t;
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

#if UT_DB==DB_PGSQL
#include "../rx_dbc_pgsql.h"
#include "../rx_dbc_util.h"

    typedef rx_dbc::dbc_conn_t<rx_dbc::pgsql::type_t> dbc_conn_t;
    typedef rx_dbc::dbc_ext_t<rx_dbc::pgsql::type_t> dbc_ext_t;
    typedef rx_dbc::dbc_tiny_t<rx_dbc::pgsql::type_t> dbc_tiny_t;
    using namespace rx_dbc::pgsql;
    /*
    CREATE TABLE "public"."tmp_dbc" (
        "id" int4 NOT NULL,
        "intn" int4,
        "uint" int8,
        "str" varchar(255) COLLATE "default",
        "mdate" timestamp(6),
        "short" int2,
        CONSTRAINT "tmp_dbc_pkey" PRIMARY KEY ("id")
    )WITH (OIDS=FALSE);
    ALTER TABLE "public"."tmp_dbc" OWNER TO "postgres";
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
            strcpy(conn_param.host, "20.0.2.82");
            strcpy(conn_param.user, "system");
            strcpy(conn_param.pwd, "sysdba");
            strcpy(conn_param.db, "oradb");
            conn_param.port = 1521;
#elif UT_DB==DB_PGSQL
            strcpy(conn_param.host, "10.110.38.201");
            strcpy(conn_param.user, "postgres");
            strcpy(conn_param.pwd, "postgres");
            strcpy(conn_param.db, "postgres");
            conn_param.port = 5432;

            rx_assert(pg_data_type_by_name("int2") == PG_DATA_TYPE_INT2);
            rx_assert(pg_data_type_by_name("int4") == PG_DATA_TYPE_INT4);
            rx_assert(pg_data_type_by_name("int8") == PG_DATA_TYPE_INT8);
            rx_assert(pg_data_type_by_name("int") == PG_DATA_TYPE_INT4);
            rx_assert(pg_data_type_by_name("float4") == PG_DATA_TYPE_FLOAT4);
            rx_assert(pg_data_type_by_name("float8") == PG_DATA_TYPE_FLOAT8);
            rx_assert(pg_data_type_by_name("float") == PG_DATA_TYPE_FLOAT8);
            rx_assert(pg_data_type_by_name("numeric") == PG_DATA_TYPE_NUMERIC);
            rx_assert(pg_data_type_by_name("text") == PG_DATA_TYPE_TEXT);
            rx_assert(pg_data_type_by_name("varchar") == PG_DATA_TYPE_VARCHAR);
            rx_assert(pg_data_type_by_name("date") == PG_DATA_TYPE_DATE);
            rx_assert(pg_data_type_by_name("time") == PG_DATA_TYPE_TIME);
            rx_assert(pg_data_type_by_name("timestamp") == PG_DATA_TYPE_TIMESTAMP);
            rx_assert(pg_data_type_by_name("timestamptz") == PG_DATA_TYPE_TIMESTAMPTZ);

#elif UT_DB==DB_MYSQL
            strcpy(conn_param.host, "20.0.3.130");
            strcpy(conn_param.user, "root");
            strcpy(conn_param.pwd, "root");
            strcpy(conn_param.db, "mysql");
            conn_param.port = 3306;

            rx_assert(rx_dbc::get_sql_type("SELECT") == rx_dbc::ST_SELECT);
            rx_assert(rx_dbc::get_sql_type("UPDATE") == rx_dbc::ST_UPDATE);
            rx_assert(rx_dbc::get_sql_type("UPSERT") == rx_dbc::ST_UPDATE);
            rx_assert(rx_dbc::get_sql_type("DELETE") == rx_dbc::ST_DELETE);
            rx_assert(rx_dbc::get_sql_type("CREATE") == rx_dbc::ST_CREATE);
            rx_assert(rx_dbc::get_sql_type("DROP")   == rx_dbc::ST_DROP);
            rx_assert(rx_dbc::get_sql_type("ALTER")  == rx_dbc::ST_ALTER);
            rx_assert(rx_dbc::get_sql_type("BEGIN")  == rx_dbc::ST_BEGIN);
            rx_assert(rx_dbc::get_sql_type("SET ")   == rx_dbc::ST_SET);
            rx_assert(rx_dbc::get_sql_type("INSERT") == rx_dbc::ST_INSERT);
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
