#define AWICON	       1
#define IDNULL	       0
#define IDM_COMMANDS  10
#define IDM_PARA      11
#define IDMABOUT      13
#define IDM_EXITAW    80
#define IDM_RESUME    81
#define IDM_EXIT      82

#define IDD_ABOUT     20
#define IDD_PARA      21
#define IDD_FILE      22
#define IDD_LISTBOX   23
#define WM_PAINT_TTY  24
#define IDD_SYS       25
#define IDD_HID       26
#define IDD_ALL       27

MRESULT EXPENTRY awWndProc( HWND, USHORT, MPARAM, MPARAM );
MRESULT EXPENTRY AboutDlg( HWND, USHORT, MPARAM, MPARAM );
MRESULT EXPENTRY ParaDlg( HWND, USHORT, MPARAM, MPARAM );

SHORT cdecl main(void);
VOID find_dir(char *);
VOID search_dir(char *);
VOID FAR WndCreate(HWND);
VOID FAR SecondThread(VOID);
VOID EnableMenuItem(HWND, SHORT , BOOL);
