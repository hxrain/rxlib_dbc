#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "oci.h"                        //����OCI�ӿ�,Ĭ����"3rd_deps\ora\include"

#define RX_DEF_ALLOC_USE_STD 0          //�Ƿ�ʹ�ñ�׼malloc���������в���(�����ڼ���ڴ�й¶)

#include "rx_cc_macro.h"                //��������궨��
#include "rx_assert.h"                  //�������
#include "rx_str_util_std.h"            //��������ַ�������
#include "rx_str_util_ex.h"             //������չ�ַ�������
#include "rx_str_tiny.h"                //���붨���ַ�������
#include "rx_mem_alloc_cntr.h"          //�����ڴ��������
#include "rx_datetime.h"                //��������ʱ�书��
#include "rx_dtl_buff.h"                //���뻺��������
#include "rx_dtl_array_ex.h"            //�����������

#include "dbc_comm/sql_param_parse.h"   //SQL�󶨲��������ֽ�������

#include "dbc_ora/comm.h"               //ʵ��һЩͨ�ù���
#include "dbc_ora/conn.h"               //ʵ�����ݿ�����
#include "dbc_ora/param.h"              //ʵ�����ΰ󶨲���
#include "dbc_ora/stmt.h"               //ʵ��sql����
#include "dbc_ora/field.h"              //ʵ�ּ�¼�ֶβ�������
#include "dbc_ora/query.h"              //ʵ�ּ�¼��ѯ���ʶ���

#endif


