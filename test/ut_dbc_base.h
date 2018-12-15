#ifndef _UT_RX_DBC_ORA_BASE_H_
#define _UT_RX_DBC_ORA_BASE_H_

#include "rx_tdd.h"
#include "ut_dbc_comm.h"

//---------------------------------------------------------
//�򵥲�ѯ
inline bool ut_conn_base_query_1(rx_tdd_t &rt, ut_conn &dbc)
{
    try {
        query_t q(dbc.conn);

        for (q.exec("select * from tmp_dbc"); !q.eof(); q.next())
        {
            printf("fetch_count=%d:id(%d),intn(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",q.fetched(),
                q["id"].as_int(), q["intn"].as_int(), q["uint"].as_uint(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_int());
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
inline bool ut_conn_base_query_2(rx_tdd_t &rt, ut_conn &dbc)
{
    try {
        query_t q(dbc.conn);

        q.prepare("select * from tmp_dbc where str=:sSTR");
        q(":sSTR","2");

        for (q.exec(); !q.eof(); q.next())
        {
            printf("fc=%d:id(%d),intn(%d),uint(%u),str(%s),mdate(%s),short(%d)\n",q.fetched(),
                q["id"].as_int(), q["intn"].as_int(), q["uint"].as_uint(),
                q["str"].as_string(), q["mdate"].as_string(), q["short"].as_int());
        }

        //q.exec("delete from tmp_dbc where str!='str'").conn().trans_commit();

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
inline bool ut_conn_base_insert_1(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        query_t q(dbc.conn);

        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        q(":uvID", 20)(":iINT", -155905152)(":uUINT",(uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        q.exec();
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//�����󶨲���ʾ��(ʹ��stmt_t)
inline bool ut_conn_base_insert_2(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //�󶨵�������
        q(":uvID", 21)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //ִ�����
        q.exec();
        //�ύ
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//�����󶨲���ʾ��(ʹ��stmt_t)
inline bool ut_conn_base_insert_2b(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //�󶨲���
        q(":uvID")(":iINT")(":uUINT")(":sSTR")(":dDATE")(":iSHORT");
        //������
        q << 24 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        //ִ�����
        q.exec();
        //�ύ
        dbc.conn.trans_commit();
        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//�����󶨲���ʾ��(ʹ��stmt_t����ʾ����)
inline bool ut_conn_base_insert_2c(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        rt.tdd_assert(dbc.conn.ping());

        dbc.conn.trans_begin();

        stmt_t q(dbc.conn);
        //Ԥ�������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)");
        //�󶨲���
        q(":uvID")(":iINT")(":uUINT")(":sSTR")(":dDATE")(":iSHORT");
        //������
        q << 22 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        //ִ�����
        q.exec();

        q << 23 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
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
inline bool ut_conn_base_insert_3(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //�������в����󶨵����,ͬʱ��֪������������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)").manual_bind(2);
#if UT_DB==DB_ORA
        //��ÿ������ȶ�Ӧ�Ĳ������а��븳ֵ
        q.bulk(0)(":uvID", 25)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        q.bulk(1)(":uvID", 35)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //ִ�б�����������
        q.exec();
        rt.tdd_assert(q.rows() == 2);
        //�����Ĭ����������ύ
        dbc.conn.trans_commit();

        //���������������ݵİ�
        q.bulk(0)(":uvID", 45)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767);
        //��֪�����󶨵�������Ȳ�ִ�в���
        q.exec(1);
        rt.tdd_assert(q.rows() == 1);
#else
        q(":uvID", 25)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767).exec();
        rt.tdd_assert(q.rows() == 1);

        q(":uvID", 35)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "3")(":dDATE", cur_time_str)(":iSHORT", 32767).exec();
        rt.tdd_assert(q.rows() == 1);
        //�ȶ�Ĭ����������ύ
        dbc.conn.trans_commit();

        //���������������ݵİ�
        q(":uvID", 45)(":iINT", -155905152)(":uUINT", (uint32_t)2155905152u)(":sSTR", "2")(":dDATE", cur_time_str)(":iSHORT", 32767).exec();
        rt.tdd_assert(q.rows() == 1);

#endif
        //�ٶ�Ĭ����������ύ
        dbc.conn.trans_commit();
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }

    return true;
}
//---------------------------------------------------------
//�����Զ������󶨵Ĳ���ʾ��
inline bool ut_conn_base_insert_4(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);

        //Ԥ������������в������Զ���
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)").auto_bind();
        q << 26 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;   //˳��������������ݸ�ֵ
        q.exec().conn().trans_commit();                     //ִ����䲢�ύ

        rt.tdd_assert(q.rows() == 1);
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
}
//---------------------------------------------------------
//�����Զ������󶨵���������ʾ��
inline bool ut_conn_base_insert_5(rx_tdd_t &rt, ut_conn &dbc)
{
    char cur_time_str[20];
    rx_iso_datetime(cur_time_str);
    try {
        stmt_t q(dbc.conn);
        //�������в����󶨵����,ͬʱ��֪������������
        q.prepare("insert into tmp_dbc(id,intn,uint,str,mdate,short) values(:uvID,:iINT,:uUINT,:sSTR,:dDATE,:iSHORT)").auto_bind(2);
#if UT_DB==DB_ORA

        //��ÿ������ȶ�Ӧ�Ĳ������и�ֵ
        q.bulk(0) << 27 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.bulk(1) << 37 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32767;
        q.exec().conn().trans_commit();                     //ִ�б�����������,�������ύ
        rt.tdd_assert(q.rows() == 2);

        //���������������ݵİ�
        q.bulk(0) << 47 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.exec(1, true);                                    //��֪�����󶨵��������,ִ�в��ύ
        rt.tdd_assert(q.rows() == 1);
#else
        //��ÿ������ȶ�Ӧ�Ĳ������и�ֵ
        q << 27 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.exec().conn().trans_commit();                     //ִ�б�����������,�������ύ
        rt.tdd_assert(q.rows() == 1);

        q << 37 << -155905152 << (uint32_t)2155905152u << "3" << cur_time_str << 32767;
        q.exec().conn().trans_commit();                     //ִ�б�����������,�������ύ
        rt.tdd_assert(q.rows() == 1);

        //���������������ݵİ�
        q<< 47 << -155905152 << (uint32_t)2155905152u << "2" << cur_time_str << 32767;
        q.exec(true);                                    //��֪�����󶨵��������,ִ�в��ύ
        rt.tdd_assert(q.rows() == 1);
#endif
        return true;
    }
    catch (error_info_t &e)
    {
        dbc.conn.trans_rollback();
        printf(e.c_str(dbc.conn_param));
        printf("\n");
        return false;
    }
    return true;
}

//---------------------------------------------------------
//sql�󶨲�������ʾ��
//---------------------------------------------------------
inline void ut_conn_base_sql_parse_1(rx_tdd_t &rt)
{
    rx_dbc::sql_param_parse_t<> sp;
    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") == NULL);
    rt.tdd_assert(sp.count == 2);
    rt.tdd_assert(strncmp(sp.segs[0].name, ":uvID", sp.segs[0].length) == 0);
    rt.tdd_assert(sp.segs[0].length == 5);
    rt.tdd_assert(strncmp(sp.segs[1].name, ":uUINT", sp.segs[1].length) == 0);
    rt.tdd_assert(sp.segs[1].length == 6);

    rt.tdd_assert(sp.ora_sql("select id,:se'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'b':se,':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and (UINT=:uUINT)") != NULL);
    rt.tdd_assert(sp.ora_sql("select id,'b' :se,':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and (UINT=:uUINT)") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se"",\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT") == NULL);
    rt.tdd_assert(sp.count == 3);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',:se\"\",\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'b',':g:\":H:\":i:',\"STR\",':\" : a\":\" : INT\"',UINT:se from tmp_dbc where id=:uvID and UINT=:uUINT;") != NULL);

    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT:se from tmp_dbc where id=:uvID and UINT=: nUINT;") != NULL);
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT :se from tmp_dbc where id=:uvID and UINT=:uUINT;") == NULL);   //����ͨ��,��������sql�﷨
    rt.tdd_assert(sp.ora_sql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1") == NULL);
    rt.tdd_assert(sp.count==0);

    char tmp[1024];
    rt.tdd_assert(sp.ora2mysql("select id,'\"',UINT from tmp_dbc where id=1 and UINT=1",tmp,sizeof(tmp)) != sizeof(tmp));

    rt.tdd_assert(sp.ora2mysql("select id,'b',':g:\":H:\":i:',:se,\"STR\",':\" : a\":\" : INT\"',UINT from tmp_dbc where id=:uvID and UINT=:uUINT;",tmp,sizeof(tmp)) != sizeof(tmp));

    rt.tdd_assert(sp.pg_sql("select id,UINT from tmp_dbc where UINT=$1 or UINT=$2::int8") == NULL);
    rt.tdd_assert(sp.count == 2);
    rt.tdd_assert(rx::st::strcmp(sp.segs[1].name, "$2::int8") == 0);

}
//---------------------------------------------------------
//�������ݿ������������
inline void ut_conn_base_1(rx_tdd_t &rt)
{
    ut_conn utdb;
    rt.tdd_assert(utdb.check_conn());
    if (utdb.check_conn())
    {
        rt.tdd_assert(ut_conn_base_query_1(rt, utdb));
        
        //����ղ��Ա�
        rt.tdd_assert(utdb.exec("delete from tmp_dbc")==1);
        rt.tdd_assert(utdb.records() == 0);

        rt.tdd_assert(ut_conn_base_insert_1(rt, utdb));
        rt.tdd_assert(utdb.records() == 1);

        rt.tdd_assert(ut_conn_base_insert_2(rt, utdb));
        rt.tdd_assert(utdb.records() == 2);

        rt.tdd_assert(ut_conn_base_insert_2b(rt, utdb));
        rt.tdd_assert(utdb.records() == 3);

        rt.tdd_assert(ut_conn_base_insert_2c(rt, utdb));
        rt.tdd_assert(utdb.records() == 5);

        rt.tdd_assert(ut_conn_base_insert_3(rt, utdb));
        rt.tdd_assert(utdb.records() == 8);

        rt.tdd_assert(ut_conn_base_insert_4(rt, utdb));
        rt.tdd_assert(utdb.records() == 9);

        rt.tdd_assert(ut_conn_base_insert_5(rt, utdb));
        rt.tdd_assert(utdb.records() == 12);

        for (int i = 0; i < 10; ++i)
            rt.tdd_assert(ut_conn_base_query_1(rt, utdb));

        rt.tdd_assert(ut_conn_base_query_2(rt, utdb));
    }
}

//---------------------------------------------------------
//db�������������
//---------------------------------------------------------
rx_tdd(ut_conn_base)
{
    //����sql���������Ĳ���
    ut_conn_base_sql_parse_1(*this);

    //���еײ㹦�ܵĲ���
    ut_conn_base_1(*this);

    //ѭ�����еײ㹦�ܵĲ���
    for(int i=0;i<10;++i)
        ut_conn_base_1(*this);
}


#endif
