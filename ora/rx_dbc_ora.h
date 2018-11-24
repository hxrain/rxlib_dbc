#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "oci.h"                        //����OCI�ӿ�

#define RX_DEF_ALLOC_USE_STD 1

#include "rx_cc_macro.h"                //��������궨��
#include "rx_assert.h"                  //�������
#include "rx_str_util.h"                //��������ַ�������
#include "rx_str_tiny.h"
#include "rx_mem_alloc_cntr.h"          //�����ڴ��������
#include "rx_datetime.h"                //��������ʱ��
#include "rx_dtl_array_ex.h"            //�����������

#include "ora_base_comm.h"              //ʵ��һЩͨ�ù���
#include "ora_part_conn.h"              //ʵ�����ݿ�����

#include "ora_base_param.h"             //ʵ�����ΰ󶨲���
#include "ora_part_stmt.h"              //ʵ��sql����

#include "ora_base_field.h"             //ʵ�ּ�¼�ֶβ�������
#include "ora_part_query.h"             //ʵ�ּ�¼��ѯ���ʶ���

#endif


