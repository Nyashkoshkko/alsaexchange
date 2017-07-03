#include "alsaexchange.h"
#include <windef.h> /* Part of the Wine header files */

int WINAPI alsaexchange_init_linuxcall(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount), char *** AlsaNames, int * AlsaCount, char *** MidiInNames, int * MidiInCount)
{
    return alsaexchange_init(FillOutputBuffers_ptr, AlsaNames, AlsaCount, MidiInNames, MidiInCount);
}

int WINAPI alsaexchange_show_linuxcall()
{
    return alsaexchange_show();
}

int WINAPI alsaexchange_alsa_run_linuxcall(int index, int Freq, int SamplesCount)
{
    return alsaexchange_alsa_run(index, Freq, SamplesCount);
}

int WINAPI alsaexchange_alsa_stop_linuxcall(int index)
{
    return alsaexchange_alsa_stop(index);
}

int WINAPI alsaexchange_exit_linuxcall()
{
    return alsaexchange_exit();
}

int WINAPI alsaexchange_midiin_run_linuxcall(int index)
{
    return alsaexchange_midiin_run(index);
}

int WINAPI alsaexchange_midiin_stop_linuxcall(int index)
{
    return alsaexchange_midiin_stop(index);
}

int WINAPI alsaexchange_midiin_getnewdata_linuxcall(char * mididata, int BytesAssigned)
{
    return alsaexchange_midiin_getnewdata(mididata, BytesAssigned);
}
