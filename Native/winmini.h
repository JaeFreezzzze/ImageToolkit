#ifndef WINMINI_H
#define WINMINI_H
#include <stddef.h>
#ifdef _MSC_VER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#endif
#include <stdint.h>
typedef uint32_t DWORD, UINT, ULONG, COLORREF, ARGB;
typedef int32_t LONG, BOOL;
typedef uint16_t WORD, ATOM;
typedef unsigned char BYTE;
typedef uintptr_t WPARAM, ULONG_PTR;
typedef intptr_t LPARAM, LRESULT;
typedef void *HANDLE, *HWND, *HINSTANCE, *HICON, *HCURSOR, *HBRUSH, *HDC, *HGDIOBJ, *HFONT, *HBITMAP, *HMENU;
typedef wchar_t WCHAR;
typedef struct { LONG x,y; } POINT;
typedef struct { LONG left,top,right,bottom; } RECT;
typedef LRESULT (*WNDPROC)(HWND,UINT,WPARAM,LPARAM);
typedef DWORD (*THREADPROC)(void*);
typedef struct { UINT cbSize,style; WNDPROC lpfnWndProc; int cbClsExtra,cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground; const WCHAR *lpszMenuName,*lpszClassName; HICON hIconSm; } WNDCLASSEXW;
typedef struct { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; DWORD lPrivate; } MSG;
typedef struct { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore,fIncUpdate; BYTE rgbReserved[32]; } PAINTSTRUCT;
typedef struct { POINT ptReserved,ptMaxSize,ptMaxPosition,ptMinTrackSize,ptMaxTrackSize; } MINMAXINFO;
typedef struct { DWORD lStructSize; HWND hwndOwner; HINSTANCE hInstance; const WCHAR *lpstrFilter; WCHAR *lpstrCustomFilter; DWORD nMaxCustFilter,nFilterIndex; WCHAR *lpstrFile; DWORD nMaxFile; WCHAR *lpstrFileTitle; DWORD nMaxFileTitle; const WCHAR *lpstrInitialDir,*lpstrTitle; DWORD Flags; WORD nFileOffset,nFileExtension; const WCHAR *lpstrDefExt; LPARAM lCustData; void *lpfnHook; const WCHAR *lpTemplateName; void *pvReserved; DWORD dwReserved,FlagsEx; } OPENFILENAMEW;
typedef struct { DWORD lStructSize; HWND hwndOwner; HWND hInstance; COLORREF rgbResult; COLORREF *lpCustColors; DWORD Flags; LPARAM lCustData; void *lpfnHook; const WCHAR *lpTemplateName; } CHOOSECOLORW;
typedef struct { DWORD LowDateTime,HighDateTime; } FILETIME;
typedef struct { DWORD fileAttributes; FILETIME creation,access,write; DWORD sizeHigh,sizeLow; } WIN32_FILE_ATTRIBUTE_DATA;
typedef struct { uint32_t Data1; uint16_t Data2,Data3; uint8_t Data4[8]; } GUID;
typedef struct { GUID Clsid,FormatID; const WCHAR *CodecName,*DllName,*FormatDescription,*FilenameExtension,*MimeType; DWORD Flags,Version,SigCount,SigSize; const BYTE *SigPattern,*SigMask; } ImageCodecInfo;
typedef struct { UINT version; void *debug; BOOL suppressThread,suppressCodecs; } GdiplusStartupInput;
typedef struct { GUID Guid; ULONG NumberOfValues,Type; void *Value; } EncoderParameter;
typedef struct { UINT Count; EncoderParameter Parameter[1]; } EncoderParameters;
typedef struct { int32_t X,Y,Width,Height; } GpRect;
typedef struct { UINT Width,Height; int32_t Stride; int PixelFormat; void *Scan0; uintptr_t Reserved; } BitmapData;
typedef struct { UINT id,length; WORD type; void *value; } PropertyItem;
typedef void GpImage;
typedef struct { LONG type,width,height,widthBytes; WORD planes,bitsPixel; void *bits; } WINBITMAP;
extern BOOL OpenClipboard(HWND);
extern BOOL CloseClipboard(void);
extern HANDLE GetClipboardData(UINT);
extern UINT RegisterClipboardFormatW(const WCHAR*);
extern void *GlobalLock(HANDLE);
extern BOOL GlobalUnlock(HANDLE);
extern size_t GlobalSize(HANDLE);
extern HANDLE CopyImage(HANDLE,UINT,int,int,UINT);
extern int GetObjectW(HANDLE,int,void*);
extern short GetKeyState(int);
extern HMENU CreatePopupMenu(void);
extern BOOL AppendMenuW(HMENU,UINT,uintptr_t,const WCHAR*);
extern BOOL DestroyMenu(HMENU);
extern BOOL GetCursorPos(POINT*);
extern UINT TrackPopupMenu(HMENU,UINT,int,int,int,HWND,const RECT*);
extern int GdipCreateBitmapFromHBITMAP(HBITMAP,HANDLE,void**);
typedef void GpBitmap;
typedef void GpGraphics;
typedef void GpImageAttributes;

