#ifndef	_RX_DBC_PGSQL_H_
#define	_RX_DBC_PGSQL_H_

//---------------------------------------------------------
#include "libpq-fe.h"                                       //����libpq�ӿ�,Ĭ����"3rd_deps\pgsql\include"
#include "dbc_comm/dbc_comm.h"                              //���������ʩ
#include "pg_type_sc.h"                                     //����pqsql��������OID

//---------------------------------------------------------
namespace rx_dbc
{
    #include "dbc_pgsql/base.h"                             //ʵ��һЩͨ�ù���
    #include "dbc_pgsql/conn.h"                             //ʵ�����ݿ�����
    #include "dbc_pgsql/field.h"                            //ʵ�ּ�¼�ֶβ�������

    #include "dbc_pgsql/param.h"                            //ʵ�����ΰ󶨲���
    #include "dbc_pgsql/stmt.h"                             //ʵ��sql����
    #include "dbc_pgsql/query.h"                            //ʵ�ּ�¼��ѯ���ʶ���

    namespace pgsql
    {
        //-------------------------------------------------
        //���������ռ��еĶ��⿪�����ͽ���ͳһ����,����dbc�ϲ㹤�ߵ�ʹ��.
        class type_t
        {
        public:
            typedef pgsql::env_option_t    env_option_t;
            typedef pgsql::error_info_t    error_info_t;
            typedef pgsql::datetime_t      datetime_t;

            typedef pgsql::conn_t          conn_t;
            typedef pgsql::param_t         param_t;
            typedef pgsql::stmt_t          stmt_t;
            typedef pgsql::field_t         field_t;
            typedef pgsql::query_t         query_t;
        };
    }
}

#endif
