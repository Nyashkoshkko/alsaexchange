// - проект готов к компиляции
//

#include <windows.h>


//LINUX ST
#include <iostream> 
#include <stdio.h> 

using namespace std;


HINSTANCE hLib;

int (*alsaexchange_init)(int (*FillOutputBuffers_ptr)(int * LeftOutput, int * RightOutput, int SamplesCount), char *** AlsaNames, int * AlsaCount);
int (*alsaexchange_show)();
int (*alsaexchange_alsa_run)(int index, int Freq, int SamplesCount);
int (*alsaexchange_alsa_stop)(int index);
int (*alsaexchange_exit)();

int FillOutputBuffers(int * LeftOutput, int * RightOutput, int SamplesCount);// proto
char ** AlsaNames;
int AlsaCount = 0;

int run_func_drom_dll()
{
    static int fRun = 1;
    if(fRun == 1)
    {
        hLib=LoadLibrary(".\\alsaexchange.dll");
        if(hLib==NULL) 
        {
            cout << "Unable to load library!" << endl;
            return -1;
        }
      
        alsaexchange_init=( int (*) (int(*)(int*, int*, int), char***, int*))GetProcAddress((HMODULE)hLib, "alsaexchange_init");
        alsaexchange_show = ( int (*)() )GetProcAddress((HMODULE)hLib, "alsaexchange_show");
        alsaexchange_alsa_run = ( int (*)(int, int, int) )GetProcAddress((HMODULE)hLib, "alsaexchange_alsa_run");
        alsaexchange_alsa_stop = ( int (*)(int) )GetProcAddress((HMODULE)hLib, "alsaexchange_alsa_stop");
        alsaexchange_exit = ( int (*)() )GetProcAddress((HMODULE)hLib, "alsaexchange_exit");
    
        fRun = 0;
    }
    
    if(alsaexchange_show==NULL) {cout << "Unable to load function(s) alsaexchange_show" << endl;}
    if(alsaexchange_alsa_run==NULL) {cout << "Unable to load function(s) alsaexchange_alsa_run" << endl;}
    if(alsaexchange_alsa_stop==NULL) {cout << "Unable to load function(s) alsaexchange_alsa_stop" << endl;}
    if(alsaexchange_exit==NULL) {cout << "Unable to load function(s) alsaexchange_exit" << endl;}
      
    if(alsaexchange_init==NULL) 
    {
        cout << "Unable to load function(s) alsaexchange_init" << endl;
    }
    else
    {
        int res = alsaexchange_init((int (*)(int*, int*, int))&FillOutputBuffers, &AlsaNames, &AlsaCount);
    }

    return 0;
} 
int free_dll()
{
    if(hLib != NULL) 
    {
        alsaexchange_exit();
        FreeLibrary((HMODULE)hLib);
    }
    return 0;
}
//LINUX ED


#include <commctrl.h>
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//#define DEBUG_OUTPUT_WINDOW

#define INI_FILENAME "\\alsaexchange.ini" // changed 25.12.12 due to after loading FL Studio Plugin dll current path of all application stay FL folder path

#define MAX_NET_USERS 8 // максимальное число удаленных участников музыкальной онлайн-сессии; в первом релизе сделать 8

#define ID_BUTTON_ASIOCONFIG 1
#define ID_BUTTON_SAMPLES_64 200
#define ID_BUTTON_SAMPLES_128 201
#define ID_BUTTON_SAMPLES_256 202
#define ID_TIMER1 3
#define ID_ASIOLIST 4
#define ID_LABEL3 8 // alsa sample count


#define ID_LABEL8 24//"output volume"
#define ID_SLIDER_OUTPUT_VOLUME 20
#define ID_LABEL1 2 // 1..600(output volume value static text)
HWND H_SLIDER_OUTPUT_VOLUME; // слайдер регулировки уровня входной громкости(0.100..300)



//#define ID_CHECKBOX_WAVELOOP 32
#define ID_LABEL12 33//статик для имени сохраненного файла
#define ID_CHECKBOX_SAVEWAVE 34
//90,..



// прототип функции обработчика сообщений окна
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM,LPARAM);

HWND H_MAINWINDOW; // главное окно приложения
HWND H_ASIOLIST; // раскрывающийся список обнаруженных ASIO драйверов
HWND H_CB_SAVEWAVE; // чекбокс сохранения всего выходного вудиопотока в файл(запись(сами данные + обновить размер) раз в пять секунд + записать хвост в конце), по изменению галочки создать файл с заголовком

// add 17.12.2012
HWND H_TEXT_BUF_SIZE;
HWND H_BTN_ASIO_CFG;
HWND H_BTN_SAMPLES_64;
HWND H_BTN_SAMPLES_128;
HWND H_BTN_SAMPLES_256;
HWND H_TEXT_OUTPUT_VOLUME;
HWND H_TEXT_OUTPUT_VOL_CNT;
HWND H_TEXT_ASIOLIST;
#define ID_TEXT_ASIOLIST 92

HWND H_TEXT_VSTPLUGIN;
#define ID_TEXT_VSTPLUGIN 96
HWND H_TEXT_ASIO_BUFSIZE;
#define ID_TEXT_ASIO_BUFSIZE 97

HWND H_BTN_SHOWVST;
#define ID_BTN_SHOWVST 98

HWND H_VSTLIST;
#define ID_VSTLIST 99

#define ID_TIMER2 100







int VolumeOutput = 100; // громкость выходного канала; 100 - базовая громкость
#define WAVEDATAOUTSIZE 4410*15
struct SWaveDataOut// выходной буфер записи в .wav-файл расчитан на 1.5 секунды 32-хбитных стерео сэмплов
{
	short Left[WAVEDATAOUTSIZE];
	short Right[WAVEDATAOUTSIZE];
	int PosRead;
	int PosWrite;
	int IsSaveWave; // признак записи всего выходного аудиопотока в WAVE файл
	int CanRealWriteBuf;
};
struct SWaveDataOut WaveDataOut;

void CloseWAVEFile(); // прототип функции закрытия выходного .wav-файла
int StopWaveFile(); // прототип функции остановки входного .wav-файла

struct SVolLabel
{
	int x;
	int y;
	int lenX;
	int lenY;
} VolLabel[MAX_NET_USERS + 3];
#define VOL_LBL_OUT	MAX_NET_USERS
#define VOL_LBL_IN	(MAX_NET_USERS + 1)
#define VOL_LBL_WAV	(MAX_NET_USERS + 2)


unsigned char VSTPath[1024];

unsigned char AppRootPath[1024] = ".";
unsigned char AppIniPath[1024 + 266];
unsigned char AppPathTemp[1024 + 266];

unsigned long MySystemTime = 0;


///////////////////////////////////////////////////////////////////////////
////////// ~~~ функциональность VST Plugin SDK 2.4 начало ~~~ /////////////
#define _CRT_SECURE_NO_DEPRECATE
#include "aeffectx.h"

