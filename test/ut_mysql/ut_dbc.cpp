
#define RX_DEF_ALLOC_USE_STD 1          //是否使用标准malloc分配器进行测试(可用于检测内存泄露)

#include "../../rx_dbc_mysql.h"
#include "../../rx_dbc_ora.h"


#include "../ut_dbc_base.h"

#include "stdlib.h"
#include "rx_tdd.h"

#if RX_CC==RX_CC_VC
    #include <crtdbg.h>
#endif

int main()
{
#if RX_CC==RX_CC_VC
    //_CrtSetBreakAlloc(74);
#endif

    rx_tdd_run();

    getchar();

#if RX_CC==RX_CC_VC
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}

