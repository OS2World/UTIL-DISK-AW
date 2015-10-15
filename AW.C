#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "aw.h"
#include "scr.h"
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#define CSTR	7

HAB	hAB;
HMQ	hmqaw;
HWND	hwndaw;
HWND	hwndawFrame;
HHEAP hHeap;
struct TTYWND MWnd;

CHAR	szClassName[] = "aw";
char far filename[20];
char far pathname[180];
SHORT	iSel = 0;
RECTL	rect;
USHORT all = TRUE;
char drive_str[27][4];
USHORT drive_sel[27];
USHORT num_drive = 0;
LONG SecSem;
TID idThread;
UCHAR cThreadStack[5120];
USHORT busy = FALSE;
USHORT file_attr = 0x0000;

SHORT cdecl main( )
{
    QMSG qmsg;
    ULONG ctldata;
    RECTL rect;

    hAB = WinInitialize(NULL);

    hmqaw = WinCreateMsgQueue(hAB, 0);
    if ((hHeap = WinCreateHeap(0, 0, 0, 0, 0, 0)) == NULL)
	return FALSE;

    if (!WinRegisterClass( hAB,
			   (PCH)szClassName,
			   (PFNWP)awWndProc,
			   CS_SYNCPAINT | CS_SIZEREDRAW,
			   0))
	return( 0 );

    ctldata = FCF_STANDARD  & ~FCF_SHELLPOSITION;;

    hwndawFrame = WinCreateStdWindow( HWND_DESKTOP,
					 WS_VISIBLE,
					 &ctldata,
					 (PCH)szClassName,
					 NULL,
					 0L,
					 (HMODULE)NULL,
					 AWICON,
					 (HWND FAR *)&hwndaw );
    WinQueryWindowRect( HWND_DESKTOP, (PRECTL)&rect );
    WinSetWindowPos(hwndawFrame,HWND_TOP,
	     (SHORT)rect.xLeft,(SHORT)rect.yTop  - 270,230,
	     (SHORT)270,
	     SWP_SIZE | SWP_MOVE |SWP_ACTIVATE);

    WinShowWindow( hwndawFrame, TRUE );

    MWnd.hWnd = hwndaw;

    while( WinGetMsg( hAB, (PQMSG)&qmsg, (HWND)NULL, 0, 0 ) )
    {
	WinDispatchMsg( hAB, (PQMSG)&qmsg );
    }
    DosSuspendThread(idThread);
    WinDestroyWindow( hwndawFrame );
    WinDestroyMsgQueue( hmqaw );
    WinTerminate( hAB );
}

void FAR WndCreate(HWND hWnd)
{
    FONTMETRICS FM;
    HPS hPS;
    short width, height, cwidth, cheight;

    DosSemSet(&SecSem);
    if(DosCreateThread(SecondThread,&idThread,cThreadStack + sizeof(cThreadStack)))
	WinAlarm(HWND_DESKTOP,WA_ERROR);
    hPS = WinGetPS(hWnd);
    GpiQueryFontMetrics(hPS, (LONG)sizeof(FONTMETRICS), &FM);
    cwidth = (short)(FM.lMaxBaselineExt + FM.lExternalLeading);
    cheight = (short)FM.lMaxAscender + (short)FM.lMaxDescender;
    WinReleasePS (hPS);

    width = (SHORT)WinQuerySysValue(HWND_DESKTOP,SV_CXFULLSCREEN);
    height = (SHORT)WinQuerySysValue(HWND_DESKTOP,SV_CYFULLSCREEN);
    InitWindow(&MWnd,0,0,width,height,cwidth,cheight,FALSE,TRUE,TRUE,0xff);
}

MRESULT EXPENTRY awWndProc( hWnd, msg, mp1, mp2 )
HWND   hWnd;
USHORT msg;
MPARAM mp1;
MPARAM mp2;
{
    HPS    hPS;
    if(!busy)
       EnableMenuItem(hwndaw,IDM_PARA,TRUE);
    switch (msg)
    {
	case WM_CREATE:
	     WndCreate(hWnd);
	     break;
	 case WM_COMMAND:
	     switch(LOUSHORT(mp1))
	     {
		  case IDM_EXITAW:
		       WinPostMsg( hWnd, WM_QUIT, 0L, 0L );
		       break;
		  case IDM_RESUME:
		       break;
		  case IDMABOUT:
		       WinDlgBox(HWND_DESKTOP,hWnd,(PFNWP)AboutDlg,NULL,IDD_ABOUT, NULL );
		       break;
		  case IDM_PARA:
		       WinDlgBox(HWND_DESKTOP,hWnd,(PFNWP)ParaDlg,NULL,IDD_PARA, NULL );
		       break;
	     }
	     break;
   case WM_MOVE:
	break;
   case WM_CLOSE:
	WinPostMsg( hWnd, WM_QUIT, 0L, 0L );
	break;

    case WM_PAINT:
	hPS = WinBeginPaint( hWnd, (HPS)NULL, (PWRECT)NULL );
	WinQueryWindowRect( hWnd, (PRECTL)&rect );
	WndPaint(&MWnd, hPS,(short)rect.yTop);
	WinEndPaint( hPS );
	break;

    case WM_ERASEBACKGROUND:
	return( TRUE );
	break;

    case WM_PAINT_TTY:
	Display(&MWnd,strlen(mp1), mp1);
	DosSemClear(&SecSem);
	break;

    default:
	return( WinDefWindowProc( hWnd, msg, mp1, mp2 ) );
	break;
    }
    return(0L);
}

