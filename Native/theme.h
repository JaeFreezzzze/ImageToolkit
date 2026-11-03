/* Quiet, layered desktop styling, inspired by Fluent's content hierarchy. */
#define INK RGB(30,41,59)
#define MUTED RGB(112,124,143)
#define ACCENT RGB(55,100,231)
#define LINE RGB(226,232,240)
#define CANVAS RGB(246,248,252)
static HWND hotButton;
static RECT surfaces[3];static int surfaceCount;
static HFONT smallFont,monoFont;
static HBRUSH editBrush;
static RECT fieldFrames[4];static int fieldCount;
static void rounded(HDC dc,RECT r,int radius,COLORREF fill,COLORREF stroke){
 HBRUSH brush=CreateSolidBrush(fill);HANDLE pen=CreatePen(0,1,stroke);
 HGDIOBJ oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,pen);
 RoundRect(dc,r.left,r.top,r.right,r.bottom,px(radius),px(radius));
 SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);
}
static void line(HDC dc,int x1,int y1,int x2,int y2,COLORREF color,int width){
 HANDLE pen=CreatePen(0,px(width),color);HGDIOBJ old=SelectObject(dc,pen);MoveToEx(dc,x1,y1,0);LineTo(dc,x2,y2);SelectObject(dc,old);DeleteObject(pen);
}
static void label(HDC dc,const WCHAR *text,RECT r,HFONT font,COLORREF color,UINT flags){
 HGDIOBJ old=SelectObject(dc,font);SetBkMode(dc,1);SetTextColor(dc,color);DrawTextW(dc,text,-1,&r,flags|0x800);SelectObject(dc,old);
}
static LRESULT button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,ULONG_PTR id,ULONG_PTR data){
 (void)id;(void)data;
 if(msg==0x200&&hotButton!=hwnd){HWND old=hotButton;hotButton=hwnd;if(old)InvalidateRect(old,0,0);InvalidateRect(hwnd,0,0);TRACKMOUSEEVENT t={sizeof(t),2,hwnd,0};TrackMouseEvent(&t);}
 if(msg==0x2a3){if(hotButton==hwnd)hotButton=0;InvalidateRect(hwnd,0,0);}
 return DefSubclassProc(hwnd,msg,wp,lp);
}
static void paint_button(const DRAWITEMSTRUCT *d){
 int id=(int)d->CtlID,tab=id>=200&&id<=202,selected=tab&&page==id-200;
 int primary=id==302||id==308||id==CONVERT;
 int disabled=(d->itemState&4)!=0,pressed=(d->itemState&1)!=0,hover=hotButton==d->hwndItem;
 COLORREF fill=0xffffff,edge=LINE,text=INK;
 if(tab){fill=selected?RGB(232,239,255):CANVAS;edge=fill;text=selected?ACCENT:MUTED;}
 else if(primary){fill=ACCENT;edge=fill;text=0xffffff;}
 if(hover&&!disabled){fill=primary?RGB(44,83,205):tab?RGB(225,234,255):RGB(245,248,255);edge=tab?fill:RGB(187,204,245);}
 if(pressed&&!disabled){fill=primary?RGB(34,67,174):RGB(220,231,255);edge=fill;}
 if(disabled){fill=RGB(244,246,249);edge=RGB(237,240,244);text=RGB(161,170,185);}
 RECT r=d->rcItem;FillRect(d->hDC,&r,tab?pageBrush:whiteBrush);rounded(d->hDC,r,tab?12:10,fill,edge);
 WCHAR caption[160];GetWindowTextW(d->hwndItem,caption,160);
 if(tab&&selected){RECT bar={r.left+px(24),r.bottom-px(4),r.right-px(24),r.bottom-px(2)};rounded(d->hDC,bar,2,ACCENT,ACCENT);}
 if(pressed){r.top+=px(1);r.bottom+=px(1);}
 label(d->hDC,caption,r,normalFont,text,1|4|32);
 if(d->itemState&16){r.left+=px(4);r.right-=px(4);r.top+=px(4);r.bottom-=px(4);DrawFocusRect(d->hDC,&r);}
}
static void paint_chrome(HDC dc){
 RECT client;GetClientRect(window,&client);FillRect(dc,&client,pageBrush);
 for(int i=0;i<surfaceCount;i++){
  RECT r=surfaces[i],shadow=r;shadow.top+=px(2);shadow.bottom+=px(2);
  rounded(dc,shadow,18,RGB(236,240,246),RGB(236,240,246));rounded(dc,r,18,0xffffff,LINE);
 }
 for(int i=0;i<fieldCount;i++)rounded(dc,fieldFrames[i],10,RGB(248,250,253),LINE);
 if(page!=2){RECT a=surfaces[page==0?1:0];RECT note={a.left+px(20),px(210),a.right-px(20),px(233)};
 label(dc,page==0?L"文本预览 · 复制与保存均为完整内容":L"粘贴纯 Base64 或 Data URI",note,smallFont,MUTED,32);
 if(page==1){a=surfaces[1];note.left=a.left+px(20);note.right=a.right-px(20);label(dc,L"支持保存，也可继续调整尺寸",note,smallFont,MUTED,32);}}
 RECT icon={px(28),px(20),px(70),px(62)};rounded(dc,icon,12,ACCENT,ACCENT);
 RECT inner={px(38),px(30),px(60),px(51)};rounded(dc,inner,4,ACCENT,RGB(225,234,255));
 line(dc,px(39),px(47),px(46),px(39),0xffffff,2);line(dc,px(46),px(39),px(58),px(49),0xffffff,2);
 RECT sub={px(87),px(51),client.right-px(24),px(74)};label(dc,L"图片与编码，一处处理",sub,smallFont,MUTED,32);
 RECT badge={client.right-px(160),px(29),client.right-px(28),px(56)};
 label(dc,L"本地处理  ·  离线使用",badge,smallFont,MUTED,2|4|32);
 line(dc,px(28),client.bottom-px(47),client.right-px(28),client.bottom-px(47),LINE,1);
}
static void surface(int i,int x,int y,int w,int h){surfaces[i]=(RECT){px(x),px(y),px(x+w),px(y+h)};}
