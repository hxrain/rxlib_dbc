#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "oci.h"                        //����OCI�ӿ�,Ĭ����"3rd_deps\ora\include"

#define RX_DEF_ALLOC_USE_STD 0          //�Ƿ�ʹ�ñ�׼malloc���������в���(�����ڼ���ڴ�й¶)

#include "rx_cc_macro.h"                //��������궨��
#include "rx_assert.h"                  //�������
#include "rx_str_util.h"                //��������ַ�������
#include "rx_str_tiny.h"
#include "rx_mem_alloc_cntr.h"          //�����ڴ��������
#include "rx_datetime.h"                //��������ʱ�书��
#include "rx_dtl_buff.h"                //���뻺��������
#include "rx_dtl_array_ex.h"            //�����������

#include "comm/sql_param_parse.h"       //SQL�󶨲��������ֽ�������

#include "ora/comm.h"                   //ʵ��һЩͨ�ù���
#include "ora/conn.h"                   //ʵ�����ݿ�����

#include "ora/param.h"                  //ʵ�����ΰ󶨲���
#include "ora/stmt.h"                   //ʵ��sql����

#include "ora/field.h"                  //ʵ�ּ�¼�ֶβ�������
#include "ora/query.h"                  //ʵ�ּ�¼��ѯ���ʶ���

#endif