struct MyDLGTEMPLATE: DLGTEMPLATE
{
	WORD ext[3];
	MyDLGTEMPLATE ()
	{
		memset (this, 0, sizeof (*this));
	};
};

static INT_PTR CALLBACK EditorProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static AEffect* theEffect = 0;

//HINSTANCE My_hInstance;
HANDLE MyVST_WindowThread = NULL;

bool checkEffectEditor (AEffect* effect)
{
	if ((effect->flags & effFlagsHasEditor) == 0)
	{
		////printf ("This plug does not have an editor!\n");
		return false;
	}

	theEffect = effect;

	MyDLGTEMPLATE t;	
	t.style = WS_POPUPWINDOW | WS_DLGFRAME | DS_MODALFRAME | DS_CENTER;
	t.cx = 100;
	t.cy = 100;
	DialogBoxIndirectParam (GetModuleHandle (0), &t, H_MAINWINDOW, (DLGPROC)EditorProc, (LPARAM)effect); // вот здесь прога зависает когда вызвалось окошко редактора

	theEffect = 0;
	MyVST_WindowThread = NULL;
	return true;
}

INT_PTR CALLBACK EditorProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	AEffect* effect = theEffect;

	switch(msg)
	{
		case WM_INITDIALOG :
		{
			SetWindowText (hwnd, "VST Editor");
			SetTimer (hwnd, 1, 20, 0);

			if (effect)
			{
				////printf ("HOST> Open editor...\n");
				effect->dispatcher (effect, effEditOpen, 0, 0, hwnd, 0);

				////printf ("HOST> Get editor rect..\n");
				ERect* eRect = 0;
				effect->dispatcher (effect, effEditGetRect, 0, 0, &eRect, 0);
				if (eRect)
				{
					int width = eRect->right - eRect->left;
					int height = eRect->bottom - eRect->top;
					if (width < 100)
						width = 100;
					if (height < 100)
						height = 100;

					RECT wRect;
					SetRect (&wRect, 0, 0, width, height);
					AdjustWindowRectEx (&wRect, GetWindowLong (hwnd, GWL_STYLE), FALSE, GetWindowLong (hwnd, GWL_EXSTYLE));
					width = wRect.right - wRect.left;
					height = wRect.bottom - wRect.top;

					SetWindowPos (hwnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
				}
			}
		}	break;

		case WM_TIMER :
			if (effect)
				effect->dispatcher (effect, effEditIdle, 0, 0, 0, 0);
			break;

		case WM_CLOSE :
		{
			KillTimer (hwnd, 1);

			//printf ("HOST> Close editor..\n");
			if (effect)
				effect->dispatcher (effect, effEditClose, 0, 0, 0, 0);

			EndDialog (hwnd, IDOK);
		}	break;
	}

	return 0;
}

//-------------------------------------------------------------------------------------------------------
static const float kSampleRate = 44100.f; // LINUX TODO: здесь залаётся частота для всех VST-инструментов!!!!
int ALSADriverSampleCount = 256;

//-------------------------------------------------------------------------------------------------------
typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);
VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	VstIntPtr result = 0;

	switch (opcode)
	{
		case audioMasterVersion :
			result = kVstVersion;
			break;
	}

	return result;
}

struct PluginLoader
{
	void* module;

	PluginLoader ()
	: module (0)
	{}

	~PluginLoader ()
	{
		if (module)
		{
			FreeLibrary ((HMODULE)module);
		}
	}

	bool loadLibrary (const char* fileName)
	{
		module = LoadLibrary (fileName);
		return module != 0;
	}

	PluginEntryProc getMainEntry ()
	{
		PluginEntryProc mainProc = 0;
		mainProc = (PluginEntryProc)GetProcAddress ((HMODULE)module, "VSTPluginMain");
		if (!mainProc)
			mainProc = (PluginEntryProc)GetProcAddress ((HMODULE)module, "main");
		return mainProc;
	}
//-------------------------------------------------------------------------------------------------------
};

AEffect* MyVSTEffect;
PluginLoader MyVSTPluginLoader;
float** MyVST_Inputs = 0;
float** MyVST_Outputs = 0;
VstInt32 MyVST_NumInputs;
VstInt32 MyVST_NumOutputs;
int MyVST_active = 0;
char MyVST_DllFilename[1024] = "NONE";

//VST
DWORD WINAPI MyVST_StartWindow( LPVOID lpParam );
int MyVST_Init()
{
	//const char* fileName = "Guitar Rig 4.dll";

	//printf ("HOST> Load library...\n");
	//if (!MyVSTPluginLoader.loadLibrary (fileName))
	if (!MyVSTPluginLoader.loadLibrary (MyVST_DllFilename))
	{
		//printf ("Failed to load VST Plugin library!\n");
		return -1;
	}

	PluginEntryProc mainEntry = MyVSTPluginLoader.getMainEntry ();
	if (!mainEntry)
	{
		//printf ("VST Plugin main entry not found!\n");
		return -1;
	}

	//printf ("HOST> Create effect...\n");
	MyVSTEffect = mainEntry (HostCallback);
	if (!MyVSTEffect)
	{
		//printf ("Failed to create effect instance!\n");
		return -1;
	}

	//printf ("HOST> Init sequence...\n");
	MyVSTEffect->dispatcher (MyVSTEffect, effOpen, 0, 0, 0, 0);
	MyVSTEffect->dispatcher (MyVSTEffect, effSetSampleRate, 0, 0, 0, kSampleRate);
	MyVSTEffect->dispatcher (MyVSTEffect, effSetBlockSize, 0, ALSADriverSampleCount, 0, 0);

	MyVST_NumInputs = MyVSTEffect->numInputs;
	MyVST_NumOutputs = MyVSTEffect->numOutputs;
	
	if (MyVST_NumInputs > 0)
	{
		MyVST_Inputs = new float*[MyVST_NumInputs];
		for (VstInt32 i = 0; i < MyVST_NumInputs; i++)
		{
			MyVST_Inputs[i] = new float[ALSADriverSampleCount];
			memset (MyVST_Inputs[i], 0, ALSADriverSampleCount * sizeof (float));
		}
	}

	if (MyVST_NumOutputs > 0)
	{
		MyVST_Outputs = new float*[MyVST_NumOutputs];
		for (VstInt32 i = 0; i < MyVST_NumOutputs; i++)
		{
			MyVST_Outputs[i] = new float[ALSADriverSampleCount];
			memset (MyVST_Outputs[i], 0, ALSADriverSampleCount * sizeof (float));
		}
	}

	//printf ("HOST> Resume effect...\n");
	MyVSTEffect->dispatcher (MyVSTEffect, effMainsChanged, 0, 1, 0, 0);

	MyVST_WindowThread = CreateThread(NULL, 0, MyVST_StartWindow, NULL, 0, NULL);

	return 0;
}

