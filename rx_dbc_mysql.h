#ifndef	_RX_DBC_MYSQL_H_
#define	_RX_DBC_MYSQL_H_

//---------------------------------------------------------
#include "mysql.h"                                          //����mysql�ӿ�,Ĭ����"3rd_deps\mysql\include"
#include "errmsg.h"                                         //����mysql������Ϣ
#include "mysqld_error.h"                                   //����mysql��չ������Ϣ
#include "dbc_comm/dbc_comm.h"                              //���������ʩ
//---------------------------------------------------------
namespace rx_dbc
{
    #include "dbc_mysql/base.h"                             //ʵ��һЩͨ�ù���
    #include "dbc_mysql/conn.h"                             //ʵ�����ݿ�����
    #include "dbc_mysql/field.h"                            //ʵ�ּ�¼�ֶβ�������
    #include "dbc_mysql/param.h"                            //ʵ�����ΰ󶨲���
    #include "dbc_mysql/stmt.h"                             //ʵ��sql����
    #include "dbc_mysql/query.h"                            //ʵ�ּ�¼��ѯ���ʶ���

    namespace mysql
    {
        //-------------------------------------------------
        //���������ռ��еĶ��⿪�����ͽ���ͳһ����,����dbc�ϲ㹤�ߵ�ʹ��.
        class type_t
        {
        public:
            typedef mysql::env_option_t    env_option_t;
            typedef mysql::error_info_t    error_info_t;
            typedef mysql::datetime_t       datetime_t;

            typedef mysql::conn_t           conn_t;
            typedef mysql::param_t          param_t;
            typedef mysql::stmt_t           stmt_t;
            typedef mysql::field_t          field_t;
            typedef mysql::query_t          query_t;
        };
    }
}

#endif
