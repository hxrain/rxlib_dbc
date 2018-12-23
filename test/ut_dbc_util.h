#ifndef _UT_RX_DBC_ORA_UTIL_H_
#define _UT_RX_DBC_ORA_UTIL_H_

#include "rx_tdd.h"
#include "ut_dbc_comm.h"

//---------------------------------------------------------
//�����ϲ��װ����db���ʲ���
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
//��չӦ�ò����Ӷ���,�����ӽ�������Ҫ�л��û�ģʽ
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
//DBC�¼�ί�ж�Ӧ�ĺ���ָ������;//����ֵ:<0����; 0�û�Ҫ�����; >0���,�������
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
    //�������ݿ⹦�ܶ��󲢰󶨺���ָ��
    dbc_ext_t dbc(conn, ut_conn_event_func_1);

    //ִ��sql���,ʹ�ø���������
    int rc=dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)",&dat);
    rt.tdd_assert(rc > 0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);

    //����sql���,����������
    rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
    rt.tdd_assert(rc >= 0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc(NULL,&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
//ʹ��dbc_ext_t��Ϊ�������ҵ����
class mydbc :public dbc_ext_t
{
    //��֪��ִ�е�ҵ��SQL���
    virtual const char* on_sql() { return "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)"; }
    //�������ݰ�;����ֵ:<0����;0�û�Ҫ�����;>0������������
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
    //�������ݿ⹦�ܶ���,��֪���ݰ󶨺��������ݶ���
    mydbc dbc(conn);
    //�״�ִ��sql���,����֪����
    int rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
    rt.tdd_assert(rc>=0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
inline void ut_conn_ext_a3(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //ID���ı�,Ӧ�û����Ψһ��Լ���ĳ�ͻ
    --dat.ID;

    //����ģʽ,ʹ��ҵ���ܵ���ʱ����ִ��ҵ�������䲢��������
    rt.tdd_assert( mydbc(conn).action(&dat) < 0);
    rt.tdd_assert(conn.last_err()== rx_dbc::DBEC_DB_UNIQUECONST);
}
//---------------------------------------------------------
//ʹ��dbc_ext_t��Ϊ�������ҵ����,���Բ�ѯ��ȡ���
class mydbc4 :public dbc_ext_t
{
    //-----------------------------------------------------
    //��֪��ִ�е�ҵ��SQL���
    virtual const char* on_sql() { return "select * from tmp_dbc where str=:sSTR"; }
    //-----------------------------------------------------
    //�������ݰ�;����ֵ:<0����;0�û�Ҫ�����;>0���
    virtual int32_t on_bind_data(query_t &q, void *usrdat)
    {
        if (!usrdat) return 0;
        ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
        q << dat.STR ;
        return 1;
    }
    //-----------------------------------------------------
    //��ȡ�����,���ʵ�ǰ������;����ֵ:<0����;0�û�Ҫ�����;>0���
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
//����������ȡ����
inline void ut_conn_ext_a4(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
    mydbc4 dbc(conn);
    dbc.action(&dat);
    while (dbc.fetch(3) > 0);
}
//---------------------------------------------------------
//���зǰ�sql����
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
//���ϲ��װ��db����������������������
inline void ut_conn_ext_a(rx_tdd_t &rt)
{
    //�������Ӳ���
    ut_conn cp;

    //�������������
    ut_ins_dat_t dat;

    //�������ݿ�����
    my_conn_t conn;
    conn.set_conn_param(cp.conn_param);

    //����ղ��Ա�
    rt.tdd_assert(conn.exec("delete from tmp_dbc"));

    //ִ�в��Թ���
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
//db�������������
//---------------------------------------------------------
rx_tdd(ut_conn_util)
{
    //�����ϲ��װ�Ĳ���
    ut_conn_ext_a(*this);
}


#endif