_Static_assert(sizeof(WCHAR)==2,"Windows UTF-16 required");
_Static_assert(sizeof(WNDCLASSEXW)==80,"WNDCLASS ABI");
_Static_assert(sizeof(MSG)==48,"MSG ABI");
_Static_assert(sizeof(PAINTSTRUCT)==72,"PAINTSTRUCT ABI");
_Static_assert(sizeof(OPENFILENAMEW)==152,"OPENFILENAME ABI");
_Static_assert(sizeof(CHOOSECOLORW)==72,"CHOOSECOLOR ABI");
_Static_assert(sizeof(ImageCodecInfo)==104,"Codec ABI");
_Static_assert(sizeof(GdiplusStartupInput)==24,"GDI+ startup ABI");
_Static_assert(sizeof(EncoderParameters)==40,"Encoder ABI");

extern HINSTANCE GetModuleHandleW(const WCHAR*);
extern DWORD GetModuleFileNameW(HINSTANCE,WCHAR*,DWORD);
extern const WCHAR *GetCommandLineW(void);
extern void ExitProcess(UINT);
extern DWORD GetLastError(void);
extern BOOL GetFileAttributesExW(const WCHAR*,int,WIN32_FILE_ATTRIBUTE_DATA*);
extern DWORD GetFileAttributesW(const WCHAR*);
extern DWORD GetFullPathNameW(const WCHAR*,DWORD,WCHAR*,WCHAR**);
extern BOOL MoveFileExW(const WCHAR*,const WCHAR*,DWORD);
extern BOOL CopyFileW(const WCHAR*,const WCHAR*,BOOL);
extern BOOL DeleteFileW(const WCHAR*);
extern UINT GetTempFileNameW(const WCHAR*,const WCHAR*,UINT,WCHAR*);
extern DWORD GetTempPathW(DWORD,WCHAR*);
extern BOOL CloseHandle(HANDLE);
extern HANDLE CreateThread(void*,size_t,THREADPROC,void*,DWORD,DWORD*);
extern HANDLE CreateFileW(const WCHAR*,DWORD,DWORD,void*,DWORD,DWORD,HANDLE);
extern BOOL WriteFile(HANDLE,const void*,DWORD,DWORD*,void*);
extern BOOL FlushFileBuffers(HANDLE);
extern ATOM RegisterClassExW(const WNDCLASSEXW*);
extern HWND CreateWindowExW(DWORD,const WCHAR*,const WCHAR*,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,void*);
extern LRESULT DefWindowProcW(HWND,UINT,WPARAM,LPARAM);
extern BOOL ShowWindow(HWND,int);
extern BOOL UpdateWindow(HWND);
extern BOOL DestroyWindow(HWND);
extern BOOL GetMessageW(MSG*,HWND,UINT,UINT);
extern BOOL TranslateMessage(const MSG*);
extern LRESULT DispatchMessageW(const MSG*);
extern BOOL IsDialogMessageW(HWND,MSG*);
extern void PostQuitMessage(int);
extern BOOL PostMessageW(HWND,UINT,WPARAM,LPARAM);
extern LRESULT SendMessageW(HWND,UINT,WPARAM,LPARAM);
extern BOOL SetWindowTextW(HWND,const WCHAR*);
extern int GetWindowTextW(HWND,WCHAR*,int);
extern BOOL EnableWindow(HWND,BOOL);
extern BOOL MoveWindow(HWND,int,int,int,int,BOOL);
extern BOOL GetClientRect(HWND,RECT*);
extern BOOL GetWindowRect(HWND,RECT*);
extern BOOL InvalidateRect(HWND,const RECT*,BOOL);
extern HDC BeginPaint(HWND,PAINTSTRUCT*);
extern BOOL EndPaint(HWND,const PAINTSTRUCT*);
extern int FillRect(HDC,const RECT*,HBRUSH);
extern int FrameRect(HDC,const RECT*,HBRUSH);
extern int DrawTextW(HDC,const WCHAR*,int,RECT*,UINT);
extern int MessageBoxW(HWND,const WCHAR*,const WCHAR*,UINT);
extern HWND SetFocus(HWND);
extern HCURSOR LoadCursorW(HINSTANCE,const WCHAR*);
extern HCURSOR SetCursor(HCURSOR);
extern HICON LoadIconW(HINSTANCE,const WCHAR*);
extern HANDLE LoadImageW(HINSTANCE,const WCHAR*,UINT,int,int,UINT);
extern BOOL DestroyIcon(HICON);
extern int GetSystemMetrics(int);
extern HDC GetDC(HWND);
extern int ReleaseDC(HWND,HDC);
extern BOOL SetProcessDPIAware(void);
extern BOOL GetOpenFileNameW(OPENFILENAMEW*);
extern BOOL GetSaveFileNameW(OPENFILENAMEW*);
extern DWORD CommDlgExtendedError(void);
extern BOOL ChooseColorW(CHOOSECOLORW*);
extern void DragAcceptFiles(HWND,BOOL);
extern UINT DragQueryFileW(HANDLE,UINT,WCHAR*,UINT);
extern void DragFinish(HANDLE);
extern WCHAR **CommandLineToArgvW(const WCHAR*,int*);
extern HANDLE LocalFree(HANDLE);
extern HBRUSH CreateSolidBrush(COLORREF);
extern HFONT CreateFontW(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,const WCHAR*);
extern HGDIOBJ SelectObject(HDC,HGDIOBJ);
extern BOOL DeleteObject(HGDIOBJ);
extern int SetBkMode(HDC,int);
extern COLORREF SetBkColor(HDC,COLORREF);
extern COLORREF SetTextColor(HDC,COLORREF);
extern HDC CreateCompatibleDC(HDC);
extern BOOL DeleteDC(HDC);
extern HBITMAP CreateCompatibleBitmap(HDC,int,int);
extern BOOL BitBlt(HDC,int,int,int,int,HDC,int,int,DWORD);
extern int GetDeviceCaps(HDC,int);
extern int GdiplusStartup(ULONG_PTR*,const GdiplusStartupInput*,void*);
extern void GdiplusShutdown(ULONG_PTR);
extern int GdipLoadImageFromFileICM(const WCHAR*,GpImage**);
extern int GdipDisposeImage(GpImage*);
extern int GdipGetImageWidth(GpImage*,UINT*);
extern int GdipGetImageHeight(GpImage*,UINT*);
extern int GdipCreateBitmapFromScan0(int,int,int,int,BYTE*,GpBitmap**);
extern int GdipCreateBitmapFromHICON(HICON,GpBitmap**);
extern int GdipGetImageGraphicsContext(GpImage*,GpGraphics**);
extern int GdipCreateFromHDC(HDC,GpGraphics**);
extern int GdipDeleteGraphics(GpGraphics*);
extern int GdipGraphicsClear(GpGraphics*,ARGB);
extern int GdipSetInterpolationMode(GpGraphics*,int);
extern int GdipSetPixelOffsetMode(GpGraphics*,int);
extern int GdipSetCompositingMode(GpGraphics*,int);
extern int GdipSetCompositingQuality(GpGraphics*,int);
extern int GdipDrawImageRectRectI(GpGraphics*,GpImage*,int,int,int,int,int,int,int,int,int,GpImageAttributes*,void*,void*);
extern int GdipCreateImageAttributes(GpImageAttributes**);
extern int GdipDisposeImageAttributes(GpImageAttributes*);
extern int GdipSetImageAttributesWrapMode(GpImageAttributes*,int,ARGB,BOOL);
extern int GdipSaveImageToFile(GpImage*,const WCHAR*,const GUID*,const EncoderParameters*);
extern int GdipGetImageEncodersSize(UINT*,UINT*);
extern int GdipGetImageEncoders(UINT,UINT,ImageCodecInfo*);
extern int GdipGetPropertyItemSize(GpImage*,UINT,UINT*);
extern int GdipGetPropertyItem(GpImage*,UINT,UINT,PropertyItem*);
extern int GdipImageRotateFlip(GpImage*,int);
extern int GdipImageGetFrameDimensionsCount(GpImage*,UINT*);
extern int GdipImageGetFrameDimensionsList(GpImage*,GUID*,UINT);
extern int GdipImageGetFrameCount(GpImage*,const GUID*,UINT*);
extern int GdipBitmapGetPixel(GpBitmap*,int,int,ARGB*);
extern int GdipBitmapSetPixel(GpBitmap*,int,int,ARGB);
extern int CoInitializeEx(void*,DWORD);
extern void CoUninitialize(void);
extern void *malloc(size_t);
extern void *calloc(size_t,size_t);
extern void free(void*);
extern void *memcpy(void*,const void*,size_t);
extern void *memset(void*,int,size_t);
extern size_t wcslen(const WCHAR*);
extern int wcscmp(const WCHAR*,const WCHAR*);
extern int _wcsicmp(const WCHAR*,const WCHAR*);
extern int _snwprintf(WCHAR*,size_t,const WCHAR*,...);

