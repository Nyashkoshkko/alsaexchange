#include <asoundlib.h>  // ALSA
#include <string>       // std::string
#include <vector>       // std::vector


//#define USING_GTK3
//#define DEBUG_LABEL


#ifdef USING_GTK3
#include <gtk/gtk.h>    // GTK3
#endif // USING_GTK3

struct SALSACardRecord
{
    std::string Name;
    std::string ID;
    int Device;
    bool Online; // 0 - offline, 1 - online
    snd_pcm_format_t SampleFormat;
    int SampleRate;
    int SampleCount;
    
    #ifdef USING_GTK3
    GtkLabel * StatusLabel;
    #endif // USING_GTK3
    
    snd_pcm_t * handle;
};
std::vector<struct SALSACardRecord> SoundCards;

// this lib(libalsaexchange.so) call this function for fill sound data from WINAPI part of this application
int (*FillOutputBuffers)(int * LeftOutput, int * RightOutput, int SamplesCount);
int LeftOutput[1024], RightOutput[1024]; // temp; then call filling 

#ifdef DEBUG_LABEL
int CallsCounter = 0;
#endif


int xrun_recovery(snd_pcm_t *handle, int err)
{
        //if (verbose)
                printf("stream recovery(err=%i)\n",err);
        if (err == -EPIPE) {    /* under-run */
                err = snd_pcm_prepare(handle);
                if (err < 0)
                        printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
                return 0;
        } else if (err == -ESTRPIPE) {
                while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                        sleep(1);       /* wait until the suspend flag is released */
                if (err < 0) {
                        err = snd_pcm_prepare(handle);
                        if (err < 0)
                                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
                }
                return 0;
        }
        return err;
}
void async_direct_callback(snd_async_handler_t *ahandler)
{
        snd_pcm_t *handle = (snd_pcm_t *)snd_async_handler_get_pcm(ahandler);
        const snd_pcm_channel_area_t *my_areas;
        snd_pcm_uframes_t offset, frames, size;
        snd_pcm_sframes_t avail, commitres;
        snd_pcm_state_t state;
        int first = 0, err;
        
        struct SALSACardRecord * Cur = (struct SALSACardRecord *)snd_async_handler_get_callback_private(ahandler);
        
        #ifdef DEBUG_LABEL
        CallsCounter++; // debug
        #endif

        while (1) {
                state = snd_pcm_state(handle);
                if (state == SND_PCM_STATE_XRUN) {
                        err = xrun_recovery(handle, -EPIPE);
                        if (err < 0) {
                                printf("XRUN recovery failed: %s\n", snd_strerror(err));
                                return;//exit(EXIT_FAILURE);
                        }
                        first = 1;
                } else if (state == SND_PCM_STATE_SUSPENDED) {
                        err = xrun_recovery(handle, -ESTRPIPE);
                        if (err < 0) {
                                printf("SUSPEND recovery failed: %s\n", snd_strerror(err));
                                return;//exit(EXIT_FAILURE);
                        }
                }
                avail = snd_pcm_avail_update(handle);
                if (avail < 0) {
                        err = xrun_recovery(handle, avail);
                        if (err < 0) {
                                printf("avail update failed: %s\n", snd_strerror(err));
                                return;//exit(EXIT_FAILURE);
                        }
                        first = 1;
                        continue;
                }
                if (avail < Cur->SampleCount) {
                        if (first) {
                                first = 0;
                                err = snd_pcm_start(handle);
                                if (err < 0) {
                                        printf("Start error: %s\n", snd_strerror(err));
                                        return;//exit(EXIT_FAILURE);
                                }
                        } else {
                                break;
                        }
                        continue;
                }
                size = Cur->SampleCount;
                
                FillOutputBuffers(LeftOutput, RightOutput, Cur->SampleCount);//потом добавить параметр - кол-во байт(4,3,2), чтобы winapi-часть приложения заполняла сразу буфер в драйвере (zero-copy)
                
                while (size > 0) {
                        frames = size;
                        err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
                        if (err < 0) {
                                if ((err = xrun_recovery(handle, err)) < 0) {
                                        printf("MMAP begin avail error: %s\n", snd_strerror(err));
                                        return;//exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        
                        
                        //generate_sine(my_areas, offset, frames, &data->phase);
                        const snd_pcm_channel_area_t *areas = my_areas; 
                        //snd_pcm_uframes_t offset,
                        int count = frames;
                        unsigned char *samples[2];
                        int steps[2];
                        
                        //!! todo доставить в этот коблюк индекс звуковой карты и исходя из нее выбрать кол-во байт(2,3,4)!!
                        int bps = 16 / 8;  // bytes per sample // default SND_PCM_FORMAT_S16
                        
                        switch(Cur->SampleFormat)
                        {//SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S16
                            case SND_PCM_FORMAT_S32_LE: bps = 4; break;
                            case SND_PCM_FORMAT_S24_3LE: bps = 3; break;
                        }
                        int delta = 4 - bps;
                        
                        //int res[2] = {0, 0};//current samples
                        
                        samples[0] = (((unsigned char *)areas[0].addr) + (areas[0].first / 8));
                        samples[1] = (((unsigned char *)areas[1].addr) + (areas[1].first / 8));
                        steps[0] = areas[0].step / 8;
                        samples[0] += offset * steps[0];
                        steps[1] = areas[1].step / 8;
                        samples[1] += offset * steps[1];
                        int CurSample = 0;
                        while (count-- > 0)
                        {
                            
                            //res[0] += 500000000; res[1] += 500000000;  // there's no sound if commented
                            //res[0] = ;
                            //res[1] = ;
                            
                            for (int i = 0; i < bps; i++)
                            {
                                *(samples[0] + i) = (LeftOutput[CurSample] >>  (i + delta) * 8) & 0xff;
                                *(samples[1] + i) = (RightOutput[CurSample] >>  (i + delta) * 8) & 0xff;
                            }
                            samples[0] += steps[0];
                            samples[1] += steps[1];
                            
                            CurSample++;
                        }
                        
                        commitres = snd_pcm_mmap_commit(handle, offset, frames);
                        if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                                if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                        printf("MMAP commit error: %s\n", snd_strerror(err));
                                        return;//exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        size -= frames;
                }
        }

}

void ALSA_RUN_real(int i, int Freq, int SamplesCount)
{
    if(SoundCards[i].Online) return; // Already online
    
    snd_pcm_hw_params_t * hwparams;
    snd_pcm_sw_params_t * swparams;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);
    
    if((snd_pcm_open(&SoundCards[i].handle, (std::string(SoundCards[i].ID + "," + std::to_string(SoundCards[i].Device))).c_str(), SND_PCM_STREAM_PLAYBACK, 0) < 0) || (SoundCards[i].handle == NULL)) return;
    
    #define CLOS() snd_pcm_close(SoundCards[i].handle);return
    
    //1
    
    //HWPARAMS
    
    unsigned int rrate;
    int err, dir;
    
    // choose all parameters 
    err = snd_pcm_hw_params_any(SoundCards[i].handle, hwparams);
    if (err < 0) {
            printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
            CLOS();
    }
    // set hardware resampling
    err = snd_pcm_hw_params_set_rate_resample(SoundCards[i].handle, hwparams, 0);
    if (err < 0) {
            printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // set the interleaved read/write format
    err = snd_pcm_hw_params_set_access(SoundCards[i].handle, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (err < 0) {
            printf("Access type not available for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // set the sample format : SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S16
    snd_pcm_format_t format;// = SND_PCM_FORMAT_S32_LE;
    format = SND_PCM_FORMAT_S32_LE; err = snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, format);
    if(err < 0)
    format = SND_PCM_FORMAT_S24_3LE; err = snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, format);
    if(err < 0)
    format = SND_PCM_FORMAT_S16; err = snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, format);
    
    
    //err = snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, SND_PCM_FORMAT_S32_LE); // SND_PCM_FORMAT_S32_LE because motwt has ASIOSTInt32LSB as default
    if (err < 0) {                                                                             // but SND_PCM_FORMAT_FLOAT_LE is default VST format
            printf("Sample format not available for playback: %s\n", snd_strerror(err));
            
            printf("Supported formats by hardware are:\n");
            
            #define CHECK_FORMAT(a)\
            {\
                if(snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, a) >= 0)\
                {\
                    printf("\t%s\n",#a);\
                    snd_pcm_close(SoundCards[i].handle);\
                    snd_pcm_open(&SoundCards[i].handle, (std::string(SoundCards[i].ID + "," + std::to_string(SoundCards[i].Device))).c_str(), SND_PCM_STREAM_PLAYBACK, 0);\
                    snd_pcm_hw_params_any(SoundCards[i].handle, hwparams);\
                    snd_pcm_hw_params_set_rate_resample(SoundCards[i].handle, hwparams, 0);\
                    snd_pcm_hw_params_set_access(SoundCards[i].handle, hwparams,  SND_PCM_ACCESS_MMAP_INTERLEAVED);\
                }\
            }
            
            CHECK_FORMAT(SND_PCM_FORMAT_S8);
            CHECK_FORMAT(SND_PCM_FORMAT_U8);
            CHECK_FORMAT(SND_PCM_FORMAT_S16_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_S16_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_U16_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_U16_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_S24_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_S24_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_U24_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_U24_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_S32_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_S32_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_U32_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_U32_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_FLOAT_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_FLOAT_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_FLOAT64_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_FLOAT64_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_IEC958_SUBFRAME_LE);
            CHECK_FORMAT(SND_PCM_FORMAT_IEC958_SUBFRAME_BE);
            CHECK_FORMAT(SND_PCM_FORMAT_MU_LAW);
            CHECK_FORMAT(SND_PCM_FORMAT_A_LAW);
            CHECK_FORMAT(SND_PCM_FORMAT_IMA_ADPCM);
            CHECK_FORMAT(SND_PCM_FORMAT_MPEG);
            CHECK_FORMAT(SND_PCM_FORMAT_GSM);
            CHECK_FORMAT(SND_PCM_FORMAT_SPECIAL);
            CHECK_FORMAT(SND_PCM_FORMAT_S24_3LE);
            CHECK_FORMAT(SND_PCM_FORMAT_S24_3BE);
            CHECK_FORMAT(SND_PCM_FORMAT_U24_3LE);
            CHECK_FORMAT(SND_PCM_FORMAT_U24_3BE);
            CHECK_FORMAT(SND_PCM_FORMAT_S20_3LE);
            CHECK_FORMAT(SND_PCM_FORMAT_S20_3BE);
            CHECK_FORMAT(SND_PCM_FORMAT_U20_3LE);
            CHECK_FORMAT(SND_PCM_FORMAT_U20_3BE);
            CHECK_FORMAT(SND_PCM_FORMAT_S18_3LE);
            CHECK_FORMAT(SND_PCM_FORMAT_S18_3BE);
            CHECK_FORMAT(SND_PCM_FORMAT_U18_3LE);
            CHECK_FORMAT(SND_PCM_FORMAT_U18_3BE);
            CHECK_FORMAT(SND_PCM_FORMAT_S16);
            CHECK_FORMAT(SND_PCM_FORMAT_U16);
            CHECK_FORMAT(SND_PCM_FORMAT_S24);
            CHECK_FORMAT(SND_PCM_FORMAT_U24);
            CHECK_FORMAT(SND_PCM_FORMAT_S32);
            CHECK_FORMAT(SND_PCM_FORMAT_U32);
            CHECK_FORMAT(SND_PCM_FORMAT_FLOAT);
            CHECK_FORMAT(SND_PCM_FORMAT_FLOAT64);
            CHECK_FORMAT(SND_PCM_FORMAT_IEC958_SUBFRAME);
            
            CLOS();
    }
    printf("Using supported sample format %s - OK\n", snd_pcm_format_name(format));
    SoundCards[i].SampleFormat = format;
    
    unsigned int chns; snd_pcm_hw_params_get_channels(hwparams, &chns); //printf("chns=%i\n",chns);
    
    // set the count of channels
    err = snd_pcm_hw_params_set_channels(SoundCards[i].handle, hwparams, 2);
    if ((err < 0) && (chns == 0)) {
        
        //unsigned int chns; snd_pcm_hw_params_get_channels(hwparams, &chns); printf("chns=%i\n",chns);
        
            printf("Channels count (%i) not available for playbacks: %s\n", 2, snd_strerror(err));
            CLOS();
    }
    snd_pcm_hw_params_get_channels(hwparams, &chns);
    printf("Using 2 from %i output channels - OK\n", chns);
    
    // set the stream rate
    rrate = 44100;
    err = snd_pcm_hw_params_set_rate_near(SoundCards[i].handle, hwparams, &rrate, 0);
    if (err < 0) {
            printf("Rate %iHz not available for playback: %s\n", 44100, snd_strerror(err));
            CLOS();
    }
    if ((rrate != 44100) && (rrate != 48000)) {
            printf("Rate doesn't match (requested %iHz, get %iHz)\n", 44100, rrate);
            CLOS();
    }
    printf("Using supported sample rate %i - OK\n", rrate);
    SoundCards[i].SampleRate = rrate;
    
    
    // set the buffer time
    snd_pcm_uframes_t buffer_size = SamplesCount * 2; // buffer_size must be period_size * 2
    err = snd_pcm_hw_params_set_buffer_size(SoundCards[i].handle, hwparams, buffer_size);
    if (err < 0) {
            printf("Unable to set buffer size %i for playback: %s\n", (int)buffer_size, snd_strerror(err));
            CLOS();
    }
    
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0) {
            printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    
    // set the period time
    snd_pcm_uframes_t period_size = SamplesCount; dir = 0;
    err = snd_pcm_hw_params_set_period_size(SoundCards[i].handle, hwparams, period_size, dir); // LAST PARAMETER IS dir: Wanted exact value is <,=,> val following dir (-1,0,1)
    if (err < 0) {
            printf("Unable to set period size %i for playback: %s\n", (int)period_size, snd_strerror(err));
            CLOS();
    }
    
    err = snd_pcm_hw_params_get_period_size(hwparams, &period_size, &dir);
    if (err < 0) {
            printf("Unable to get period size for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    SoundCards[i].SampleCount = period_size;
    
    printf("period_size = %i, buffer_size = %i\n", (int)period_size, (int)buffer_size);
    
    // write the parameters to device
    err = snd_pcm_hw_params(SoundCards[i].handle, hwparams);
    if (err < 0) {
            printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
            CLOS();
    }

    //SWPARAMS
    
    // get the current swparams
    err = snd_pcm_sw_params_current(SoundCards[i].handle, swparams);
    if (err < 0) {
            printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // start the transfer when the buffer is almost full:
    // (buffer_size / avail_min) * avail_min
    //err = snd_pcm_sw_params_set_start_threshold(SoundCards[i].handle, swparams, (buffer_size / period_size) * period_size); // сейчас эта хрень = 2 * сэмплес, может сделать 1.* сэмплес (PCM is automatically started when playback frames available to PCM are >= threshold or when requested capture frames are >= threshold)
    err = snd_pcm_sw_params_set_start_threshold(SoundCards[i].handle, swparams, period_size);
    if (err < 0) {
            printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // allow the transfer when at least period_size samples can be processed
    // or disable this mechanism when period event is enabled (aka interrupt like style processing)
    err = snd_pcm_sw_params_set_avail_min(SoundCards[i].handle, swparams, period_size);
    if (err < 0) {
            printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // write the parameters to the playback device
    err = snd_pcm_sw_params(SoundCards[i].handle, swparams);
    if (err < 0) {
            printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    
    
    //0
    
    //INIT ASYNC CALLBACKS
//    struct async_private_data data;
    snd_async_handler_t *ahandler;
    const snd_pcm_channel_area_t *my_areas;
    snd_pcm_uframes_t offset, frames, size;
    snd_pcm_sframes_t commitres;
    int count;
    //data.samples = NULL;    /* we do not require the global sample area for direct write */
    //data.areas = NULL;      /* we do not require the global areas for direct write */
    //data.phase = 0;
    //data.SoundCardIndex = i;
    err = snd_async_add_pcm_handler(&ahandler, SoundCards[i].handle, async_direct_callback, &SoundCards[i]);
    if (err < 0) {
            printf("Unable to register async handler\n");
            CLOS();//exit(EXIT_FAILURE);
    }
    for (count = 0; count < 2; count++) {
            size = period_size;
            while (size > 0) {
                    frames = size;
                    err = snd_pcm_mmap_begin(SoundCards[i].handle, &my_areas, &offset, &frames);
                    if (err < 0) {
                            if ((err = xrun_recovery(SoundCards[i].handle, err)) < 0) {
                                    printf("MMAP begin avail error: %s\n", snd_strerror(err));
                                    //exit(EXIT_FAILURE);
                                    CLOS();
                            }
                    }
                    //generate_sine(my_areas, offset, frames, &data.phase);
                    commitres = snd_pcm_mmap_commit(SoundCards[i].handle, offset, frames);
                    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                            if ((err = xrun_recovery(SoundCards[i].handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                    printf("MMAP commit error: %s\n", snd_strerror(err));
                                    //exit(EXIT_FAILURE);
                                    CLOS();
                            }
                    }
                    size -= frames;
            }
    }
    err = snd_pcm_start(SoundCards[i].handle);
    if (err < 0) {
            printf("Start error: %s\n", snd_strerror(err));
            //exit(EXIT_FAILURE);
            CLOS();
    }
    
        
    SoundCards[i].Online = true;
}


void ALSA_STOP_real(int i)
{
    if(SoundCards[i].Online)
    {
        snd_pcm_close(SoundCards[i].handle);
        SoundCards[i].Online = false;
    }
}

int alsaexchange_main_init()
{
    static int fRun = 1;
    
    if(fRun == 1)
    {
        //ALSA find sound cards and printf it
        int cardnum = -1;
        snd_ctl_t * handle;
        snd_ctl_card_info_t * info;
        snd_ctl_card_info_alloca(&info); // блядская архитектура альсы Оо
        int device_inside_card = -1;
        snd_pcm_info_t * pcminfo;
        snd_pcm_info_alloca(&pcminfo);//FUCKING ALSA PIDORI RAZRABI
        while(1)
        {
            snd_card_next(&cardnum); if(cardnum == -1) break;
            std::string devname = "hw:" + std::to_string(cardnum);
            if(snd_ctl_open(&handle, devname.c_str(), 0) < 0) continue; // третий параметр ЭТО РЕЖИМ, в т.ч. SND_CTL_ASYNC
            if(snd_ctl_card_info(handle,info) < 0) continue;
            std::string cardid = "hw:" + std::string(snd_ctl_card_info_get_id(info));
            std::string cardname = snd_ctl_card_info_get_name(info); device_inside_card = -1;
            while(1)
            {
                if((snd_ctl_pcm_next_device(handle, &device_inside_card) < 0) || (device_inside_card == -1)) break;
                snd_pcm_info_set_device(pcminfo, device_inside_card);
                snd_pcm_info_set_subdevice(pcminfo, 0); // RESSEARCH IF 1,2,3 WFT PARAMETER!!!!???
                snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
                if(snd_ctl_pcm_info(handle, pcminfo) < 0) continue;
                printf("device '%s' (name '%s', id '%s') -> device '%i' can do 'SND_PCM_STREAM_PLAYBACK'\n", devname.c_str(), cardname.c_str(), cardid.c_str(), device_inside_card);
                
                SoundCards.push_back({cardname, devname, device_inside_card, false});
            }
            snd_ctl_close(handle);
        }
    }

    return 0;
}

#ifdef USING_GTK3

GtkWidget * alsaexchange_window;

#ifdef DEBUG_LABEL
//Debug datas
GtkLabel * DebugLabel1;
int last_counter1 = 0;
gboolean Timer1Sec(gpointer data)
{
    char str[200];
    
    int rrate = 1;
    for(SALSACardRecord Cur : SoundCards)
    {
        if(Cur.Online)
        {
            rrate = Cur.SampleRate;
        }
    }
    int last_sec_calls = CallsCounter - last_counter1;
    
    snprintf(str, sizeof(str) - 1, "CALLS = %i, last sec calls = %i, samples = %i", CallsCounter, last_sec_calls, rrate/((last_sec_calls == 0) ? (rrate*10) : (last_sec_calls)));
    
    last_counter1 = CallsCounter;
    
    gtk_label_set_text(DebugLabel1, str);
    return true;
}
#endif

void update_ui()
{
    for(SALSACardRecord Cur : SoundCards)
    {
        if(Cur.Online)
        {
            gtk_label_set_markup (Cur.StatusLabel, std::string("<span foreground=\"green\" weight = \"bold\">  ONLINE</span>  (" + std::to_string(Cur.SampleRate) + ", " + std::string(snd_pcm_format_name(Cur.SampleFormat)) + ")").c_str());
        }
        else 
        {
            gtk_label_set_text(Cur.StatusLabel, "  OFFLINE");
        }
    }
}

void ALSA_RUN(GtkButton *button, gpointer data)
{
    ALSA_RUN_real((int)data, 44100, 256);
    update_ui();
}

void ALSA_STOP(GtkButton *button, gpointer data)
{
    ALSA_STOP_real((int)data);
    update_ui();
}

int alsaexchange_main_show()
{
    static int fRun = 1;
    
    if(fRun == 1)
    {
        //GTK3 init
        int argc = 0; char **argv = NULL;
        gtk_init(&argc, &argv);
        // Timer 1 sec init
        #ifdef DEBUG_LABEL
        g_timeout_add_seconds(1, Timer1Sec, NULL);
        #endif
        
        fRun = 0;
    }
    
    alsaexchange_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(alsaexchange_window), "ALSA Devices");
    gtk_container_set_border_width (GTK_CONTAINER(alsaexchange_window), 50);
    g_signal_connect(G_OBJECT(alsaexchange_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    //static void _quit_cb (GtkWidget *button, gpointer data) { (void)button; (void)data; gtk_main_quit(); return; }
    //g_signal_connect(G_OBJECT(alsaexchange_window), "destroy", G_CALLBACK(_quit_cb), NULL);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(alsaexchange_window), grid);

    for(int i = 0; i < SoundCards.size(); i++)
    {
        GtkLabel * label = (GtkLabel *)gtk_label_new(std::string(SoundCards[i].Name + " : " + std::to_string(SoundCards[i].Device) + "  ").c_str());
        gtk_grid_attach(GTK_GRID(grid), (GtkWidget *)label, 0, i, 1, 1);

        GtkWidget * button = gtk_button_new_with_label("RUN");
        gtk_grid_attach(GTK_GRID (grid), button, 1, i, 1, 1);
        g_signal_connect(GTK_BUTTON(button), "clicked", G_CALLBACK(ALSA_RUN), (gpointer)i);
        
        button = gtk_button_new_with_label("STOP");
        gtk_grid_attach(GTK_GRID (grid), button, 2, i, 1, 1);
        g_signal_connect(GTK_BUTTON(button), "clicked", G_CALLBACK(ALSA_STOP), (gpointer)i);
    
        SoundCards[i].StatusLabel = (GtkLabel *)gtk_label_new("status");
        gtk_grid_attach(GTK_GRID(grid), (GtkWidget *)SoundCards[i].StatusLabel, 3, i, 1, 1);
    }
    
    // Debug label that fill stom 1 sec timer
    #ifdef DEBUG_LABEL
    DebugLabel1 = (GtkLabel *)gtk_label_new("debug string");
    gtk_grid_attach(GTK_GRID(grid), (GtkWidget *)DebugLabel1, 0, SoundCards.size(), 4, 1);
    #endif
    
    update_ui();
    
    gtk_widget_show_all(alsaexchange_window);
    gtk_main();
    
    return 0;
}
#else // USING_GTK3
int alsaexchange_main_show(){ return 0; }
#endif // USING_GTK3

#include "alsaexchange.h"
extern "C" int alsaexchange_init(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount), char *** AlsaNames, int * AlsaCount) 
{
    FillOutputBuffers = FillOutputBuffers_ptr;
    alsaexchange_main_init();
    *AlsaNames = new char*[SoundCards.size()];
    for(int i = 0; i < SoundCards.size(); i++)
    {
        (*AlsaNames)[i] = (char*)SoundCards[i].Name.c_str();
    }
    *AlsaCount = SoundCards.size();
    return 0;
}

extern "C" int alsaexchange_show()
{
    alsaexchange_main_show();
    return 0;
}

extern "C" int alsaexchange_alsa_run(int index, int Freq, int SamplesCount)
{
    if(index < SoundCards.size())
    {
        ALSA_RUN_real(index, Freq, SamplesCount);
    }
    return 0;
}

extern "C" int alsaexchange_alsa_stop(int index)
{
    if(index < SoundCards.size())
    {
        ALSA_STOP_real(index);
    }
    return 0;
}

extern "C" int alsaexchange_exit()
{
    for(SALSACardRecord Cur : SoundCards)
    {
        if(Cur.Online)
        {
            snd_pcm_close(Cur.handle);
        }
    }
    
    return 0;
}
