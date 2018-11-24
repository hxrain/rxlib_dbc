
#include "stdlib.h"
#include "rx_tdd.h"
#include "../ut_ora_base.h"

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

