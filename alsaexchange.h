#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_init(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount), char *** AlsaNames, int * AlsaCount);

#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_show();

#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_alsa_run(int index, int Freq, int SamplesCount);

#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_alsa_stop(int index);


#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_exit();
