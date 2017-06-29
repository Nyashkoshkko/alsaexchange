#include "alsaexchange.h"
#include <windef.h> /* Part of the Wine header files */

int WINAPI alsaexchange_init_linuxcall(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount))
{
    return alsaexchange_init(FillOutputBuffers_ptr);
}

int WINAPI alsaexchange_exit_linuxcall()
{
    return alsaexchange_exit();
}