//void MyVST_StartWindow() // функция, которая вернет управление по закрытию окна
DWORD WINAPI MyVST_StartWindow( LPVOID lpParam )
{
	checkEffectEditor (MyVSTEffect);
	return 0;
}

void MyVST_Close()
{
	MyVST_active = 0;

	TerminateThread(MyVST_WindowThread, 0);

	//printf ("HOST> Suspend effect...\n");
	MyVSTEffect->dispatcher (MyVSTEffect, effMainsChanged, 0, 0, 0, 0);

	if (MyVST_NumInputs > 0)
	{
		for (VstInt32 i = 0; i < MyVST_NumInputs; i++)
			delete [] MyVST_Inputs[i];
		delete [] MyVST_Inputs;
	}

	if (MyVST_NumOutputs > 0)
	{
		for (VstInt32 i = 0; i < MyVST_NumOutputs; i++)
			delete [] MyVST_Outputs[i];
		delete [] MyVST_Outputs;
	}

	//printf ("HOST> Close effect...\n");
	MyVSTEffect->dispatcher (MyVSTEffect, effClose, 0, 0, 0, 0);
}

void MyVST_BufferUpdate()
{
	//printf ("HOST> Process Replacing...\n");
	MyVSTEffect->processReplacing (MyVSTEffect, MyVST_Inputs, MyVST_Outputs, ALSADriverSampleCount);
}


////////// ~~~ функциональность VST Plugin SDK 2.4 конец ~~~ /////////////
//////////////////////////////////////////////////////////////////////////

int ALSADriverCurrentIndex = 0;

