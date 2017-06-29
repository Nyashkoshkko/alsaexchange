#include <gtk/gtk.h>    // GTK3
#include <asoundlib.h>  // ALSA
#include <string>       // std::string
#include <vector>       // std::vector

GtkWidget * alsaexchange_window;

struct SALSACardRecord
{
    std::string Name;
    std::string ID;
    int Device;
    bool Online; // 0 - offline, 1 - online
    
    GtkLabel * StatusLabel;
    snd_pcm_t * handle;
};
std::vector<struct SALSACardRecord> SoundCards;

// this lib(libalsaexchange.so) call this function for fill sound data from WINAPI part of this application
int (*FillOutputBuffers)(int * LeftOutput, int * RightOutput, int SamplesCount);

void update_ui()
{
    for(SALSACardRecord Cur : SoundCards)
    {
        if(Cur.Online)
        {
            gtk_label_set_markup (Cur.StatusLabel, "<span foreground=\"green\" weight = \"bold\">  ONLINE</span>");
        }
        else 
        {
            gtk_label_set_text(Cur.StatusLabel, "  OFFLINE");
        }
    }
}

static void async_direct_callback(snd_async_handler_t *ahandler)
{
        snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
        struct async_private_data *data = snd_async_handler_get_callback_private(ahandler);
        const snd_pcm_channel_area_t *my_areas;
        snd_pcm_uframes_t offset, frames, size;
        snd_pcm_sframes_t avail, commitres;
        snd_pcm_state_t state;
        int first = 0, err;
 /*       
        while (1) {
                state = snd_pcm_state(handle);
                if (state == SND_PCM_STATE_XRUN) {
                        err = xrun_recovery(handle, -EPIPE);
                        if (err < 0) {
                                printf("XRUN recovery failed: %s\n", snd_strerror(err));
                                exit(EXIT_FAILURE);
                        }
                        first = 1;
                } else if (state == SND_PCM_STATE_SUSPENDED) {
                        err = xrun_recovery(handle, -ESTRPIPE);
                        if (err < 0) {
                                printf("SUSPEND recovery failed: %s\n", snd_strerror(err));
                                exit(EXIT_FAILURE);
                        }
                }
                avail = snd_pcm_avail_update(handle);
                if (avail < 0) {
                        err = xrun_recovery(handle, avail);
                        if (err < 0) {
                                printf("avail update failed: %s\n", snd_strerror(err));
                                exit(EXIT_FAILURE);
                        }
                        first = 1;
                        continue;
                }
                if (avail < period_size) {
                        if (first) {
                                first = 0;
                                err = snd_pcm_start(handle);
                                if (err < 0) {
                                        printf("Start error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                        } else {
                                break;
                        }
                        continue;
                }
                size = period_size;
                while (size > 0) {
                        frames = size;
                        err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
                        if (err < 0) {
                                if ((err = xrun_recovery(handle, err)) < 0) {
                                        printf("MMAP begin avail error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        generate_sine(my_areas, offset, frames, &data->phase);
                        commitres = snd_pcm_mmap_commit(handle, offset, frames);
                        if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                                if ((err = xrun_recovery(handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                        printf("MMAP commit error: %s\n", snd_strerror(err));
                                        exit(EXIT_FAILURE);
                                }
                                first = 1;
                        }
                        size -= frames;
                }
        }
*/
}