#define RGB(r,g,b) ((COLORREF)((r)|((g)<<8)|((b)<<16)))
#define RES(id) ((const WCHAR*)(uintptr_t)(id))
#define WM_CREATE 1
#define WM_DESTROY 2
#define WM_SIZE 5
#define WM_PAINT 15
#define WM_CLOSE 16
#define WM_ERASEBKGND 20
#define WM_SETCURSOR 32
#define WM_GETMINMAXINFO 36
#define WM_SETFONT 48
#define WM_KEYDOWN 256
#define WM_COMMAND 273
#define WM_CTLCOLORSTATIC 312
#define WM_LBUTTONUP 514
#define WM_DROPFILES 563
#define WM_APP_DONE 32769
#define WS_CHILD 0x40000000u
#define WS_VISIBLE 0x10000000u
#define WS_TABSTOP 0x00010000u
#define WS_BORDER 0x00800000u
#define WS_CLIPSIBLINGS 0x04000000u
#define WM_USER 1024
#define CB_ADDSTRING 323
#define CB_GETCURSEL 327
#define CB_SETCURSEL 334
#define BM_GETCHECK 240
#define BM_SETCHECK 241
#define EN_CHANGE 768
#define PIXEL_ARGB 0x26200a
extern BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*);
extern int GetWindowTextLengthW(HWND);
extern HANDLE GlobalAlloc(UINT,size_t);
extern HANDLE GlobalFree(HANDLE);
extern BOOL EmptyClipboard(void);
extern HANDLE SetClipboardData(UINT,HANDLE);
typedef struct {UINT CtlType,CtlID,itemID,itemAction,itemState;HWND hwndItem;HDC hDC;RECT rcItem;ULONG_PTR itemData;} DRAWITEMSTRUCT;
typedef struct {DWORD cbSize,dwFlags;HWND hwndTrack;DWORD dwHoverTime;} TRACKMOUSEEVENT;
typedef LRESULT (*SUBCLASSPROC)(HWND,UINT,WPARAM,LPARAM,ULONG_PTR,ULONG_PTR);
extern BOOL RoundRect(HDC,int,int,int,int,int,int);
extern HANDLE CreatePen(int,int,COLORREF);
extern BOOL MoveToEx(HDC,int,int,POINT*);
extern BOOL LineTo(HDC,int,int);
extern BOOL DrawFocusRect(HDC,const RECT*);
extern BOOL SetWindowSubclass(HWND,SUBCLASSPROC,ULONG_PTR,ULONG_PTR);
extern LRESULT DefSubclassProc(HWND,UINT,WPARAM,LPARAM);
extern BOOL TrackMouseEvent(TRACKMOUSEEVENT*);
extern BOOL IsWindowEnabled(HWND);
_Static_assert(sizeof(DRAWITEMSTRUCT)==64,"DRAWITEMSTRUCT ABI");

#endif