const int VstM_Pix = 45;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR IpCmdLine, int nCmdShow)
{
    run_func_drom_dll(); // linux ALSA window settings from .so
    
	int i;
	char sss[256];

	// регистрируем класс окна
	WNDCLASS w;
	memset(&w,0,sizeof(WNDCLASS));
	w.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // применяем стиль CS_DBLCLKS чтобы окно могло получать сообщения о двойных щелчках мыши
	w.lpfnWndProc = WndProc;
	w.hInstance = hInstance; //My_hInstance = hInstance;
	w.hbrBackground = HBRUSH(COLOR_BTNFACE + 1);
	w.lpszClassName = "Window_nya";
	RegisterClass(&w);

// 	// создаем экземпляр класса окна // linux: 260 is settings width
	H_MAINWINDOW = CreateWindow("Window_nya", "Music Over The World Tool RC4", WS_OVERLAPPEDWINDOW, 300, 250, 575 + 260, 580 + VstM_Pix, NULL, NULL, hInstance, NULL);

	// создаем раскрывающийся список обнаруженных ASIO драйверов
	H_ASIOLIST = CreateWindow("combobox", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 658, 10, 147, 150, H_MAINWINDOW, (HMENU)ID_ASIOLIST, hInstance, NULL);

	H_TEXT_ASIOLIST = CreateWindow("static", "ALSA driver:", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 570, 13, 80, 20, H_MAINWINDOW, (HMENU)ID_TEXT_ASIOLIST, hInstance, NULL);

	//создаем экземпляр класса кнопка открытия окна конфигурирования выбранного ASIO драйвера
	H_BTN_ASIO_CFG = CreateWindow("static", "ALSA Config", WS_VISIBLE | WS_CHILD | SS_NOTIFY | WS_TABSTOP | SS_CENTER | WS_BORDER, 570, 50, 130, 25, H_MAINWINDOW, (HMENU)ID_BUTTON_ASIOCONFIG, hInstance, NULL);
    
    H_BTN_SAMPLES_64 = CreateWindow("static", "64", WS_VISIBLE | WS_CHILD | SS_NOTIFY | WS_TABSTOP | SS_CENTER | WS_BORDER, 130, 50, 40, 20, H_MAINWINDOW, (HMENU)ID_BUTTON_SAMPLES_64, hInstance, NULL);
    H_BTN_SAMPLES_128 = CreateWindow("static", "128", WS_VISIBLE | WS_CHILD | SS_NOTIFY | WS_TABSTOP | SS_CENTER | WS_BORDER, 130 + ((50 + 5) * 1), 50, 40, 20, H_MAINWINDOW, (HMENU)ID_BUTTON_SAMPLES_128, hInstance, NULL);
    H_BTN_SAMPLES_256 = CreateWindow("static", "256", WS_VISIBLE | WS_CHILD | SS_NOTIFY | WS_TABSTOP | SS_CENTER | WS_BORDER, 130 + ((50 + 5) * 2), 50, 40, 20, H_MAINWINDOW, (HMENU)ID_BUTTON_SAMPLES_256, hInstance, NULL);



	// создаем экземпляр класса метка, показывающий заполненность буфера полученных по сети данных(из структуры хост сэмпла)
	H_TEXT_BUF_SIZE = CreateWindow("static", "0", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 250, 25, 40, 20, H_MAINWINDOW, (HMENU)ID_LABEL3, hInstance, NULL);
	H_TEXT_ASIO_BUFSIZE = CreateWindow("static", "ALSA buffer size:", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 130, 25, 170, 20, H_MAINWINDOW, (HMENU)ID_TEXT_ASIO_BUFSIZE, hInstance, NULL);

	int POff_ip1 = 9;
	int POff_ip2 = -17 + 1;
	int POff_ip3 = POff_ip2 + 3 + 1;
	

	int PixelOffset = 40;


    PixelOffset += 15;


	int PixelOffset2 = 66;
	// создаем экземпляр класса метка, показывающая "output volume:"
	H_TEXT_OUTPUT_VOLUME = CreateWindow("static", "Output volume", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 32, 35 + PixelOffset2 - 10, 105, 20, H_MAINWINDOW, (HMENU)ID_LABEL8, hInstance, NULL);
	// слайдер регулятора уровня громкости выходного аудио-потока
	H_SLIDER_OUTPUT_VOLUME=CreateWindow("msctls_trackbar32", "output volume", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS /*| TBS_AUTOTICKS*/, 135, 35 + PixelOffset2, 140, 30, H_MAINWINDOW, (HMENU)ID_SLIDER_OUTPUT_VOLUME, hInstance, NULL);
	// создаем экземпляр класса метка, показывающая выходную громкость * 100
	H_TEXT_OUTPUT_VOL_CNT = CreateWindow("static", "0", WS_VISIBLE | WS_CHILD | WS_TABSTOP, VolLabel[VOL_LBL_OUT].x = 275, VolLabel[VOL_LBL_OUT].y = 35 + PixelOffset2, VolLabel[VOL_LBL_OUT].lenX = 40, VolLabel[VOL_LBL_OUT].lenY = 20, H_MAINWINDOW, (HMENU)ID_LABEL1, hInstance, NULL);

    
	H_TEXT_VSTPLUGIN = CreateWindow("static", "Input VST:", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 32 + 9, 165, 76, 25, H_MAINWINDOW, (HMENU)ID_TEXT_VSTPLUGIN, hInstance, NULL);
	H_VSTLIST = CreateWindow("combobox", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 114 + 3, 162, 204 - 3, 150, H_MAINWINDOW, (HMENU)ID_VSTLIST, hInstance, NULL);
	H_BTN_SHOWVST = CreateWindow("static", "e", WS_VISIBLE | WS_CHILD | SS_NOTIFY | WS_TABSTOP | SS_CENTER | WS_BORDER, 330, 165, 25, 25, H_MAINWINDOW, (HMENU)ID_BTN_SHOWVST, hInstance, NULL);

	int PixelOffsetVSTi = 43;

	int PixelOffsetVST_m = 43 + 43;

	// чекбокс записи всего выходного аудиопотока в файл
	H_CB_SAVEWAVE=CreateWindow("button", "Save master stream", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_AUTOCHECKBOX, 380, 163/* + PixelOffsetVSTi/2*/, 150, 30, H_MAINWINDOW, (HMENU)ID_CHECKBOX_SAVEWAVE, hInstance, NULL);
	
	PixelOffset -= -35;

	#define CLIENT_PIX_OFFSET 33

	int XOffset = 7;

	PixelOffsetVSTi = 50 + VstM_Pix;

	
	PixelOffset += 123 + (i * CLIENT_PIX_OFFSET);

	// создаем экземпляр класса метка, показывающая "filename.wav saved"
	//"C:\\Nyashkoshkkko\'s Music Over The World\\Nyashkoshkkko\'s Music Over The World\\smallwin32\\2012_12_25 17_51_10.wav saved"
	//"\\2012_12_25 17_51_10.wav saved"
	CreateWindow("static", "", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 382, 195, 140, 80, H_MAINWINDOW, (HMENU)ID_LABEL12, hInstance, NULL);

	

	



	//add 17.12.2012 - new user interface
	// создаем массив хэндлов для пробега интерфейса через один цикл
	#define HWND_INTERFACE_CNT (16)
	HWND H_ALL[HWND_INTERFACE_CNT];
	i = 0;
	H_ALL[i++] = H_MAINWINDOW;
	H_ALL[i++] = H_ASIOLIST;
	H_ALL[i++] = H_CB_SAVEWAVE;
	H_ALL[i++] = H_TEXT_BUF_SIZE;
	H_ALL[i++] = H_BTN_ASIO_CFG;
	H_ALL[i++] = H_TEXT_OUTPUT_VOLUME;
	H_ALL[i++] = H_TEXT_OUTPUT_VOL_CNT;
	H_ALL[i++] = H_SLIDER_OUTPUT_VOLUME;
	H_ALL[i++] = H_TEXT_ASIOLIST;
	H_ALL[i++] = H_TEXT_VSTPLUGIN;
	H_ALL[i++] = H_TEXT_ASIO_BUFSIZE;
	H_ALL[i++] = H_BTN_SHOWVST;
	H_ALL[i++] = H_VSTLIST;
    H_ALL[i++] = H_BTN_SAMPLES_64;
    H_ALL[i++] = H_BTN_SAMPLES_128;
    H_ALL[i++] = H_BTN_SAMPLES_256;
	// создаем цвет окна - белый
	HFONT fnt;

	int MyFontSize = -16;
	if((DWORD)(LOBYTE(LOWORD(GetVersion()))) < 6) MyFontSize = -14; // если Windows не виста и не 7, а ниже, то шрифт меньше, иначе расползается
	fnt = CreateFont(MyFontSize,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,

					OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
					DEFAULT_PITCH|FF_DONTCARE, "Segoe UI Light");
	for(i = 0; i < HWND_INTERFACE_CNT; i++)
	{
		SetClassLong(H_ALL[i], GCL_HBRBACKGROUND, (LONG)CreateSolidBrush(RGB(255,255,255))) ;
		SendMessage(H_ALL[i], WM_SETFONT, (WPARAM)fnt, 0);
	}

	
	#define VOLIME_SLIDER_MAX 600

	// установить диапазон значений слайдера регулировки громкости выходного аудио потока
	SendDlgItemMessage(H_MAINWINDOW, ID_SLIDER_OUTPUT_VOLUME, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(1,VOLIME_SLIDER_MAX));//Установка диапазона значений регулятора. Сообщение TBM_SETRANGE задаёт диапазон, указанный в LPARAM
	SendDlgItemMessage(H_MAINWINDOW, ID_SLIDER_OUTPUT_VOLUME, TBM_SETPOS, (WPARAM)1, (LPARAM)100);//Установка позиции движка сообщением TBM_SETPOS.
	for (i=0; i<=VOLIME_SLIDER_MAX; i+=100) SendMessage(H_SLIDER_OUTPUT_VOLUME, TBM_SETTIC, 0 , i); // зададим промежуточные метки

	// прочитать текущий путь в переменную
	GetCurrentDirectory(1024, (LPSTR)AppRootPath);

	// установить путь для ини-файла
	wsprintf((LPSTR)AppIniPath,"%s%s", AppRootPath, INI_FILENAME);

	// считать из ини-файла output volume
	VolumeOutput = GetPrivateProfileInt("MAIN","VolumeOutput", 100, (LPSTR)AppIniPath);
	SendDlgItemMessage(H_MAINWINDOW, ID_SLIDER_OUTPUT_VOLUME, TBM_SETPOS, (WPARAM)1, (LPARAM)VolumeOutput);
    
    // read last active alsa index
    ALSADriverCurrentIndex = GetPrivateProfileInt("MAIN","ALSADriverCurrentIndex", 0, (LPSTR)AppIniPath);
    
    // alsa samples
    ALSADriverSampleCount = GetPrivateProfileInt("MAIN","ALSADriverSampleCount", 256, (LPSTR)AppIniPath);

	// инициализация начальных значений переменных
	WaveDataOut.IsSaveWave = 0;
	WaveDataOut.PosRead = 0;
	WaveDataOut.PosWrite = 0;
	WaveDataOut.CanRealWriteBuf = 0;

    //
    for(i = 0; i < AlsaCount; i++)
    {
        SendMessage(H_ASIOLIST, CB_ADDSTRING, 0, (LPARAM)(AlsaNames[i]) ); // Добавление пункта
    }
    SendMessage(H_ASIOLIST, CB_SETCURSEL, ALSADriverCurrentIndex, 0); // Выбор текущего элемента списка

	// запустить поток, если в системе найден хоть один ASIO драйвер
	if(AlsaCount > 0)
    {
        alsaexchange_alsa_run(ALSADriverCurrentIndex, 44100, ALSADriverSampleCount);
    }
	
	// add 18.12.2012 чтение списка установленных вст-плагинов
	//i = 1;
	HKEY hVSTKey;
	DWORD RegData_VSTKey = 1024;
	CHAR CheckBoxText[1024];
	SendMessage(H_VSTLIST, CB_ADDSTRING, 0, (LPARAM)((LPCWSTR)"Not loaded")); // Добавление пункта
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\VST", 0, KEY_READ, &hVSTKey) == 0)
	{
		if(RegQueryValueEx(hVSTKey, "VSTPluginsPath", NULL, NULL, VSTPath, &RegData_VSTKey) == ERROR_SUCCESS)
		{
			// в реестре найден путь для вст-плагинов - добавить их список в список
			WIN32_FIND_DATA ffd;
			wsprintf(CheckBoxText, "%s\\*", VSTPath);
			HANDLE hFind = FindFirstFile((LPCSTR)CheckBoxText, &ffd);
			if(hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
/*					if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // если нужен поиск по вложенным директориям, то как-то так
					{
						wsprintf(CheckBoxText, "%s   <DIR>", ffd.cFileName);
					}
*/					if(strstr(ffd.cFileName, ".dll") != NULL)
					{
						SendMessage(H_VSTLIST, CB_ADDSTRING, 0, (LPARAM)((LPCWSTR)ffd.cFileName)); // Добавление пункта

						//if(i != -1) i++;
					}
				}
				while (FindNextFile(hFind, &ffd) != 0);
			}
		}
		else // в реестре не найдена информация о пути для вст-плагинов
		{
			SendMessage(H_VSTLIST, CB_ADDSTRING, 0, (LPARAM)((LPCWSTR)"NO VST FOLDER")); // Добавление пункта
		}
		RegCloseKey(hVSTKey);
	}
	//if(i != -1) SendMessage(H_VSTLIST, CB_SETCURSEL, 0, 0); // Выбор текущего элемента списка
	
	// создаем таймеры
	SetTimer(H_MAINWINDOW, ID_TIMER1, 1000, NULL);
	SetTimer(H_MAINWINDOW, ID_TIMER2, 66, NULL);

	// показываем основное окно
	ShowWindow(H_MAINWINDOW, nCmdShow);

	//входим в цикл ожидания сообщения
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// закрываем таймеры
	KillTimer(H_MAINWINDOW, ID_TIMER1);
	KillTimer(H_MAINWINDOW, ID_TIMER2);

	// закрываем VST-dll, если была открыта
	if(MyVST_active == 1)
	{
		MyVST_Close();
	}

	// записываем выходной .wav-файл если отмечена галочка
	if(WaveDataOut.IsSaveWave == 1)
	{
		CloseWAVEFile();
	}

	// сохранение данных в ини-файл
	char IniFileParam[256];
    // сохранить в ини-файл output volume
	wsprintf(IniFileParam, "%i", VolumeOutput);
	WritePrivateProfileString("MAIN","VolumeOutput", IniFileParam, (LPSTR)AppIniPath);
    
    // write active alsa driver index to .ini file
    wsprintf(IniFileParam, "%i", ALSADriverCurrentIndex);
	WritePrivateProfileString("MAIN","ALSADriverCurrentIndex", IniFileParam, (LPSTR)AppIniPath);
    
    // writa alsa samples to .ini file
    wsprintf(IniFileParam, "%i", ALSADriverSampleCount);
	WritePrivateProfileString("MAIN","ALSADriverSampleCount", IniFileParam, (LPSTR)AppIniPath);

	return msg.wParam;
}



