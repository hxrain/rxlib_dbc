
#define RX_DEF_ALLOC_USE_STD 1          //�Ƿ�ʹ�ñ�׼malloc���������в���(�����ڼ���ڴ�й¶)

#include "../ut_dbc_comm.h"
#include "../ut_dbc_base.h"
#include "../ut_dbc_util.h"

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

