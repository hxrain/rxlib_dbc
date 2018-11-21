#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "sdk\include\oci.h"            //引入OCI接口
#include <string>

#include "rx_cc_macro.h"                //引入基础宏定义
#include "rx_assert.h"                  //引入断言
#include "rx_str_util.h"                //引入基础字符串功能
#include "rx_mem_alloc_cntr.h"          //引入内存分配容器
#include "rx_datetime.h"                //引入日期时间
#include "rx_dtl_array_ex.h"            //引入别名数组

#include "ora_base_comm.h"              //实现一些通用功能
#include "ora_part_conn.h"              //实现数据库连接

#include "ora_base_param.h"             //实现语句段绑定参数
#include "ora_part_cmd.h"               //实现SQL语句段

#include "ora_base_field.h"             //实现记录字段操作对象
#include "ora_part_query.h"             //实现记录查询访问对象

#endif


