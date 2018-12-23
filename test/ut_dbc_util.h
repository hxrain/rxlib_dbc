#ifndef _UT_RX_DBC_ORA_UTIL_H_
#define _UT_RX_DBC_ORA_UTIL_H_

#include "rx_tdd.h"
#include "ut_dbc_comm.h"

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
    virtual void on_connect(dbc_conn_t::conn_t& conn, const rx_dbc::conn_param_t &param)
    { 
#if UT_DB==DB_ORA
        conn.schema_to("SCOTT"); 
#endif
    }
};

//---------------------------------------------------------
//DBC事件委托对应的函数指针类型;//返回值:<0错误; 0用户要求放弃; >0完成,批量深度
inline int32_t ut_conn_event_func_1(query_t &q, void *usrdat)
{
    if (!usrdat) return 0;
    ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
    q << dat.ID << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
    return 1;
}
//---------------------------------------------------------
inline void ut_conn_ext_a1(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //定义数据库功能对象并绑定函数指针
    dbc_ext_t dbc(conn, ut_conn_event_func_1);

    //执行sql语句,使用给定的数据
    int rc=dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)",&dat);
    rt.tdd_assert(rc > 0);

    //更换数据对象后再次执行
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);

    //解析sql语句,不操作数据
    rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
    rt.tdd_assert(rc >= 0);

    //更换数据对象后再次执行
    ++dat.ID;
    rc = dbc(NULL,&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
//使用dbc_ext_t作为基类进行业务处理
class mydbc :public dbc_ext_t
{
    //告知待执行的业务SQL语句
    virtual const char* on_sql() { return "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)"; }
    //处理数据绑定;返回值:<0错误;0用户要求放弃;>0绑定批量块的深度
    virtual int32_t on_bind_data(query_t &q, void *usrdat)
    {
        if (!usrdat) return 0;
        ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
        q << dat.ID++ << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
        return 1;
    }
public:
    mydbc(dbc_conn_t  &c) :dbc_ext_t(c) {}
};
//---------------------------------------------------------
inline void ut_conn_ext_a2(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //定义数据库功能对象,告知数据绑定函数与数据对象
    mydbc dbc(conn);
    //首次执行sql语句,不告知数据
    int rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
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
inline void ut_conn_ext_a3(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //ID不改变,应该会出现唯一性约束的冲突
    --dat.ID;

    //极简模式,使用业务功能的临时对象执行业务定义的语句并处理数据
    rt.tdd_assert( mydbc(conn).action(&dat) < 0);
    rt.tdd_assert(conn.last_err()== rx_dbc::DBEC_DB_UNIQUECONST);
}
//---------------------------------------------------------
//使用dbc_ext_t作为基类进行业务处理,测试查询提取结果
class mydbc4 :public dbc_ext_t
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
        printf("fetcount=%d:id(%d),intn(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",q.fetched(),
            q["id"].as_uint(), q["intn"].as_int(), q["uint"].as_uint(),
            q["str"].as_string(), q["mdate"].as_string(), q["short"].as_int());
        return 1;
    }
public:
    mydbc4(dbc_conn_t  &c) :dbc_ext_t(c) {}
};
//---------------------------------------------------------
//进行数据提取操作
inline void ut_conn_ext_a4(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
    mydbc4 dbc(conn);
    dbc.action(&dat);
    while (dbc.fetch(3) > 0);
}
//---------------------------------------------------------
//进行非绑定sql处理
inline void ut_conn_ext_a5(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
#if UT_DB==DB_ORA
    const char* sql = "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(123456789,-123,123,'insert',to_date('2000-01-01 13:14:20','yyyy-MM-dd HH24:mi:ss'),1)";
#else
    const char* sql = "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(123456789,-123,123,'insert','2000-01-01 13:14:20',1)";
#endif
    rt.tdd_assert(dbc_tiny_t(conn).action(sql) > 0);
}
//---------------------------------------------------------
//对上层封装的db操作进行真正的驱动测试
inline void ut_conn_ext_a(rx_tdd_t &rt)
{
    //借用连接参数
    ut_conn cp;

    //定义待处理数据
    ut_ins_dat_t dat;

    //定义数据库连接
    my_conn_t conn;
    conn.set_conn_param(cp.conn_param);

    //先清空测试表
    rt.tdd_assert(conn.exec("delete from tmp_dbc"));

    //执行测试过程
    ut_conn_ext_a1(rt, conn, dat);
    int rc=cp.records();
    int rc1=conn.records("tmp_dbc");
    rt.tdd_assert(rc == 3 && rc1==rc);

    ut_conn_ext_a2(rt, conn, dat);
    rc=cp.records();
    rc1=conn.records("tmp_dbc");
    rt.tdd_assert(rc == 5 && rc1==rc);

    ut_conn_ext_a3(rt, conn, dat);
    rc=cp.records();
    rc1=conn.records("tmp_dbc");
    rt.tdd_assert(rc == 5 && rc1==rc);

    ut_conn_ext_a4(rt, conn, dat);
    rc=cp.records();
    rc1=conn.records("tmp_dbc");
    rt.tdd_assert(rc == 5 && rc1==rc);

    ut_conn_ext_a5(rt, conn, dat);
    rc=cp.records();
    rc1=conn.records("tmp_dbc");
    rt.tdd_assert(rc == 6 && rc1==rc);
}

//---------------------------------------------------------
//db测试用例的入口
//---------------------------------------------------------
rx_tdd(ut_conn_util)
{
    //进行上层封装的测试
    ut_conn_ext_a(*this);
}


#endif