// макросы и переменные, относящиеся к сохранению выходного аудио-потока в .wav-файл
#define OFFSET_SAMPLES_COUNT	0x28	// [0x28..0x2b] количество байт данных сэмплов
#define OFFSET_FILESIZE_MINUS_8	4		// [4..7] размер файла - 8 байт
static char Filename[256];
static HANDLE FileHandle;
static int TotalSamplesWrittenInBytes = 0; // количество записанных сэмплов в .wav-файл
static int CurrentWaveDataOffsetInFile = 0; // переменная для установки указателя на индекс байта файла куда писать очередные данные
int SaveFileLabelSeconds = 0;

void CreateWAVEFile()
{
	unsigned char FileHeader[0x2c];

	DWORD BytesWritten;

	// создать файл с именем дата-время.wav, объявить глобальный дескриптор, записать заголовок в .wav-файл
	SYSTEMTIME sm;
	GetLocalTime(&sm); // получаем  системные дату и время
	wsprintf(Filename, "\\%04d_%02d_%02d %02d_%02d_%02d.wav", sm.wYear, sm.wMonth, sm.wDay, sm.wHour, sm.wMinute, sm.wSecond);
	wsprintf((LPSTR)AppPathTemp, "%s%s", AppRootPath, Filename);
	FileHandle=CreateFile((LPCSTR)AppPathTemp, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// создать массив заголовка, определить указатели на индекс кол-ва сэмплов и размер-8, а также индекс записи данных
	FileHeader[0] = 'R';
	FileHeader[1] = 'I';
	FileHeader[2] = 'F';
	FileHeader[3] = 'F';
	FileHeader[8] = 'W';
	FileHeader[9] = 'A';
	FileHeader[10] = 'V';
	FileHeader[11] = 'E';
	FileHeader[12] = 'f';
	FileHeader[13] = 'm';
	FileHeader[14] = 't';
	FileHeader[15] = ' ';
	*((UINT  *)&FileHeader[12+0x04]) = 16;				//0x04	4	Chunk Data Size	16 + extra format bytes
	*((USHORT*)&FileHeader[12+0x08]) = 1;				//0x08	2	Compression code	1 - 65,535
	*((USHORT*)&FileHeader[12+0x0a]) = 2;				//0x0a	2	Number of channels	1 - 65,535
	*((UINT  *)&FileHeader[12+0x0c]) = 44100;			//0x0c	4	Sample rate	1 - 0xFFFFFFFF
	*((UINT  *)&FileHeader[12+0x10]) = 2 * 2 * 44100;	//0x10	4	Average bytes per second	1 - 0xFFFFFFFF
	*((USHORT*)&FileHeader[12+0x14]) = 4;				//0x14	2	Block align	1 - 65,535
	*((USHORT*)&FileHeader[12+0x16]) = 16;				//0x16	2	Significant bits per sample	2 - 65,535
	FileHeader[0x24] = 'd';
	FileHeader[0x25] = 'a';
	FileHeader[0x26] = 't';
	FileHeader[0x27] = 'a';
	CurrentWaveDataOffsetInFile = 0x2c;
	WriteFile(FileHandle, FileHeader, 0x2c, &BytesWritten, NULL);

	// разрешаем обновлять буферы
	WaveDataOut.IsSaveWave = 1;
}

void UpdateWAVEFile_real()
{
	static short TempBuf[WAVEDATAOUTSIZE * 2];
	DWORD BytesWritten;

	// сохранить очередную порцию записанных в буфер данных на диск в файл
	int FirstPos = WaveDataOut.PosRead;
	int AfterLastPos = WaveDataOut.PosWrite;

	SetFilePointer(FileHandle, CurrentWaveDataOffsetInFile, NULL, FILE_BEGIN);

	int CurrentPackageDataLen;
	// записать новые двоичные данные сэмплов в файл
	if(AfterLastPos > FirstPos)
	{
		CurrentPackageDataLen = AfterLastPos - FirstPos;
	}
	else
	{
		CurrentPackageDataLen = (AfterLastPos + WAVEDATAOUTSIZE) - FirstPos;
	}
	for(int i = 0; i < CurrentPackageDataLen; i++)
	{
		// записать 2 байта левого сэмпла
		TempBuf[i*2] = WaveDataOut.Left[FirstPos];
		// записать 2 байта правого сэмпла
		TempBuf[(i*2)+1] = WaveDataOut.Right[FirstPos];

		// инкрементировать кол-во записанных сэмплов
		TotalSamplesWrittenInBytes += 4;
		CurrentWaveDataOffsetInFile += 4;

		FirstPos++;
		if(FirstPos == WAVEDATAOUTSIZE)
		{
			FirstPos = 0;
		}
	}
	WriteFile(FileHandle, TempBuf, CurrentPackageDataLen * 4, &BytesWritten, NULL);

	// обновить размер файла
	SetFilePointer(FileHandle, OFFSET_FILESIZE_MINUS_8, NULL, FILE_BEGIN);
	UINT FileSize_minus_8 = 0x2c + TotalSamplesWrittenInBytes - 8;
	WriteFile(FileHandle, &FileSize_minus_8, 4, &BytesWritten, NULL);

	// обновить инфомрацию о кол-ве сэмплов в файле
	SetFilePointer(FileHandle, OFFSET_SAMPLES_COUNT, NULL, FILE_BEGIN);
	WriteFile(FileHandle, &TotalSamplesWrittenInBytes, 4, &BytesWritten, NULL);

	WaveDataOut.PosRead = AfterLastPos;
}

void CloseWAVEFile()
{
	// запрещаем обновлять буферы
	WaveDataOut.IsSaveWave = 0;

	// записать хвост
	WaveDataOut.CanRealWriteBuf = 0;
	UpdateWAVEFile_real();

	TotalSamplesWrittenInBytes = 0;
	CurrentWaveDataOffsetInFile = 0;
	WaveDataOut.PosRead = 0;
	WaveDataOut.PosWrite = 0;
	// справа снизу в пустом статике написать "имяфайла saved"
	char ss[256];
	wsprintf(ss,"%s saved", Filename);
	SaveFileLabelSeconds = 2;
	SetDlgItemText(H_MAINWINDOW, ID_LABEL12, ss);
	// закрыть файл
	CloseHandle(FileHandle);
}

// функция сохранения выходного аудио-потока в .wav-файл
// вызывается раз в секунду
void UpdateWAVEFile()
{
	if(SaveFileLabelSeconds > 0)
	{
		SaveFileLabelSeconds--;
	}
	else
	{
		SetDlgItemText(H_MAINWINDOW, ID_LABEL12, "");
	}

	if((WaveDataOut.IsSaveWave == 1) && (WaveDataOut.CanRealWriteBuf == 1)) // если программа находится в режиме записи в файл и в буфере для записи есть данные
	{
		UpdateWAVEFile_real();
	}
}


long long VolumeMeterPeakCnt[2] = {0, 0};	// кол-во рисуемых штук
																				// за основу берется среднее абсолютное(без знака) значение сэмплов за 33 мсекунды (30 гц)
																				// причем при выборе правый-левый в каждом сэмпле выбирается максимальный
int VolumeMeterPeakCntCounter[2] = {0, 0};

unsigned long get_sys_reference_time() 
{	// get the system reference time
	return timeGetTime(); // winmm library (wineg++ -lwinmm)
}

LRESULT CALLBACK WndProc(HWND H_HWND, UINT Message, WPARAM wparam,LPARAM lparam)
{
	UINT code=HIWORD(wparam);       // код уведомления
	UINT idCtrl=LOWORD(wparam);     // идентификатор дочернего окна
	HWND hChild=(HWND)lparam;       // дескриптор дочернего окна

	// Координаты курсора мыши
	WORD xPos = LOWORD(lparam);
	WORD yPos = HIWORD(lparam);

	int i, j;

	static HBRUSH myBrush = CreateSolidBrush(RGB(255, 255, 255));
	static HPEN myPenPink = CreatePen(PS_SOLID, 8, RGB(255, 221, 232));
	static HPEN myPenBlack = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	static HPEN myPenRed = CreatePen(PS_SOLID, 5, RGB(255, 75, 75));
	static HPEN myPenWhileBold = CreatePen(PS_SOLID, 4, RGB(255, 255, 255));
	static HPEN myPenGreenBold = CreatePen(PS_SOLID, 4, RGB(75, 255, 75));
	//static HPEN myPenVolMeter = CreatePen(PS_SOLID, 2, RGB(128, 0, 0));
	static HPEN myPenVolMeter = CreatePen(PS_SOLID, 2, RGB(255, 151, 182));

	static const int VolumeMeterX[2 + MAX_NET_USERS] = {38,  38, 316, 316, 316, 316, 316, 316, 316, 316};
	static const int VolumeMeterY[2 + MAX_NET_USERS] = {88 - 10, 128 - 10, 218 + 50 + VstM_Pix, 251 + 50 + VstM_Pix, 284 + 50 + VstM_Pix, 317 + 50 + VstM_Pix, 350 + 50 + VstM_Pix, 383 + 50 + VstM_Pix, 416 + 50 + VstM_Pix, 449 + 50 + VstM_Pix};

	switch (Message)
	{
		case WM_PAINT:
		{
			// add 17.12.2012 нарисовать розовую хуйню
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(H_MAINWINDOW, &ps);
			SelectObject(hDC, myPenPink);
			Rectangle(hDC, 10, 10, 550, 533 + VstM_Pix);
			SelectObject(hDC, myPenBlack);
			Rectangle(hDC, 315, 23, 316, 140);
			Rectangle(hDC, 32, 147, 533, 148);
			Rectangle(hDC, 367, 152, 368, 200 + 46 + VstM_Pix-4);
			Rectangle(hDC, 32, 205 + 46 + VstM_Pix-4, 533, 206 + 46 + VstM_Pix-4);
			EndPaint(H_MAINWINDOW, &ps);
			break;
		}
		case WM_CTLCOLORSTATIC: // add 17.12.2012
        {
			return (LRESULT)myBrush;
        }

		case WM_NOTIFY: // это сообщение шлют ползунки msctls_trackbar32
			if(idCtrl == ID_SLIDER_OUTPUT_VOLUME)
			{
				VolumeOutput = SendDlgItemMessage(H_HWND, ID_SLIDER_OUTPUT_VOLUME, TBM_GETPOS, 0, 0l);
				SetDlgItemInt(H_HWND, ID_LABEL1, VolumeOutput, true);
			}
			break;
		case WM_COMMAND:
			if(code == STN_CLICKED) // 17.12.2012 обработка одиночных нажатий по текстовым полям static text
			{
				if (idCtrl == ID_BUTTON_ASIOCONFIG)
				{
					alsaexchange_show();//run_func_drom_dll(); // linux: обработчик нажатия кнопки конфигурации асио, здесь можно вызвать окно альсы
				}
				
				if(idCtrl == ID_BTN_SHOWVST)
				{
					if(MyVST_active == 1)
					{
						MyVST_WindowThread = CreateThread(NULL, 0, MyVST_StartWindow, NULL, 0, NULL);
					}
				}
				
				if(idCtrl == ID_BUTTON_SAMPLES_64)
                {
                    alsaexchange_alsa_stop(ALSADriverCurrentIndex);
                    ALSADriverSampleCount = 64;
                    alsaexchange_alsa_run(ALSADriverCurrentIndex, 44100, ALSADriverSampleCount);
                }
                
                if(idCtrl == ID_BUTTON_SAMPLES_128)
                {
                    alsaexchange_alsa_stop(ALSADriverCurrentIndex);
                    ALSADriverSampleCount = 128;
                    alsaexchange_alsa_run(ALSADriverCurrentIndex, 44100, ALSADriverSampleCount);
                }
                
                if(idCtrl == ID_BUTTON_SAMPLES_256)
                {
                    alsaexchange_alsa_stop(ALSADriverCurrentIndex);
                    ALSADriverSampleCount = 256;
                    alsaexchange_alsa_run(ALSADriverCurrentIndex, 44100, ALSADriverSampleCount);
                }
			}

			if (idCtrl == ID_CHECKBOX_SAVEWAVE && code == BN_CLICKED)
			{
				if (SendMessage(H_CB_SAVEWAVE, BM_GETCHECK, 0, 0l) != 0)
				{
					CreateWAVEFile();
                }
				else
				{
					CloseWAVEFile();
                }
            }

			//VSTPath MyVST_DllFilename
			if(idCtrl == ID_VSTLIST && code == CBN_SELCHANGE)
			{
				char CBText[1024];
				int idx = SendMessage(H_VSTLIST, CB_GETCURSEL, 0, 0);
				SendMessage(H_VSTLIST, CB_GETLBTEXT, idx, (LPARAM)CBText);
				
				wsprintf(MyVST_DllFilename, "%s\\%s", VSTPath, CBText);//второй параметр - текст из самого комбобокса
				if(MyVST_active == 1) MyVST_Close();
				if(MyVST_Init() == 0) MyVST_active = 1;
			}


			if(idCtrl == ID_ASIOLIST && code == CBN_SELCHANGE)
            {
				// linux: обработчик выбора другой строчки в списке драйверов асио, потом можно приспособить под список альса драйверов
                alsaexchange_alsa_stop(ALSADriverCurrentIndex);
                ALSADriverCurrentIndex = SendMessage(H_ASIOLIST, CB_GETCURSEL, 0, 0);
                alsaexchange_alsa_run(ALSADriverCurrentIndex, 44100, ALSADriverSampleCount);
            }

			break;

		case WM_LBUTTONDBLCLK:
			// двойной щелчок по меткам сбрасывает настройки громкости:
			// выходная громкость VOL_LBL_OUT
			if(xPos >= VolLabel[VOL_LBL_OUT].x && xPos <= VolLabel[VOL_LBL_OUT].x + VolLabel[VOL_LBL_OUT].lenX && yPos >= VolLabel[VOL_LBL_OUT].y && yPos <= VolLabel[VOL_LBL_OUT].y + VolLabel[VOL_LBL_OUT].lenY)
			{
				SendDlgItemMessage(H_MAINWINDOW, ID_SLIDER_OUTPUT_VOLUME, TBM_SETPOS, (WPARAM)1, (LPARAM)100l);
			}
			break;

		case WM_TIMER:
			if(idCtrl == ID_TIMER1) // 1 раз в секунду
			{
				MySystemTime = get_sys_reference_time();
				

				SetDlgItemInt(H_HWND, ID_LABEL3, ALSADriverSampleCount, true);

				UpdateWAVEFile();


				HDC hDC = GetDC(H_MAINWINDOW);
				SelectObject(hDC, myPenWhileBold);
				i = 1;//for(i = 0; i < 2 + MAX_NET_USERS; i++)
				{
					Rectangle(hDC, VolumeMeterX[i] - 2,  VolumeMeterY[i],      VolumeMeterX[i] + 96 + 3, VolumeMeterY[i] + 1);
					Rectangle(hDC, VolumeMeterX[i],      VolumeMeterY[i],      VolumeMeterX[i] + 1,      VolumeMeterY[i] + 14);
					Rectangle(hDC, VolumeMeterX[i] + 96, VolumeMeterY[i],      VolumeMeterX[i] + 97,     VolumeMeterY[i] + 14);
					Rectangle(hDC, VolumeMeterX[i] - 2,  VolumeMeterY[i] + 14, VolumeMeterX[i] + 96 + 3, VolumeMeterY[i] + 15);
				}
				ReleaseDC(H_MAINWINDOW, hDC);
			}
			if(idCtrl == ID_TIMER2) // 15 раз в секунду
			{
				//PAINTSTRUCT ps;
				HDC hDC = GetDC(H_MAINWINDOW);//BeginPaint(H_MAINWINDOW, &ps);
				SelectObject(hDC, myPenBlack);

				int myVolumeMeterPeakCnt[2 + MAX_NET_USERS];

				i = 1;//for(i = 0; i < 2 + MAX_NET_USERS; i++)
				{
					if(VolumeMeterPeakCntCounter[i] != 0)
					{
						myVolumeMeterPeakCnt[i] = (int)((long long)(VolumeMeterPeakCnt[i] / VolumeMeterPeakCntCounter[i]));
						for(j = 0; j < 15; j++)
						{
							if((myVolumeMeterPeakCnt[i] >> (30 - j)) & 1) // в общем эта штука делает так, чтобы каждый квадратик шкалы монитора соответствовал 15 старшим битам абсолютного среднего значения сэмпла
							{
								myVolumeMeterPeakCnt[i] = 15 - j;
								break;
							}
						}
						if(j == 15)
						{
							myVolumeMeterPeakCnt[i] = 0;
						}

					}
					else
					{
						myVolumeMeterPeakCnt[i] = 0;
					}
					VolumeMeterPeakCnt[i] = 0;
					VolumeMeterPeakCntCounter[i] = 0;
				}

				// 19.12.2012 ползунки уровней сигналов
				
				i = 1;//for(i = 0; i < 2 + MAX_NET_USERS; i++)
				{
					if(myVolumeMeterPeakCnt[i] == 15) SelectObject(hDC, myPenRed);
					Rectangle(hDC, VolumeMeterX[i], VolumeMeterY[i], VolumeMeterX[i] + 96, VolumeMeterY[i] + 14);
					if(myVolumeMeterPeakCnt[i] == 15) SelectObject(hDC, myPenBlack);
				}
				i = 1;//for(i = 0; i < 2 + MAX_NET_USERS; i++)
				{
					SelectObject(hDC, myPenVolMeter);
					//if(VolumeMeterPeakCnt[i] > 15) VolumeMeterPeakCnt[i] = 15;
					for(j = 0; j < myVolumeMeterPeakCnt[i]; j++)
					{
						Rectangle(hDC, VolumeMeterX[i] + 5 + j*6, VolumeMeterY[i] + 4, VolumeMeterX[i] + 7 + j*6, VolumeMeterY[i] + 10);
					}
				}

				ReleaseDC(H_MAINWINDOW, hDC);//EndPaint(H_MAINWINDOW, &ps);
			}
        	break;

		case WM_DESTROY:
            free_dll();//linux free lib
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(H_HWND,Message,wparam,lparam);
	}
	return 0;
}




void SaveDataToWaveBuf(int* Left, int* Right, int Count) // сохранение выходного аудио-потока в буфер для последующей записи в .wav-файл раз в секунду в основном алгоритме по таймеру
{
	int i = 0;
	while(i < Count)
	{
		WaveDataOut.Left[WaveDataOut.PosWrite] = (short)((int)(Left[i] / (int)65536));
		WaveDataOut.Right[WaveDataOut.PosWrite] = (short)((int)(Right[i] / (int)65536));

		WaveDataOut.PosWrite++;
		if(WaveDataOut.PosWrite == WAVEDATAOUTSIZE)
		{
			WaveDataOut.PosWrite = 0;
		}
		i++;
	}
	WaveDataOut.CanRealWriteBuf = 1;
}


int FillOutputBuffers(int * LeftOutput, int * RightOutput, int SamplesCount)
{	// the actual processing callback.
	

	// buffer size in samples
	long buffSize = SamplesCount;
    //ALSADriverSampleCount = SamplesCount; // linux: for VST informing
    
    int LeftInput[512], RightInput[512]; // DUMMY
    memset(LeftInput, 0, 512 * 4);
    memset(RightInput, 0, 512 * 4);

	////
	// CUSTOM BUFFER UPDATE CODE START
	////

	//if ((asioDriverInfo.channelInfos[0].type == ASIOSTInt32LSB) && (asioDriverInfo.channelInfos[1].type == ASIOSTInt32LSB) &&
		//(asioDriverInfo.channelInfos[OutputCh].type == ASIOSTInt32LSB) && (asioDriverInfo.channelInfos[OutputCh+1].type == ASIOSTInt32LSB))
	{
		
		long long PeakMeterSum = 0;
		
		
		// здесь мы имеем входные данные(плюс, к сожалению, данные входного .вав-файла) с учетом галочек левого/правого канала и с учетом громкости
		// если подгружена VST-dll, то обработать эти данные ею
		if(MyVST_active == 1)
		{
			// MyVST_NumInputs MyVST_NumOutputs MyVST_Inputs[i][j] MyVST_Outputs[i][j]
			// ((int*)(LeftOutput))[NewBufPos]
			// ((int*)(RightOutput))[NewBufPos]

			// если (кол-во входных буферов VST-dll > 0) то каждый сэмпл левого канала в выходной буфер VST-dll
			if(MyVST_NumInputs > 0)
			{
				for(int NewBufPos = 0; NewBufPos < buffSize; NewBufPos++)
				{
					MyVST_Inputs[0][NewBufPos] = (float)((double)(((int*)(LeftOutput))[NewBufPos]) / 2147483647.);
				}
			}

			// если (кол-во входных буферов VST-dll > 1) то каждый сэмпл парвого канала в выходной буфер VST-dll
			if(MyVST_NumInputs > 1)
			{
				for(int NewBufPos = 0; NewBufPos < buffSize; NewBufPos++)
				{
					MyVST_Inputs[1][NewBufPos] = (float)((double)(((int*)(RightOutput))[NewBufPos]) / 2147483647.);
				}
			}


			// обработать вст-шечкой - вот тут может поломаться ибо функция VST-dll вызывается напрямую из АСИО-драйвера
			MyVST_BufferUpdate();

			// если (кол-во выходных буферов VST-dll > 0) то каждый сэмпл из выходного буфера VST-dll в левый канал
			if(MyVST_NumOutputs > 0)
			{
				for(int NewBufPos = 0; NewBufPos < buffSize; NewBufPos++)
				{
					((int*)(LeftOutput))[NewBufPos] = (int)((double)MyVST_Outputs[0][NewBufPos] * 2147483647.);
				}
			}

			// если (кол-во выходных буферов VST-dll > 1) то каждый сэмпл из выходного буфера VST-dll в парвый канал
			if(MyVST_NumOutputs > 1)
			{
				for(int NewBufPos = 0; NewBufPos < buffSize; NewBufPos++)
				{
					((int*)(RightOutput))[NewBufPos] = (int)((double)MyVST_Outputs[1][NewBufPos] * 2147483647.);
				}
			}


		}


		// применить выходную громкость
		for(int NewBufPos = 0; NewBufPos < buffSize; NewBufPos++)
		{
			// применить громкость VolumeInput к сэмплу левого канала
			(((int*)(LeftOutput))[NewBufPos]) = (int)((double)(((int*)(LeftOutput))[NewBufPos]) * (double)(VolumeOutput)/100.f);
			// применить громкость VolumeInput к сэмплу правого канала
			(((int*)(RightOutput))[NewBufPos]) = (int)((double)(((int*)(RightOutput))[NewBufPos]) * (double)(VolumeOutput)/100.f);

			// для монитора шкалы сигнала
			if(abs(((int*)(LeftOutput))[NewBufPos]) > abs(((int*)(RightOutput))[NewBufPos]))
			{
				PeakMeterSum += abs(((int*)(LeftOutput))[NewBufPos]);
			}
			else
			{
				PeakMeterSum += abs(((int*)(RightOutput))[NewBufPos]);
			}
		}

		
		PeakMeterSum /= buffSize; VolumeMeterPeakCnt[1] += PeakMeterSum; VolumeMeterPeakCntCounter[1]++; // для монитора шкалы выходного сигнала

		// если признак записи выходного потока в файл == 1 то сохранить очередную понцию аудиоданных в буфер сохранения в файл
		if(WaveDataOut.IsSaveWave == 1)
		{
			SaveDataToWaveBuf((int*)(LeftOutput), (int*)(RightOutput), buffSize);
		}
	}
	////
	// CUSTOM BUFFER UPDATE CODE END
	////

	

	return 0L;
}


