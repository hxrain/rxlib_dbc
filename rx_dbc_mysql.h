#ifndef	_RX_DBC_MYSQL_H_
#define	_RX_DBC_MYSQL_H_

//---------------------------------------------------------
#include "mysql.h"                                          //����mysql�ӿ�,Ĭ����"3rd_deps\mysql\include"
#include "errmsg.h"                                         //����mysql������Ϣ
#include "mysqld_error.h"                                   //����mysql��չ������Ϣ
//---------------------------------------------------------
#include "rx_cc_macro.h"                                    //��������궨��
#include "rx_assert.h"                                      //�������
#include "rx_str_util_std.h"                                //��������ַ�������
#include "rx_str_util_ex.h"                                 //������չ�ַ�������
#include "rx_str_tiny.h"                                    //���붨���ַ�������
#include "rx_mem_alloc_cntr.h"                              //�����ڴ��������
#include "rx_datetime.h"                                    //��������ʱ�书��
#include "rx_dtl_buff.h"                                    //���뻺��������
#include "rx_dtl_array_ex.h"                                //�����������
#include "rx_ct_delegate.h"                                 //����ί�й���
#include "rx_datetime_ex.h"                                 //��������ʱ�书����չ
#include "rx_cc_atomic.h"                                   //����ԭ�ӱ�������

//---------------------------------------------------------
namespace rx_dbc
{
    #include "dbc_comm/dbc_parse.h"                         //SQL�󶨲��������ֽ�������
    #include "dbc_comm/dbc_type.h"                          //ͳһ���Ͷ���
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