void ALSA_RUN (GtkButton *button, gpointer data)
{
    int i = (int)data;

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
    snd_pcm_uframes_t size;
    int err, dir;
    unsigned int buffer_time = 500000; // ring buffer length in us
    unsigned int period_time = 100000; // period time in us
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    
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
    err = snd_pcm_hw_params_set_access(SoundCards[i].handle, hwparams,  SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (err < 0) {
            printf("Access type not available for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // set the sample format
    err = snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, SND_PCM_FORMAT_S32_LE); // SND_PCM_FORMAT_S32_LE because motwt has ASIOSTInt32LSB as default
    if (err < 0) {                                                                             // but SND_PCM_FORMAT_FLOAT_LE is default VST format
            printf("Sample format not available for playback: %s\n", snd_strerror(err));
            
            printf("Supported formats are\n");
            
            #define CHECK_FORMAT(a)\
            {\
                if(snd_pcm_hw_params_set_format(SoundCards[i].handle, hwparams, a) >= 0)\
                {\
                    printf("%s\n",#a);\
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
    // set the count of channels
    err = snd_pcm_hw_params_set_channels(SoundCards[i].handle, hwparams, 2);
    if (err < 0) {
            printf("Channels count (%i) not available for playbacks: %s\n", 2, snd_strerror(err));
            CLOS();
    }
    // set the stream rate
    rrate = 44100;
    err = snd_pcm_hw_params_set_rate_near(SoundCards[i].handle, hwparams, &rrate, 0);
    if (err < 0) {
            printf("Rate %iHz not available for playback: %s\n", 44100, snd_strerror(err));
            CLOS();
    }
    if (rrate != 44100) {
            printf("Rate doesn't match (requested %iHz, get %iHz)\n", 44100, err);
            CLOS();
    }
    // set the buffer time
    err = snd_pcm_hw_params_set_buffer_time_near(SoundCards[i].handle, hwparams, &buffer_time, &dir);
    if (err < 0) {
            printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
            CLOS();
    }
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &size);
    if (err < 0) {
            printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    buffer_size = size;
    // set the period time
    err = snd_pcm_hw_params_set_period_time_near(SoundCards[i].handle, hwparams, &period_time, &dir);
    if (err < 0) {
            printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
            CLOS();
    }
    err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
    if (err < 0) {
            printf("Unable to get period size for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    period_size = size;
    // write the parameters to device
    err = snd_pcm_hw_params(SoundCards[i].handle, hwparams);
    if (err < 0) {
            printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
            CLOS();
    }

    //SWPARAMS
    
    int period_event = 0; // produce poll event after each period
    
    // get the current swparams
    err = snd_pcm_sw_params_current(SoundCards[i].handle, swparams);
    if (err < 0) {
            printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // start the transfer when the buffer is almost full:
    // (buffer_size / avail_min) * avail_min
    err = snd_pcm_sw_params_set_start_threshold(SoundCards[i].handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0) {
            printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // allow the transfer when at least period_size samples can be processed
    // or disable this mechanism when period event is enabled (aka interrupt like style processing)
    err = snd_pcm_sw_params_set_avail_min(SoundCards[i].handle, swparams, period_event ? buffer_size : period_size);
    if (err < 0) {
            printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    // enable period events when requested
    if (period_event) {
            err = snd_pcm_sw_params_set_period_event(SoundCards[i].handle, swparams, 1);
            if (err < 0) {
                    printf("Unable to set period event: %s\n", snd_strerror(err));
                    CLOS();
            }
    }
    // write the parameters to the playback device
    err = snd_pcm_sw_params(SoundCards[i].handle, swparams);
    if (err < 0) {
            printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
            CLOS();
    }
    
    
    //0
    
    //INIT ASYNC CALLBACKS
    struct async_private_data data;
    snd_async_handler_t *ahandler;
    const snd_pcm_channel_area_t *my_areas;
    snd_pcm_uframes_t offset, frames, size;
    snd_pcm_sframes_t commitres;
    int err, count;
    data.samples = NULL;    /* we do not require the global sample area for direct write */
    data.areas = NULL;      /* we do not require the global areas for direct write */
    data.phase = 0;
    err = snd_async_add_pcm_handler(&ahandler, SoundCards[i].handle, async_direct_callback, &data);
    if (err < 0) {
            printf("Unable to register async handler\n");
            exit(EXIT_FAILURE);
    }
    for (count = 0; count < 2; count++) {
            size = period_size;
            while (size > 0) {
                    frames = size;
                    err = snd_pcm_mmap_begin(SoundCards[i].handle, &my_areas, &offset, &frames);
                    if (err < 0) {
                            //if ((err = xrun_recovery(SoundCards[i].handle, err)) < 0) {
                                    printf("MMAP begin avail error: %s\n", snd_strerror(err));
                                    //exit(EXIT_FAILURE);
                                    CLOS();
                            //}
                    }
                    generate_sine(my_areas, offset, frames, &data.phase);
                    commitres = snd_pcm_mmap_commit(handle, offset, frames);
                    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                            //if ((err = xrun_recovery(SoundCards[i].handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
                                    printf("MMAP commit error: %s\n", snd_strerror(err));
                                    //exit(EXIT_FAILURE);
                                    CLOS();
                            //}
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
    update_ui();
    
    FillOutputBuffers(NULL,NULL,0);//DELETE THIS DEBUG CALL STRING
}

void ALSA_STOP (GtkButton *button, gpointer data)
{
    int i = (int)data;
    //printf("STOP CALLED WITH %s device %i\n", SoundCards[i].ID.c_str(), SoundCards[i].Device);

    if(SoundCards[i].Online)
    {
        snd_pcm_close(SoundCards[i].handle);
        SoundCards[i].Online = false;
        
        update_ui();
    }
}

int alsaexchange_main()
{
    static int fRun = 1;
    
    if(fRun == 1)
    {
        //GTK3 init
        int argc = 0; char **argv = NULL;
        gtk_init(&argc, &argv);
        
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
        
        //SoundCards.push_back({"AHUENNO NET ZVUKA", "ID:40", 40, true}); //Add Card for debug TESTOWA9 RFKATTAa11
        fRun = 0;
    }
    
    alsaexchange_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(alsaexchange_window), "ALSA Devices");
    gtk_container_set_border_width (GTK_CONTAINER(alsaexchange_window), 50);
    g_signal_connect(G_OBJECT(alsaexchange_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

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
    
    update_ui();
    
    gtk_widget_show_all(alsaexchange_window);
    gtk_main();

    return 0;
}
 
#include "alsaexchange.h"
extern "C" int alsaexchange_init(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount)) 
{
    FillOutputBuffers = FillOutputBuffers_ptr;
    alsaexchange_main();
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
