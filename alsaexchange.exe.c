#include <windows.h> 
#include <iostream> 
#include <stdio.h> 

using namespace std;

#define MAXMODULE 255 

typedef int (WINAPI*cfunc)(void*,int);

//cfunc NumberList;
cfunc alsaexchange_putsamples;
HINSTANCE hLib;

//int main(int argc, char *argv[]) 
int run_func_drom_dll()
{
    static int fRun = 1;
    if(fRun == 1)
    {
      //static HINSTANCE hLib;
      hLib=LoadLibrary(".\\alsaexchange.dll");
      if(hLib==NULL) {

           cout << "Unable to load library!" << endl;
           return -1;
      }
      //char mod[MAXMODULE];
      //GetModuleFileName((HMODULE)hLib, (LPTSTR)mod, MAXMODULE);
      //cout << "Library loaded: " << mod << endl;
      
      alsaexchange_putsamples=(cfunc)GetProcAddress((HMODULE)hLib, "alsaexchange_putsamples");
      
      fRun = 0;
    }
      
      if(alsaexchange_putsamples==NULL) {

           cout << "Unable to load function(s) alsaexchange_putsamples" << endl;
           //FreeLibrary((HMODULE)hLib);
           //return -1;
      }
      else
      {
          cout << "calling alsaexchange_putsamples((void*)0x4040abcd,-40404242).." << endl;
          int res = alsaexchange_putsamples((void*)0x4040abcd,-40404242);
          printf("alsaexchange_putsamples ret %i\n", res);
      }

     // FreeLibrary((HMODULE)hLib);
      return 0;
} 
int free_dll()
{
    if(hLib != NULL) FreeLibrary((HMODULE)hLib);
    return 0;
}



//#include <windows.h>

LRESULT CALLBACK MainWinProc(HWND,UINT,WPARAM,LPARAM);
#define ID_MYBUTTON 1    /* идентификатор для кнопочки внутри главного окна */


int WINAPI
//WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int ss) 

WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int ss)
{
 /* создаем и регистрируем класс главного окна */
 WNDCLASS wc;
 wc.style=0;
 wc.lpfnWndProc=MainWinProc;
 wc.cbClsExtra=wc.cbWndExtra=0;
 wc.hInstance=hInst;
 wc.hIcon=NULL;
 wc.hCursor=NULL;
 wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 wc.lpszMenuName=NULL;
 wc.lpszClassName="Example 4 MainWnd Class";
 if (!RegisterClass(&wc)) return FALSE;

 /* создаем главное окно и отображаем его */
 HWND hMainWnd=CreateWindow("Example 4 MainWnd Class","EXAMPLE 4",WS_OVERLAPPEDWINDOW,
  CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,hInst,NULL);
 if (!hMainWnd) return FALSE;
 ShowWindow(hMainWnd,ss);
 UpdateWindow(hMainWnd);

 MSG msg; /* цикл обработки событий */
 while (GetMessage(&msg,NULL,0,0)) {
  TranslateMessage(&msg); 
  DispatchMessage(&msg); 
 } 
 return msg.wParam; 
}

/* процедура обработки сообщений для главного окна */
LRESULT CALLBACK MainWinProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp) {
 switch (msg) {
  case WM_CREATE:
   /* при создании окна внедряем в него кнопочку */
   CreateWindow("button","run_func_drom_dll",WS_CHILD|BS_PUSHBUTTON|WS_VISIBLE,
    5,5,100,20,hw,(HMENU)ID_MYBUTTON,NULL,NULL);
   /* стиль WS_CHILD означает, что это дочернее окно и для него
    вместо дескриптора меню будет передан целочисленный идентификатор,
    который будет использоваться дочерним окном для оповещения 
    родительского окна через WM_COMMAND */
   return 0;
  case WM_COMMAND:
   /* нажата наша кнопочка? */
   if ((HIWORD(wp)==0) && (LOWORD(wp)==ID_MYBUTTON)) 
   {
    //MessageBox(hw,"You pressed my button","MessageBox",MB_OK|MB_ICONWARNING);
    run_func_drom_dll();
   }
   return 0;
  case WM_DESTROY:
      
    printf("WINAPI WM_DESTROY called\n");
    free_dll();
    
      
   /* пользователь закрыл окно, программа может завершаться */
   PostQuitMessage(0);
   return 0;
 }
 return DefWindowProc(hw,msg,wp,lp);
}

