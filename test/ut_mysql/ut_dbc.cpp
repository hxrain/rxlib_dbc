
#include "../ut_dbc_comm.h"
#include "../ut_dbc_base.h"
#include "../ut_dbc_util.h"

#if RX_CC==RX_CC_VC
    #include <crtdbg.h>
#endif

int main()
{
    env_t mysql_env;

    rx_tdd_run();

    getchar();

    return 0;
}

