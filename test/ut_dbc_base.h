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
    //���ڵײ㹦�ܶ���������������ݿ��������
    //---------------------------------------------------------
    //����ʹ�õĶ�������
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
    //���ݿ�����
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
//�򵥲�ѯ
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
//�����󶨵Ĳ�ѯ
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
//�󶨲�������(ʹ��query_t)
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
//�����󶨲���ʾ��(ʹ��stmt_t)
inline bool ut_dbc_base_insert_2(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨵�������
        q(":nID", 21)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
inline bool ut_dbc_base_insert_2b(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨲���
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //������
        q << 24 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
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
inline bool ut_dbc_base_insert_2c(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        rt.tdd_assert(dbc.conn.ping());

        dbc.conn.trans_begin();

        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨲���
        q(":nID")(":nINT")(":nUINT")(":sSTR")(":dDATE")(":nSHORT");
        //������
        q << 22 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        //ִ�����
        q.exec();

        q << 23 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        //�ٴ�ִ�����
        q.exec();
        //�ύ
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
//���������ֶ���ʾ��
inline bool ut_dbc_base_insert_3(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //�������в����󶨵����,ͬʱ��֪������������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").manual_bind(2);

        //��ÿ������ȶ�Ӧ�Ĳ������а��븳ֵ
        q.bulk(0)(":nID", 25)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
        q.bulk(1)(":nID", 35)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":nSHORT", 32769);
        //ִ�б�����������
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //�����Ĭ����������ύ
        dbc.conn.trans_commit();

        //���������������ݵİ�
        q.bulk(0)(":nID", 45)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
inline bool ut_dbc_base_insert_4(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);

        //Ԥ������������в������Զ���
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind();
        q << 26 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;   //˳��������������ݸ�ֵ
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
inline bool ut_dbc_base_insert_5(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //�������в����󶨵����,ͬʱ��֪������������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)").auto_bind(2);

        //��ÿ������ȶ�Ӧ�Ĳ������и�ֵ
        q.bulk(0) << 27 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
        q.bulk(1) << 37 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32769;
        q.exec().conn().trans_commit();                     //ִ�б�����������,�������ύ
        rt.tdd_assert(q.rows() == 2);

        //���������������ݵİ�
        q.bulk(0) << 47 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32769;
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
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT :se from tmp_dbc where id=:nID and UINT=:nUINT;") == NULL);   //����ͨ��,��������sql�﷨
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1") == NULL);
    rt.tdd_assert(sp.count==0);

    char tmp[1024];
    rt.tdd_assert(sp.ora2mysql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1",tmp,sizeof(tmp)) != sizeof(tmp));

    rt.tdd_assert(sp.ora2mysql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:nID and UINT=:nUINT;",tmp,sizeof(tmp)) != sizeof(tmp));
}
//---------------------------------------------------------
//�������ݿ������������
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
    virtual void on_connect(conn_t& conn, const conn_param_t &param) { conn.schema_to("SCOTT"); }
};

//---------------------------------------------------------
//DBC�¼�ί�ж�Ӧ�ĺ���ָ������;//����ֵ:<0����; 0�û�Ҫ�����; >0���,�������
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
    //�������ݿ⹦�ܶ��󲢰󶨺���ָ��
    dbc_t dbc(conn, ut_dbc_event_func_1);

    //ִ��sql���,ʹ�ø���������
    int rc=dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)",&dat);
    rt.tdd_assert(rc > 0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc("",&dat);
    rt.tdd_assert(rc>0);

    //����sql���,����������
    rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
    rt.tdd_assert(rc >= 0);

    //�������ݶ�����ٴ�ִ��
    ++dat.ID;
    rc = dbc(NULL,&dat);
    rt.tdd_assert(rc>0);
}
//---------------------------------------------------------
//ʹ��dbc_t��Ϊ�������ҵ����
class mydbc :public dbc_t
{
    //��֪��ִ�е�ҵ��SQL���
    virtual const char* on_sql() { return "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)"; }
    //�������ݰ�;����ֵ:<0����;0�û�Ҫ�����;>0������������
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
    //�������ݿ⹦�ܶ���,��֪���ݰ󶨺��������ݶ���
    mydbc dbc(conn);
    //�״�ִ��sql���,����֪����
    int rc = dbc("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
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
inline void ut_dbc_ext_a3(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    //ID���ı�,Ӧ�û����Ψһ��Լ���ĳ�ͻ
    --dat.ID;

    //����ģʽ,ʹ��ҵ���ܵ���ʱ����ִ��ҵ�������䲢��������
    rt.tdd_assert( mydbc(conn).action(&dat) < 0);
    rt.tdd_assert(conn.last_err()==DBEC_OCI_UNIQUECONST);
}
//---------------------------------------------------------
//ʹ��dbc_t��Ϊ�������ҵ����,���Բ�ѯ��ȡ���
class mydbc4 :public dbc_t
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
        printf("id(%d),int(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",
            q["id"].as_ulong(), q["int"].as_long(), q["uint"].as_ulong(),
            q["str"].as_string(), q["mdate"].as_string(), q["short"].as_long());
        return 1;
    }
public:
    mydbc4(dbc_conn_t  &c) :dbc_t(c) {}
};
//---------------------------------------------------------
//����������ȡ����
inline void ut_dbc_ext_a4(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
    mydbc4 dbc(conn);
    dbc.action(&dat);
    while (dbc.fetch(3) > 0);
}
//---------------------------------------------------------
//���зǰ�sql����
inline void ut_dbc_ext_a5(rx_tdd_t &rt, dbc_conn_t &conn, ut_ins_dat_t &dat)
{
    ++dat.ID;
    const char* sql = "insert into tmp_dbc(id,intn,uint,str,mdate,short) values(123456789,-123,123,'insert',to_date('2000-01-01 13:14:20','yyyy-MM-dd HH24:mi:ss'),1)";
    rt.tdd_assert(tiny_dbc_t(conn).action(sql) > 0);
}
//---------------------------------------------------------
//���ϲ��װ��db����������������������
inline void ut_dbc_ext_a(rx_tdd_t &rt)
{
    //�������������
    ut_ins_dat_t dat;

    //�������ݿ�����
    my_conn_t conn;
    conn.set_conn_param("20.0.2.106", "system", "sysdba");

    //ִ�в��Թ���
    ut_dbc_ext_a1(rt, conn, dat);
    ut_dbc_ext_a2(rt, conn, dat);
    ut_dbc_ext_a3(rt, conn, dat);
    ut_dbc_ext_a4(rt, conn, dat);
    ut_dbc_ext_a5(rt, conn, dat);
}

//---------------------------------------------------------
//db�������������
//---------------------------------------------------------
rx_tdd(ut_dbc_base)
{
    //�����ϲ��װ�Ĳ���
    ut_dbc_ext_a(*this);

    //����sql���������Ĳ���
    ut_dbc_base_sql_parse_1(*this);

    //���еײ㹦�ܵĲ���
    ut_dbc_base_1(*this);

    //ѭ�����еײ㹦�ܵĲ���
    for(int i=0;i<10;++i)
        ut_dbc_base_1(*this);
}
*/
//�����󶨲���ʾ��(ʹ��stmt_t)
inline bool ut_dbc_base_insert_2(rx_tdd_t &rt, ut_dbc &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:nID,:nINT,:nUINT,:sSTR,:dDATE,:nSHORT)");
        //�󶨵�������
        q(":nID", 21)(":nINT", -155905152)(":nUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":nSHORT", 32769);
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
            //utdb.conn.exec("insert into tmp_dbc(id,str)values(8,'hello%s')", "��");
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
