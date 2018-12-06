#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"

#define DB_ORA      1
#define DB_MYSQL    2

#ifndef UT_DB
    #define UT_DB DB_ORA
#endif

#if UT_DB==DB_ORA
    #include "../rx_dbc_ora.h"
    using namespace rx_dbc_ora;
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
    using namespace rx_dbc_mysql;
#endif

    //---------------------------------------------------------
    //基于底层功能对象进行连接与数据库操作测试
    //---------------------------------------------------------
    //测试使用的对象容器
    typedef struct ut_dbc
    {
        conn_param_t conn_param;
        conn_t conn;

        ut_dbc()
        {
#if UT_DB==DB_ORA
            strcpy(conn_param.host, "20.0.2.106");
            strcpy(conn_param.user, "system");
            strcpy(conn_param.pwd, "sysdba");
            strcpy(conn_param.db, "oradb");
#elif UT_DB==DB_MYSQL
            strcpy(conn_param.host, "20.0.3.130");
            strcpy(conn_param.user, "root");
            strcpy(conn_param.pwd, "root");
            strcpy(conn_param.db, "mysql");
#endif
        }
    }ut_dbc;

    //---------------------------------------------------------
    //数据库连接
    inline bool ut_dbc_base_conn(rx_tdd_t &rt, ut_dbc &dbc)
    {
        try {
            dbc.conn.open(dbc.conn_param);
#if UT_DB==DB_ORA
            dbc.conn.schema_to("SCOTT");
#endif
            return true;
        }
        catch (error_info_t &e)
        {
            printf(e.c_str(dbc.conn_param));
            printf("\n");
            return false;
        }
    }
