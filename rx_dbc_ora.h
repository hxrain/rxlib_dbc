#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "oci.h"                        //引入OCI接口,默认在"3rd_deps\ora\include"

#define RX_DEF_ALLOC_USE_STD 0          //是否使用标准malloc分配器进行测试(可用于检测内存泄露)

#include "rx_cc_macro.h"                //引入基础宏定义
#include "rx_assert.h"                  //引入断言
#include "rx_str_util.h"                //引入基础字符串功能
#include "rx_str_tiny.h"
#include "rx_mem_alloc_cntr.h"          //引入内存分配容器
#include "rx_datetime.h"                //引入日期时间功能
#include "rx_dtl_buff.h"                //引入缓冲区功能
#include "rx_dtl_array_ex.h"            //引入别名数组

#include "comm/sql_param_parse.h"       //SQL绑定参数的名字解析功能

#include "ora/comm.h"                   //实现一些通用功能
#include "ora/conn.h"                   //实现数据库连接

#include "ora/param.h"                  //实现语句段绑定参数
#include "ora/stmt.h"                   //实现sql语句段

#include "ora/field.h"                  //实现记录字段操作对象
#include "ora/query.h"                  //实现记录查询访问对象

#endif


