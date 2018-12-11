
#include "../ut_dbc_comm.h"
#include "../ut_dbc_base.h"
#include "../ut_dbc_util.h"

#if RX_CC==RX_CC_VC
    #include <crtdbg.h>
#endif

int main()
{
    rx_tdd_run();

    getchar();

    return 0;
}

