#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

//-----------------------------------------------------
//OCI�����жϺ��Ĭ�ϴ��䳬ʱӦ������20sec����,���г�ʱ�ж�ʱ��Ҫע��
#include "oci.h"                                            //����OCI�ӿ�,Ĭ����"3rd_deps\ora\include"
#include "dbc_comm/dbc_comm.h"                              //���������ʩ

//-----------------------------------------------------
namespace rx_dbc
{
    #include "dbc_ora/base.h"                               //ʵ��һЩͨ�ù���
    #include "dbc_ora/conn.h"                               //ʵ�����ݿ�����
    #include "dbc_ora/param.h"                              //ʵ�����ΰ󶨲���
    #include "dbc_ora/stmt.h"                               //ʵ��sql����
    #include "dbc_ora/field.h"                              //ʵ�ּ�¼�ֶβ�������
    #include "dbc_ora/query.h"                              //ʵ�ּ�¼��ѯ���ʶ���

    namespace ora
    {
        //-------------------------------------------------
        //���������ռ��еĶ��⿪�����ͽ���ͳһ����,����dbc�ϲ㹤�ߵ�ʹ��.
        class type_t
        {
        public:
            typedef env_option_t    env_option_t;
            typedef error_info_t    error_info_t;
            typedef datetime_t      datetime_t;

            typedef conn_t          conn_t;
            typedef param_t         param_t;
            typedef stmt_t          stmt_t;
            typedef field_t         field_t;
            typedef query_t         query_t;
        };
    }
}

#endif