MRESULT EXPENTRY AboutDlg( hWndDlg, message, mp1, mp2 )
HWND   hWndDlg;
USHORT message;
MPARAM mp1;
MPARAM mp2;
{
    switch( message )
    {
      case WM_COMMAND:  		/* the user has pressed a button */
	switch( SHORT1FROMMP( mp1 ) )	/* which button? */
	{
	  case DID_OK:
	  case DID_CANCEL:
	    WinDismissDlg( hWndDlg, TRUE );
	    break;

	  default:
	    return( FALSE );
	}
	break;

      default:
	return( WinDefDlgProc( hWndDlg, message, mp1, mp2 ) );
    }
    return( FALSE );
}

MRESULT EXPENTRY ParaDlg( hWndDlg, message, mp1, mp2 )
HWND   hWndDlg;
USHORT message;
MPARAM mp1;
MPARAM mp2;
{
   char drives;
   char str[5];
   USHORT indx;
   USHORT disk;
   ULONG drivemap;
   USHORT con;

    switch( message )
    {
	case WM_INITDLG:
	    DosQCurDisk(&disk,&drivemap);
	    for(drives = 'A'; drives <= 'Z'; drives++)
	    {
		 if(drivemap & 1)
		 {
		      str[0] = drives;
		      str[1] = ':';
		      str[2] = 0;
		      WinSendDlgItemMsg( hWndDlg, IDD_LISTBOX, LM_INSERTITEM,
				       (MPARAM)LIT_END, (MPARAM)(PCH)str );
		 }
		 drivemap >>= 1;
	    }
	    for(indx = 0;indx < num_drive;indx++)
	    WinSendDlgItemMsg( hWndDlg, IDD_LISTBOX, LM_SELECTITEM,
			     (MPARAM)(drive_sel[indx]), (MPARAM)TRUE );
	    if(indx == 0)
	    WinSendDlgItemMsg( hWndDlg, IDD_LISTBOX, LM_SELECTITEM,
			     (MPARAM)(disk - 1), (MPARAM)TRUE );
	    WinSetWindowText(WinWindowFromID(hWndDlg,IDD_FILE),filename);
	    if(file_attr & 0x0004)
		WinSendDlgItemMsg(hWndDlg,IDD_SYS,BM_SETCHECK,(MPARAM)TRUE,0L);
	    if(file_attr & 0x0002)
		WinSendDlgItemMsg(hWndDlg,IDD_HID,BM_SETCHECK,(MPARAM)TRUE,0L);
	    if(all)
		WinSendDlgItemMsg(hWndDlg,IDD_ALL,BM_SETCHECK,(MPARAM)TRUE,0L);
	    WinSendDlgItemMsg(hWndDlg,IDD_FILE,EM_SETTEXTLIMIT,(MPARAM)12,0L);
	    break;
	case WM_COMMAND:
	    switch( SHORT1FROMMP( mp1 ) )
	    {
		case DID_OK:
		    file_attr = 0x0000;
		    all = FALSE;
		    WinQueryDlgItemText(hWndDlg,IDD_FILE,sizeof(filename),(PCH)filename);
		    iSel =LIT_FIRST;
		    for(indx = 0,con = 1;con == 1;indx++)
		    {
			iSel =	(SHORT)WinSendDlgItemMsg( hWndDlg, IDD_LISTBOX,
							  LM_QUERYSELECTION,(MPARAM)iSel, 0L);
			if(iSel != LIT_NONE)
			{
			    WinSendDlgItemMsg(hWndDlg,IDD_LISTBOX,LM_QUERYITEMTEXT,
					      MPFROM2SHORT((SHORT)iSel, (SHORT)3), drive_str[indx]);
			    drive_sel[indx] = iSel;
			}
			else
			    con = 0;
		    }
		    num_drive = indx - 1;
		    if(WinSendDlgItemMsg(hWndDlg,IDD_SYS,BM_QUERYCHECK,0L,0L) == 1)
			file_attr = file_attr | 0x0004;
		    if(WinSendDlgItemMsg(hWndDlg,IDD_HID,BM_QUERYCHECK,0L,0L) == 1)
			file_attr = file_attr | 0x0002;
		    if(WinSendDlgItemMsg(hWndDlg,IDD_ALL,BM_QUERYCHECK,0L,0L) == 1)
			all = TRUE;
		    WinDismissDlg( hWndDlg, TRUE );
		    Display(&MWnd,21, "\nStarting search...\n");
		    busy = TRUE;
		    EnableMenuItem(hwndaw,IDM_PARA,FALSE);
		    DosSemClear(&SecSem);
		    break;

		case DID_CANCEL:
		    WinDismissDlg( hWndDlg, TRUE );
		    break;

		default:
		    return( FALSE );
	    }
	    break;

	 default:
	     return( WinDefDlgProc( hWndDlg, message, mp1, mp2 ) );
    }
    return( FALSE );
}