/*

//---------------------------------------------------------
//简单查询
inline bool ut_dbc_base_query_1(rx_tdd_t &rt, ut_dbc &dbc)
{
    try {
        query_t q(dbc.conn);

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
inline bool ut_dbc_base_query_2(rx_tdd_t &rt, ut_dbc &dbc)
{
    try {
        query_t q(dbc.conn);

        q.prepare("select * from tmp_dbc where str=:sSTR");
        q(":sSTR","2");

        for (q.exec(); !q.eof(); q.next())
        {
            printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
                q["id"].as_long(), q["int"].as_long(), q["uint"].as_ulong(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_long());
        }

        q.exec("delete from tmp_dbc where str!='str'").conn().trans_commit();

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
inline bool ut_dbc_base_insert_1(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        query_t q(dbc.conn);

        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        q(":nID", 20)(":nINT", -155905152)(":nUINT",(uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
inline bool ut_dbc_base_insert_2(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定单条参数
        q(":nID", 21)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
inline bool ut_dbc_base_insert_2b(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定参数
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //绑定数据
        q << 24 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
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
//参数绑定插入示例(使用stmt_t与显示事务)
inline bool ut_dbc_base_insert_2c(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        rt.tdd_assert(dbc.conn.ping());

        dbc.conn.trans_begin();

        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定参数
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //绑定数据
        q << 22 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        //执行语句
        q.exec();

        q << 23 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
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
inline bool ut_dbc_base_insert_3(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //解析带有参数绑定的语句,同时告知最大批量块深度
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").manual_bind(2);

        //给每个块深度对应的参数进行绑定与赋值
        q.bulk(0)(":nID", 25)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.bulk(1)(":nID", 35)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //执行本次批量操作
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //必须对默认事务进行提交
        dbc.conn.trans_commit();

        //继续进行批量数据的绑定
        q.bulk(0)(":nID", 45)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
inline bool ut_dbc_base_insert_4(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);

        //预处理解析并进行参数的自动绑定
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind();
        q << 26 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;   //顺序给参数进行数据赋值
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
inline bool ut_dbc_base_insert_5(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //解析带有参数绑定的语句,同时告知最大批量块深度
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind(2);

        //给每个块深度对应的参数进行赋值
        q.bulk(0) << 27 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.bulk(1) << 37 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32769;
        q.exec().conn().trans_commit();                     //执行本次批量操作,并进行提交
        rt.tdd_assert(q.rows() == 2);

        //继续进行批量数据的绑定
        q.bulk(0) << 47 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.exec(1, true);                                    //告知真正绑定的数据深度,执行并提交
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
//---------------------------------------------------------
inline void ut_dbc_base_sql_parse_1(rx_tdd_t &rt)
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
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1") == NULL);
    rt.tdd_assert(sp.count==0);

    char tmp[1024];
    rt.tdd_assert(sp.ora2mysql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1",tmp,sizeof(tmp)) != sizeof(tmp));

    rt.tdd_assert(sp.ora2mysql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;",tmp,sizeof(tmp)) != sizeof(tmp));
}
//---------------------------------------------------------
//进行数据库基础动作测试
inline void ut_dbc_base_1(rx_tdd_t &rt)
{
    ut_dbc utdb;
    if (ut_dbc_base_conn(rt, utdb))
    {
        rt.tdd_assert(ut_dbc_base_query_1(rt, utdb));

        rt.tdd_assert(ut_dbc_base_insert_1(rt, utdb));
        rt.tdd_assert(ut_dbc_base_insert_2(rt, utdb));
        rt.tdd_assert(ut_dbc_base_insert_2b(rt, utdb));
        rt.tdd_assert(ut_dbc_base_insert_2c(rt, utdb));
        rt.tdd_assert(ut_dbc_base_insert_3(rt, utdb));
        rt.tdd_assert(ut_dbc_base_insert_4(rt, utdb));
        rt.tdd_assert(ut_dbc_base_insert_5(rt, utdb));

        for (int i = 0; i < 10; ++i)
            rt.tdd_assert(ut_dbc_base_query_1(rt, utdb));

        rt.tdd_assert(ut_dbc_base_query_2(rt, utdb));
    }
}

//---------------------------------------------------------
//基于上层封装进行db访问测试
//---------------------------------------------------------
typedef struct ut_ins_dat_t
{
    uint32_t    ID;
    int32_t     INT;
    uint32_t    UINT;
    char        STR[20];
    char        DATE[20];
    int16_t     SHORT;

    ut_ins_dat_t()
    {
        ID = (uint32_t)rx_time();
        INT = -155905152;
        UINT = 2155905152u;
        sprintf(STR,"%x",ID);
        rx_iso_datetime(DATE);
        SHORT = (int16_t)ID;
    }
}ut_ins_dat_t;
//---------------------------------------------------------
//扩展应用层连接对象,在连接建立后需要切换用户模式
class my_conn_t :public dbc_conn_t
{
    virtual void on_connect(conn_t& conn, const conn_param_t &param) { conn.schema_to("SCOTT"); }
};

//---------------------------------------------------------
//DBC事件委托对应的函数指针类型;//返回值:<0错误; 0用户要求放弃; >0完成,批量深度
inline int32_t ut_dbc_event_func_1(query_t &q, void *usrdat)
{
    if (!usrdat) return 0;
    ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
    q << dat.ID << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
    return 1;
}
//---------------------------------------------------------
inline void ut_dbc_ext_a1(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //定义数据库功能对象并绑定函数指针
    dbc_t dbc(conn, ut_dbc_event_func_1);

    //执行sql语句,使用给定的数据
    int rc=dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)",&dat);
    rt.tdd_assert(rc > 0);

    //更换数据对象后再次执行
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);

    //解析sql语句,不操作数据
    rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
    rt.tdd_assert(rc >= 0);

    //更换数据对象后再次执行
    ++dat.ID;
    rc = dbc(NULL,&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
//使用dbc_t作为基类进行业务处理
class mydbc :public dbc_t
{
    //告知待执行的业务SQL语句
    virtual const char* on_sql() { return "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)"; }
    //处理数据绑定;返回值:<0错误;0用户要求放弃;>0绑定批量块的深度
    virtual int32_t on_bind_data(query_t &q, void *usrdat)
    {
        if (!usrdat) return 0;
        ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
        q.bulk(0) << dat.ID++ << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
        q.bulk(1) << dat.ID++ << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
        return 2;
    }
public:
    mydbc(dbc_conn_t  &c) :dbc_t(c) {}
};
//---------------------------------------------------------
inline void ut_dbc_ext_a2(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //定义数据库功能对象,告知数据绑定函数与数据对象
    mydbc dbc(conn);
    //首次执行sql语句,不告知数据
    int rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
    rt.tdd_assert(rc>=0);

    //更换数据对象后再次执行
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);

    //更换数据对象后再次执行
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
inline void ut_dbc_ext_a3(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //ID不改变,应该会出现唯一性约束的冲突
    --dat.ID;

    //极简模式,使用业务功能的临时对象执行业务定义的语句并处理数据
    rt.tdd_assert( mydbc(conn).action(&dat) < 0);
    rt.tdd_assert(conn.last_err()==DBEC_OCI_UNIQUECONST);
}
//---------------------------------------------------------
//使用dbc_t作为基类进行业务处理,测试查询提取结果
class mydbc4 :public dbc_t
{
    //-----------------------------------------------------
    //告知待执行的业务SQL语句
    virtual const char* on_sql() { return "select * from tmp_dbc where str=:sSTR"; }
    //-----------------------------------------------------
    //处理数据绑定;返回值:<0错误;0用户要求放弃;>0完成
    virtual int32_t on_bind_data(query_t &q, void *usrdat)
    {
        if (!usrdat) return 0;
        ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
        q << dat.STR ;
        return 1;
    }
    //-----------------------------------------------------
    //获取到结果,访问当前行数据;返回值:<0错误;0用户要求放弃;>0完成
    virtual int32_t on_row(query_t &q, void *usrdat)
    {
        printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
            q["id"].as_ulong(), q["int"].as_long(), q["uint"].as_ulong(),
            q["str"].as_string(), q["mdate"].as_string(), q["short"].as_long());
        return 1;
    }
public:
    mydbc4(dbc_conn_t  &c) :dbc_t(c) {}
};
//---------------------------------------------------------
//进行数据提取操作
inline void ut_dbc_ext_a4(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
    mydbc4 dbc(conn);
    dbc.action(&dat);
    while (dbc.fetch(3) > 0);
}
//---------------------------------------------------------
//进行非绑定sql处理
inline void ut_dbc_ext_a5(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
    const char* sql = "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(123456789,-123,123,'insert',to_date('2000-01-01 13:14:20','yyyy-MM-dd HH24:mi:ss'),1)";
    rt.tdd_assert(tiny_dbc_t(conn).action(sql) > 0);
}
//---------------------------------------------------------
//对上层封装的db操作进行真正的驱动测试
inline void ut_dbc_ext_a(rx_tdd_t &rt)
{
    //定义待处理数据
    ut_ins_dat_t dat;

    //定义数据库连接
    my_conn_t conn;
    conn.set_conn_param("20.0.2.106", "system", "sysdba");

    //执行测试过程
    ut_dbc_ext_a1(rt, conn, dat);
    ut_dbc_ext_a2(rt, conn, dat);
    ut_dbc_ext_a3(rt, conn, dat);
    ut_dbc_ext_a4(rt, conn, dat);
    ut_dbc_ext_a5(rt, conn, dat);
}

//---------------------------------------------------------
//db测试用例的入口
//---------------------------------------------------------
rx_tdd(ut_dbc_base)
{
    //进行上层封装的测试
    ut_dbc_ext_a(*this);

    //进行sql参数解析的测试
    ut_dbc_base_sql_parse_1(*this);

    //进行底层功能的测试
    ut_dbc_base_1(*this);

    //循环进行底层功能的测试
    for(int i=0;i<10;++i)
        ut_dbc_base_1(*this);
}
*/
//参数绑定插入示例(使用stmt_t)
inline bool ut_dbc_base_insert_2(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //预处理解析
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //绑定单条参数
        q(":nID", 21)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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



void tmp_ut(rx_tdd_t &rt)
{
    rt.tdd_assert(get_sql_type("SELECT") == ST_SELECT);
    rt.tdd_assert(get_sql_type("UPDATE") == ST_UPDATE);
    rt.tdd_assert(get_sql_type("UPSERT") == ST_UPDATE);
    rt.tdd_assert(get_sql_type("DELETE") == ST_DELETE);
    rt.tdd_assert(get_sql_type("CREATE") == ST_CREATE);
    rt.tdd_assert(get_sql_type("DROP") == ST_DROP);
    rt.tdd_assert(get_sql_type("ALTER") == ST_ALTER);
    rt.tdd_assert(get_sql_type("BEGIN") == ST_BEGIN);
    rt.tdd_assert(get_sql_type("SET ") == ST_SET);
    rt.tdd_assert(get_sql_type("INSERT") == ST_INSERT);

    ut_dbc utdb;
    try {
        if (ut_dbc_base_conn(rt, utdb))
        {
            //utdb.conn.exec("insert into tmp_dbc(id,str)values(8,'hello%s')","\xe5\x93\x88");
            //utdb.conn.exec("insert into tmp_dbc(id,str)values(8,'hello%s')", "哈");
            //utdb.conn.trans_commit();

            ut_dbc_base_insert_2(rt,utdb);
        }
    }
    catch (error_info_t &e)
    {
        printf(e.c_str(utdb.conn_param));
        printf("\n");
    }
}

rx_tdd(ut_dbc_tmp)
{
    tmp_ut(*this);
}

#endif
