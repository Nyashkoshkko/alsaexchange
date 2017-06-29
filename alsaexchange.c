#include <gtk/gtk.h> /* подключаем GTK+ */

GtkWidget *alsaexchange_dummy_window;
GtkWidget * label;

/* выводит приветствие */
void alsaexchange_dummy_welcome (GtkButton *button, gpointer data)
{
    /* дать окну заголовок */
    gtk_window_set_title(GTK_WINDOW(alsaexchange_dummy_window), "посмотри ты нажал меня1 сюда сюда11 кочи кочи111");
}

int my_gtk_init(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
 
    return 0;
}


int alsa_state = -1;

void state_to_1 (GtkButton *button, gpointer data)
{
    alsa_state = 1;
}

void state_to_2 (GtkButton *button, gpointer data)
{
    alsa_state = 2;
}

/* эта функция будет выполнена первой */
int alsaexchange_dummy_main( int argc, char *argv[])
{
    static int fRun = 1;
    
    if(fRun == 1)
    {
        my_gtk_init(argc, argv);
     
        fRun = 0;
    }
    
    
    /* тут мы объявим переменные */
        
        /* это вставьте вначале, хотя на самом деле порядок не так важен */
        GtkWidget *alsaexchange_dummy_button;
        

        /* запускаем GTK+ */
        //gtk_init(&argc, &argv);

        /* тут будет код нашего приложения */
        
        /* создать новый виджет - окно */
        alsaexchange_dummy_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        /* дать окну заголовок */
        gtk_window_set_title(GTK_WINDOW(alsaexchange_dummy_window), "Введение в GTK");
        /* установить внутренние границы (читайте ниже) */
        gtk_container_set_border_width (GTK_CONTAINER(alsaexchange_dummy_window), 50);

        /* создать кнопку с меткой */   
        alsaexchange_dummy_button = gtk_button_new_with_label("Привет, ХабраХабр!");
        /* упаковать нашу кнопку в окно */
        gtk_container_add(GTK_CONTAINER(alsaexchange_dummy_window), alsaexchange_dummy_button);
        
        
        GtkWidget * alsaexchange_dummy_button1 = gtk_button_new_with_label("state_to_1");
        gtk_container_add(GTK_CONTAINER(alsaexchange_dummy_window), alsaexchange_dummy_button1);
        g_signal_connect(GTK_BUTTON(alsaexchange_dummy_button1), "clicked", G_CALLBACK(state_to_1), NULL);
        
        GtkWidget * alsaexchange_dummy_button2 = gtk_button_new_with_label("state_to_2");
        gtk_container_add(GTK_CONTAINER(alsaexchange_dummy_window), alsaexchange_dummy_button2);
        g_signal_connect(GTK_BUTTON(alsaexchange_dummy_button2), "clicked", G_CALLBACK(state_to_2), NULL);
        
        label = gtk_label_new ("STATE = ??");
        gtk_container_add(GTK_CONTAINER(alsaexchange_dummy_window), label);


        
        
        // v habre etoi shtuki ne bilo
        gtk_widget_show_all(alsaexchange_dummy_window);
        
        
        /* когда пользователь закроет окно, то выйти из приложения */
        g_signal_connect(G_OBJECT(alsaexchange_dummy_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
        
        /* когда пользователь кликнет по ней, то вызвать ф-цию welcome */
        g_signal_connect(GTK_BUTTON(alsaexchange_dummy_button), "clicked", G_CALLBACK(alsaexchange_dummy_welcome), NULL);
        
        /* передаём управление GTK+ */
        gtk_main();

        return 0;
}
 

#include "alsaexchange.h"
#include <stdio.h>

int alsaexchange_putsamples(void * buf, int cnt) 
{
    alsaexchange_dummy_main(0, NULL);
    printf("alsaexchange_putsamples called with: (%p, %i)\n", buf, cnt);
    return -40123456;
}