VOID FAR SecondThread()
{
   USHORT indx;

   while(1)
   {
	DosSemWait(&SecSem,-1L);
	busy = TRUE;
	for(indx= 0; indx < num_drive;indx++)
	{
	     find_dir(drive_str[indx]);
	}
	WinPostMsg(hwndaw,WM_PAINT_TTY,"File search complete !!",0L);
	WinAlarm(HWND_DESKTOP,WA_NOTE);
	busy = FALSE;
	DosSemSet(&SecSem);
   }
}

VOID find_dir(path)
char *path;
{
   char localpath[180];
   FILEFINDBUF buff;
   USHORT count = 1;
   USHORT handle = 0xFFFF;

   strcat(path,"\\");
   strcpy(localpath,path);
   strcat(localpath,"*.*");
   if(DosFindFirst(localpath,&handle,0x0010,&buff,sizeof(buff),&count,0L))
   {
	DosFindClose(handle);
	search_dir(path);
	return;
   }
   if(buff.achName[0] != '.' && (buff.attrFile & 0x0010))
   {
	strcpy(localpath,path);
	strcat(localpath,buff.achName);
	find_dir(localpath);
   }
   while(TRUE)
   {
	if(DosFindNext(handle,&buff,sizeof(buff),&count))
	{
	     DosFindClose(handle);
	     search_dir(path);
	     return;
	}
	if(buff.achName[0] != '.' && (buff.attrFile & 0x0010))
	{
	     strcpy(localpath,path);
	     strcat(localpath,buff.achName);
	     find_dir(localpath);
	}
    }
}

VOID search_dir(path)
char *path;
{
   char localpath[180];
   FILEFINDBUF buff;
   USHORT count = 1;
   USHORT handle = 0xFFFF;

   strcpy(localpath,path);
   strcat(localpath,filename);
   strset(buff.achName,0);
   if(DosFindFirst(localpath,&handle,file_attr,&buff,sizeof(buff),&count,0L))
   {
	DosFindClose(handle);
	return;
   }
   if(all || (buff.attrFile & file_attr))
   {
	DosSemWait(&SecSem,-1L);
	sprintf(pathname,"%s%s\n",path,buff.achName);
	DosSemSet(&SecSem);
	WinPostMsg(hwndaw,WM_PAINT_TTY,pathname,0L);
   }
   while(TRUE)
   {
	if(DosFindNext(handle,&buff,sizeof(buff),&count))
	{
	     DosFindClose(handle);
	     return;
	}
	DosSemWait(&SecSem,-1L);
	if(all || (buff.attrFile & file_attr))
	{
	    sprintf(pathname,"%s%s\n",path,buff.achName);
	    DosSemSet(&SecSem);
	    WinPostMsg(hwndaw,WM_PAINT_TTY,pathname,0L);
	}
   }
}

VOID EnableMenuItem(HWND hwnd, SHORT iMenuItem, BOOL bEnable)
{
   HWND hwndParent;
   HWND hwndMenu;

   hwndParent = WinQueryWindow(hwnd,QW_PARENT,FALSE);
   hwndMenu = WinWindowFromID(hwndParent,FID_MENU);
   
   WinSendMsg(hwndMenu,MM_SETITEMATTR,(MPARAM)MAKEULONG(iMenuItem,TRUE),
	     (MPARAM)MAKEULONG(MIA_DISABLED,bEnable ? 0 : MIA_DISABLED));
}
