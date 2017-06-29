#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_init(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount));

#ifdef __cplusplus
extern "C" 
#endif
int alsaexchange_exit();
