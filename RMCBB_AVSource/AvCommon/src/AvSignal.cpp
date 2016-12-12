#include "AvConfig.h"
#include "AvDebug.h"
#include "AvSignal.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "CommonDebug.h"

void Av_signal_handle(int signal_num)
{
    if(signal_num == SIGPIPE)
    {
        DEBUG_ERROR( "Av_signal_handle SIGPIPE\n");
    }
    else if(signal_num == SIGCHLD)
    {
        int status = 0;

        waitpid(-1, &status, 0);

        DEBUG_ERROR( "Av_signal_handle children process exit: %d\n", status);
    }
    else
    {
        DEBUG_ERROR( "Av_signal_handle process exit with signal:%d\n", signal_num);
        
        exit(-1);
    }
}


int Register_av_signal_handle()
{
#if defined(_CORE_DOWN_FILE_)
    signal(SIGPIPE, Av_signal_handle);
#else
    for(int signal_num = 0; signal_num < 32; signal_num ++)
    {
        signal(signal_num, Av_signal_handle);
    }
#endif

    return 0;
}



