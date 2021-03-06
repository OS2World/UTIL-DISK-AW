#define NUL	'\000'
#define BEL	'\007'
#define BS	'\010'
#define HT	'\011'
#define LF	'\012'
#define CR	'\015'
#define ESC	'\033'
#define SP	'\040'

struct TTYWND {
    HWND hWnd;
    POINTL Pos;
    BYTE *pVidBuf;
    short CWidth, CHeight;
    short Top, Left;
    short Width, Height;
    short MaxLines,MaxCols;
    short MaxLineLength;
    short CurLineOffset;
    short oCurrentLine;
    short oVidLastLine;
    BOOL LFonCR;
    BOOL CRonLF;
    BOOL Wrap;
    BYTE ebitmask;
};
typedef struct TTYWND *PTTYWND;
extern struct TTYWND MWnd;

InitWindow(PTTYWND,short,short,short,short,short,short,BOOL,BOOL,BOOL,BYTE);
int NEAR Display(PTTYWND, short,BYTE *);
void NEAR WndPaint(PTTYWND, HPS,short);
                                                                                                                                                                             
