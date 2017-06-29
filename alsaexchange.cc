#include <gtk/gtk.h>    // GTK3
#include <asoundlib.h>  // ALSA
#include <string>       // std::string
#include <vector>       // std::vector

GtkWidget * alsaexchange_window;

struct SALSACardRecord
{
    std::string Name;
    std::string ID;
    GtkLabel * StatusLabel;
};
std::vector<struct SALSACardRecord> SoundCards;

// this lib(libalsaexchange.so) call this function for fill sound data from WINAPI part of this application
int (*FillOutputBuffers)(int * LeftOutput, int * RightOutput, int SamplesCount);

void ALSA_RUN (GtkButton *button, gpointer data)
{
    int i = (int)data;
    printf("RUN CALLED WITH %s\n", SoundCards[i].ID.c_str());
    gtk_label_set_text(SoundCards[i].StatusLabel, "try to run..");
    
    FillOutputBuffers(NULL,NULL,0);//DELETE THIS DEBUG CALL STRING
}

void ALSA_STOP (GtkButton *button, gpointer data)
{
    int i = (int)data;
    printf("STOP CALLED WITH %s\n", SoundCards[i].ID.c_str());
    gtk_label_set_text(SoundCards[i].StatusLabel, "try to stop..");
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
            }
            snd_ctl_close(handle);
            
            SoundCards.push_back({cardname,devname});
        }
        
        SoundCards.push_back({"AHUENNO NET ZVUKA","ID:40"}); //Add Card for debug TESTOWA9 RFKATTAa11
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
        GtkLabel * label = (GtkLabel *)gtk_label_new(SoundCards[i].Name.c_str());
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
