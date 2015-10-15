#define INCL_PM
#include <os2.h>
extern HAB hAB;
extern HHEAP hHeap;
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "scr.h"

static void NEAR DoLF(PTTYWND pTTYWnd);
static void NEAR DoCR(PTTYWND pTTYWnd);
static void NEAR DoBS(PTTYWND pTTYWnd);
static void NEAR DoTab(PTTYWND pTTYWnd);

InitWindow(pWnd,left,top,width,height,charwidth,charheight,LFonCR,CRonLF,wrap,mask)
PTTYWND pWnd;
short left, top, width, height, charwidth, charheight;
BOOL LFonCR, CRonLF, wrap;
BYTE mask;
{

    int bufsize;

    pWnd->Left = left;
    pWnd->Top = top;
    pWnd->Width = width;
    pWnd->Height = height;
    pWnd->CWidth = charwidth;
    pWnd->CHeight = charheight;
    pWnd->MaxLines = height / charheight;
    pWnd->MaxCols = width / charwidth;
    pWnd->MaxLineLength = pWnd->MaxCols + 1;
    bufsize = pWnd->MaxLines * pWnd->MaxLineLength;

    if (pWnd->pVidBuf = WinAllocMem(hHeap, bufsize))
    {
	memset(pWnd->pVidBuf, NUL, bufsize);
	pWnd->Pos.y = 0;
	pWnd->Pos.x = 0;
	pWnd->oCurrentLine = pWnd->CurLineOffset = 0;
	pWnd->oVidLastLine = (pWnd->MaxLines-1) * pWnd->MaxLineLength;
	pWnd->LFonCR = LFonCR;
	pWnd->CRonLF = CRonLF;
	pWnd->Wrap = wrap;
	pWnd->ebitmask = mask;
	return TRUE;
    }
    return FALSE;
}

int NEAR Display(pWnd,len,str)
PTTYWND pWnd;
short len;
BYTE *str;
{
    HPS hPS;
    register BYTE *ptr;
    register short ctr;
    short toff, txpos;
    BYTE *tbuf;

    short cols = pWnd->MaxCols;
    short cwidth = pWnd->CWidth;
    BYTE mask = pWnd->ebitmask;
    while (len)
    {
	ptr = str;
	ctr = 0;
	txpos = (short)pWnd->Pos.x;
	tbuf = pWnd->pVidBuf + pWnd->oCurrentLine;
	toff = pWnd->CurLineOffset;
	while((*ptr &= mask) >= SP)
	{
	    if ((len) && (toff < cols))
	    {
		ctr += 1;
		*(tbuf + toff++) = *ptr++;
		txpos += cwidth;
		len -= 1;
	    }
	    else
		break;
	}
	if (ctr)
	{
	    hPS = WinGetPS(pWnd->hWnd);
	    GpiSetBackMix(hPS, BM_OVERPAINT);
	    GpiCharStringAt(hPS,(PPOINTL)&pWnd->Pos,(LONG)ctr,(PCH)str);
	    GpiRestorePS(hPS, -1L);
	    WinReleasePS (hPS);
	    if (toff < cols)
	    {
		pWnd->CurLineOffset = toff;
		pWnd->Pos.x = (LONG)txpos;
	    }
	    else if(pWnd->Wrap)
		{
		    DoCR(pWnd);
		    DoLF(pWnd);
		}
	}
	while ((*ptr &= mask) < SP)
	{
	    if (len)
	    {
		switch(*ptr)
		{
		    case BEL:
			WinAlarm(HWND_DESKTOP, WA_ERROR);
			break;
		    case HT:
			DoTab(pWnd);
			break;
		    case CR:
			DoCR(pWnd);
			if (pWnd->LFonCR)
			    DoLF(pWnd);
			break;
		    case LF:
			if (pWnd->CRonLF)
			    DoCR(pWnd);
			DoLF(pWnd);
			break;
		    case BS:
			DoBS(pWnd);
			break;
		}
		len -= 1;
		ptr++;
	    }
	    else
		break;
	}
	str = ptr;
    }
    return (len);
}

static void NEAR DoCR(pWnd)
PTTYWND pWnd;
{
    pWnd->CurLineOffset = 0;
    pWnd->Pos.x = 0;
}

static void NEAR DoLF(pWnd)
PTTYWND pWnd;
{
    BYTE *pCurr;
    short offset, cols;
    int i;
    HWND hWnd = pWnd->hWnd;
    cols = pWnd->MaxCols;

    if ((pWnd->oCurrentLine+=pWnd->MaxLineLength) > pWnd->oVidLastLine)
	pWnd->oCurrentLine = 0;
    pCurr = pWnd->pVidBuf + pWnd->oCurrentLine;
    offset = pWnd->CurLineOffset;

    for (i = 0; i < offset; i++)
	*(pCurr + i) = SP;
    for (i = offset; i < cols; i++)
	*(pCurr + i) = NUL;

    WinScrollWindow(hWnd,0,pWnd->CHeight,NULL,NULL,NULL,NULL,SW_INVALIDATERGN);
    WinUpdateWindow(hWnd);
}

static void NEAR DoBS(pWnd)
PTTYWND pWnd;
{
    if (pWnd->CurLineOffset > 0)
    {
	pWnd->Pos.x -= (LONG)pWnd->CWidth;
	pWnd->CurLineOffset -= 1;
    }
}

static void NEAR DoTab(pWnd)
PTTYWND pWnd;
{
    short curoffset, xpos, ypos, cols, cwidth;
    BYTE *pCurr;
    RECTL myrect;
    cols = pWnd->MaxCols;
    curoffset = pWnd->CurLineOffset;
    cwidth = pWnd->CWidth;

    if (curoffset < (cols - 1))
    {
	xpos = (short)pWnd->Pos.x;
	ypos = (short)pWnd->Pos.y;
	pCurr = pWnd->pVidBuf + pWnd->oCurrentLine;
	do
	{
	    if (*(pCurr + curoffset) == NUL)
		*(pCurr + curoffset) = SP;
	    curoffset += 1;
	    xpos += cwidth;
	}while ((curoffset % 8 != 0) && (curoffset < (cols - 1)));
	WinSetRect(hAB,&myrect,xpos,ypos,pWnd->Width,ypos+pWnd->CHeight);
	WinInvalidateRect(pWnd->hWnd, &myrect, TRUE);
	pWnd->Pos.x = xpos;
	pWnd->CurLineOffset = curoffset;
    }
}

void NEAR WndPaint(pWnd,hPS,top)
PTTYWND pWnd;
HPS hPS;
SHORT top;
{
    POINTL pt;
    BYTE *lineptr;
    BYTE *pBuf, *pEnd;
    short lines, length;
    short cheight;
    int i;

    GpiErase (hPS);
    pt.x = 0;
    pBuf = pWnd->pVidBuf;
    pEnd = pBuf + pWnd->oVidLastLine;

    lineptr = pBuf + pWnd->oCurrentLine;
    pt.y = pWnd->Pos.y;

    length = pWnd->MaxLineLength;
    cheight = pWnd->CHeight;
    lines = top / cheight;
    if (top % cheight)
	lines += 1;
    lines = min(pWnd->MaxLines, lines);
    for (i = 0; i < lines; i++)
    {
	GpiCharStringAt(hPS,&pt,(LONG)strlen(lineptr),(PCH)lineptr);
	pt.y += (LONG)cheight;
	if ((lineptr -= length) < pBuf)
	    lineptr = pEnd;
    }
}
