#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"
#include "../rx_dbc_ora.h"
#include "rx_datetime_ex.h"

using namespace rx_dbc_ora;

//---------------------------------------------------------
//����ʹ�õĶ�������
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
//���ݿ�����
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
        printf("\n");
        return false;
    }
}

//---------------------------------------------------------
//�򵥲�ѯ
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
//�����󶨵Ĳ�ѯ
inline bool ut_ora_base_query_2(rx_tdd_t &rt, ut_ora &dbc)
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
//�󶨲�������(ʹ��query_t)
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
//�����󶨲���ʾ��(ʹ��stmt_t)
inline bool ut_ora_base_insert_2(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨵�������
        q(":nID", 2)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //ִ�����
        q.exec();
        //�ύ
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
//�����󶨲���ʾ��(ʹ��stmt_t)
inline bool ut_ora_base_insert_2b(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨲���
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //������
        q << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        //ִ�����
        q.exec();
        //�ύ
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
//�����󶨲���ʾ��(ʹ��stmt_t����ʾ����)
inline bool ut_ora_base_insert_2c(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        rt.tdd_assert(dbc.conn.ping());

        dbc.conn.trans_begin();

        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨲���
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //������
        q << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        //ִ�����
        q.exec();
        //�ٲ����°����ݵĳ���,�����ٴ�ִ�����
        q.exec();
        //�ύ
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        sword ec = 0;
        dbc.conn.trans_rollback(&ec);
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//���������ֶ���ʾ��
inline bool ut_ora_base_insert_3(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //�������в����󶨵����,ͬʱ��֪������������
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").manual_bind(2);

        //��ÿ������ȶ�Ӧ�Ĳ������а��븳ֵ
        q.bulk(0)(":nID", 2)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.bulk(1)(":nID", 3)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //ִ�б�����������
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //�����Ĭ����������ύ
        dbc.conn.trans_commit();

        //���������������ݵİ�
        q.bulk(0)(":nID", 4)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //��֪�����󶨵�������Ȳ�ִ�в���
        q.exec(1);
        rt.tdd_assert(q.rows() == 1);
        //�����Ĭ����������ύ
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
//�����Զ������󶨵Ĳ���ʾ��
inline bool ut_ora_base_insert_4(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);

        //Ԥ������������в������Զ���
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind();
        q << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;   //˳��������������ݸ�ֵ
        q.exec().conn().trans_commit();                     //ִ����䲢�ύ

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
//�����Զ������󶨵���������ʾ��
inline bool ut_ora_base_insert_5(rx_tdd_t &rt, ut_ora &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //�������в����󶨵����,ͬʱ��֪������������
        q.prepare("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind(2);

        //��ÿ������ȶ�Ӧ�Ĳ������и�ֵ
        q.bulk(0) << 2 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.bulk(1) << 3 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32769;
        q.exec().conn().trans_commit();                     //ִ�б�����������,�������ύ
        rt.tdd_assert(q.rows() == 2);

        //���������������ݵİ�
        q.bulk(0) << 4 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.exec(1, true);                                    //��֪�����󶨵��������,ִ�в��ύ
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
//sql�󶨲�������ʾ��
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
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT :se from tmp_dbc where id=:nID and UINT=:nUINT;") == NULL);   //����ͨ��,��������sql�﷨
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1") == NULL); 
    rt.tdd_assert(sp.count==0);

    char tmp[1024];
    rt.tdd_assert(sp.ora_mysql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1",tmp,sizeof(tmp)) != sizeof(tmp)); 

    rt.tdd_assert(sp.ora_mysql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;",tmp,sizeof(tmp)) != sizeof(tmp));

}
//---------------------------------------------------------
//�������ݿ������������
inline void ut_ora_base_1(rx_tdd_t &rt)
{
    ut_ora ora;
    if (ut_ora_base_conn(rt, ora))
    {
        rt.tdd_assert(ut_ora_base_insert_2c(rt, ora));

        rt.tdd_assert(ut_ora_base_query_1(rt, ora));

        rt.tdd_assert(ut_ora_base_insert_1(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_2(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_2b(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_2c(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_3(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_4(rt, ora));
        rt.tdd_assert(ut_ora_base_insert_5(rt, ora));

        for (int i = 0; i < 10; ++i)
            rt.tdd_assert(ut_ora_base_query_1(rt, ora));

        rt.tdd_assert(ut_ora_base_query_2(rt, ora));
    }
}

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

//DBC�¼�ί�ж�Ӧ�ĺ���ָ������;//����ֵ:<0����; 0�û�Ҫ�����; >0���,�������
inline int32_t ut_dbc_event_func_1(query_t &q, void *usrdat)
{
    ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
    q << dat.ID << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
    return 1;
}

inline void ut_ora_ext_1(rx_tdd_t &rt)
{
    //�������ݿ�����
    dbc_conn_t conn;
    conn.set_conn_param("20.0.2.106", "system", "sysdba");
    rt.tdd_assert(conn.schema_to("HYTPDTBILLDB"));

    //�������������
    ut_ins_dat_t dat;

    //�������ݿ⹦�ܶ���,��֪���ݰ󶨺��������ݶ���
    dbc_exec_t dbc(conn, ut_dbc_event_func_1, &dat);

    //�״�ִ��sql���,ʹ�ù���ʱ����������
    int rc=dbc("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
    rt.tdd_assert(rc>0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc(&dat);
    rt.tdd_assert(rc>0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc(NULL,&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
//ʹ��dbc_t��Ϊ�������ҵ����
class mydbc :public dbc_t
{
    //!!�ؼ�!!���в������ݵİ󶨶���;
    //����ֵ:<0����;0�û�Ҫ�����;>0���
    virtual int32_t on_bind_data(query_t &q, void *usrdat)
    {
        ut_ins_dat_t &dat = *(ut_ins_dat_t*)usrdat;
        q << dat.ID << dat.INT << dat.UINT << dat.STR << dat.DATE << dat.SHORT;
        return 1;
    }
public:
    mydbc(dbc_conn_t  &c) :dbc_t(c) {}
};
inline void ut_ora_ext_2(rx_tdd_t &rt)
{
    //�������ݿ�����
    dbc_conn_t conn;
    conn.set_conn_param("20.0.2.106", "system", "sysdba");
    rt.tdd_assert(conn.schema_to("HYTPDTBILLDB"));

    //�������������
    ut_ins_dat_t dat;

    //�������ݿ⹦�ܶ���,��֪���ݰ󶨺��������ݶ���
    mydbc dbc(conn);

    //�״�ִ��sql���,����֪����
    int rc = dbc("insert into tmp_dbc(id,int,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)", &dat);
    rt.tdd_assert(rc>0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc.exec(&dat);
    rt.tdd_assert(rc>0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc.exec(&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
rx_tdd(ut_dtl_array)
{
    ut_ora_ext_1(*this);
    ut_ora_ext_2(*this);

    ut_ora_base_sql_parse_1(*this);
    ut_ora_base_1(*this);
    for(int i=0;i<10;++i)
        ut_ora_base_1(*this);
}

#endif
