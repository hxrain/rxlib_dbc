#ifndef	_RX_DBC_MYSQL_H_
#define	_RX_DBC_MYSQL_H_

#include "mysql.h"                                          //����mysql�ӿ�,Ĭ����"3rd_deps\mysql\include"
#include "errmsg.h"
#include "mysqld_error.h"

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

#include "dbc_comm/dbc_parse.h"                             //SQL�󶨲��������ֽ�������
#include "dbc_comm/dbc_type.h"                              //ͳһ���Ͷ���

#include "dbc_mysql/base.h"                                 //ʵ��һЩͨ�ù���
#include "dbc_mysql/conn.h"                                 //ʵ�����ݿ�����
#include "dbc_mysql/field.h"                                //ʵ�ּ�¼�ֶβ�������
#include "dbc_mysql/param.h"                                //ʵ�����ΰ󶨲���
#include "dbc_mysql/stmt.h"                                 //ʵ��sql����
#include "dbc_mysql/query.h"                                //ʵ�ּ�¼��ѯ���ʶ���

#include "rx_dbc_comm.h"                                    //����ͳһ���ϲ㹦�ܷ�װ

#endif
