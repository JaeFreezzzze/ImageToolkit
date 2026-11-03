#include "winmini.h"
#include "geometry.h"
#include "base64_codec.h"

enum { WIDTH=101,HEIGHT,LOCK,PRESET,PRESET_ORIGINAL,MODE,BACKGROUND,TRANSPARENT,FORMAT,QUALITY,CONVERT,SAVE,CLEAR };
static const struct { int w,h; const WCHAR *label; } PRESETS[]={
    {640,360,L"640 × 360"},{854,480,L"854 × 480"},{960,540,L"960 × 540"},
    {1280,720,L"1280 × 720"},{1600,900,L"1600 × 900"},{1920,1080,L"1920 × 1080"},
    {2560,1440,L"2560 × 1440"},{3840,2160,L"3840 × 2160"},
    {640,640,L"640 × 640"},{1024,1024,L"1024 × 1024"},{1080,1080,L"1080 × 1080"},
    {720,1280,L"720 × 1280"},{1080,1920,L"1080 × 1920"}
};
#define PRESET_COUNT ((int)(sizeof(PRESETS)/sizeof(PRESETS[0])))
static const WCHAR TITLE[]=L"图片工具箱";
static const WCHAR OPEN_FILTER[]=L"图片文件 (PNG / JPG / BMP / TIFF / GIF / ICO)\0*.png;*.jpg;*.jpeg;*.jfif;*.bmp;*.tif;*.tiff;*.gif;*.ico\0所有文件\0*.*\0\0";
static const WCHAR *MIMES[]={L"image/png",L"image/jpeg",L"image/bmp",L"image/tiff",L"image/gif"};
static const WCHAR *EXTENSIONS[]={L"png",L"jpg",L"bmp",L"tiff",L"gif"};
static const WCHAR *SAVE_FILTERS[]={L"PNG 图片\0*.png\0\0",L"JPG 图片\0*.jpg;*.jpeg\0\0",L"BMP 图片\0*.bmp\0\0",L"TIFF 图片\0*.tiff;*.tif\0\0",L"GIF 静态图片\0*.gif\0\0"};
static const GUID QUALITY_GUID={0x1d5be4b5,0xfa4a,0x452d,{0x9c,0xdd,0x5d,0xb3,0x51,0x05,0xe7,0xeb}};
static HINSTANCE instance;
static HWND window,srcView,outView,statusText,fileText,srcText,outText,heading,settingsTitle;
static HWND fields[32],labels[8];
static HFONT normalFont,titleFont,boldFont;
static HBRUSH pageBrush,whiteBrush,checkerBrush,borderBrush;
static HCURSOR arrowCursor,handCursor,waitCursor;
static ULONG_PTR gdipToken;
static int dpi=96,busy=0,syncing=0,srcW=0,srcH=0,resultW=0,resultH=0,resultFmt=0;
static COLORREF backgroundColor=0xffffff;
static GpImage *sourceImage,*sourceSmall,*outputSmall;
static WCHAR sourcePath[260],resultPath[260];
static int page=0,b64Format=0,decodedFormat=0;
static HWND tabs[3],b64Controls[12],b64Hint;
static char *b64Text;
static size_t b64Length;
static GpImage *decodedSmall;
static WCHAR decodedPath[260];

typedef struct {
    int kind,error,w,h,stretch,transparent,format,quality,frames;
    ARGB background;
    GpImage *source,*loaded,*preview;
    WCHAR path[260],temporary[260],errorText[240];
    HBITMAP clipboardBitmap;
    int clipboard,inputTemporary,deleteTemporary;
    char *text;size_t textSize;
} Job;

static void choose_image(void);
static void paste_image(void);
static void clear_image(void);
static void invalidate_result(void);
static void set_enabled(void);
static void resize_layout(void);
static int start_job(Job *job);

static void textcopy(WCHAR *dest,size_t cap,const WCHAR *src)
{
    if (!cap) return;
    size_t i=0;
    while(src && src[i] && i+1<cap) {dest[i]=src[i];i++;}
    dest[i]=0;
}
static const WCHAR *filename(const WCHAR *path)
{
    const WCHAR *name=path;
    for(const WCHAR *p=path;*p;p++) if(*p==L'\\'||*p==L'/')name=p+1;
    return name;
}
static const WCHAR *extension(const WCHAR *path)
{
    const WCHAR *last=0;
    for(const WCHAR *p=filename(path);*p;p++) if(*p==L'.')last=p+1;
    return last?last:L"";
}
static int px(int n) {return (n*dpi+48)/96;}
static void status(const WCHAR *message) { SetWindowTextW(statusText,message); }
static void error_box(const WCHAR *message) { MessageBoxW(window,message,TITLE,0x10); }
static void dispose_image(GpImage **image) {if(*image){GdipDisposeImage(*image);*image=0;}}
static int checked(int id) {return (int)SendMessageW(fields[id-WIDTH],BM_GETCHECK,0,0)==1;}
static int selection(int id) {return (int)SendMessageW(fields[id-WIDTH],CB_GETCURSEL,0,0);}
static int number(int id)
{
    WCHAR value[24];GetWindowTextW(fields[id-WIDTH],value,24);
    int result=0;
    if(!value[0])return 0;
    for(int i=0;value[i];i++){if(value[i]<L'0'||value[i]>L'9'||result>16384)return 0;result=result*10+(value[i]-L'0');}
    return result;
}
static void set_number(int id,int value)
{
    WCHAR buffer[24];_snwprintf(buffer,24,L"%d",value);buffer[23]=0;SetWindowTextW(fields[id-WIDTH],buffer);
}
static ARGB argb(COLORREF color) {return 0xff000000u|((color&255)<<16)|(color&0xff00)|((color>>16)&255);}
#include "theme.h"

static void job_error(Job *job,int code,const WCHAR *message)
{
    job->error=code?code:1;
    _snwprintf(job->errorText,240,L"%ls（错误码 %d）",message,code);job->errorText[239]=0;
}
static int temp_file(WCHAR *path)
{
    WCHAR directory[260];DWORD n=GetTempPathW(260,directory);
    return n && n<260 && GetTempFileNameW(directory,L"irt",0,path)!=0;
}
static int draw_scaled(GpImage *src,int sw,int sh,int width,int height,int stretch,ARGB bg,GpImage **output)
{
    *output=0;
    GpGraphics *g=0;GpImageAttributes *attrs=0;
    int result=GdipCreateBitmapFromScan0(width,height,0,PIXEL_ARGB,0,(GpBitmap**)output);
    if(result)return result;
    result=GdipGetImageGraphicsContext(*output,&g);
    if(!result)result=GdipGraphicsClear(g,bg);
    if(!result)result=GdipSetInterpolationMode(g,7);
    if(!result)result=GdipSetPixelOffsetMode(g,4);
    if(!result)result=GdipSetCompositingQuality(g,2);
    if(!result)result=GdipCreateImageAttributes(&attrs);
    if(!result)result=GdipSetImageAttributesWrapMode(attrs,3,0,0);
    ImageRect r=fit_image(sw,sh,width,height,stretch);
    if(!result)result=GdipDrawImageRectRectI(g,src,r.x,r.y,r.w,r.h,0,0,sw,sh,2,attrs,0,0);
    if(attrs)GdipDisposeImageAttributes(attrs);
    if(g)GdipDeleteGraphics(g);
    if(result)dispose_image(output);
    return result;
}
static int make_small(GpImage *src,int sw,int sh,GpImage **output)
{
    ImageRect r=fit_image(sw,sh,1000,900,0);
    if(sw<=1000 && sh<=900){r.w=sw;r.h=sh;}
    return draw_scaled(src,sw,sh,r.w,r.h,1,0,output);
}
static void orient_image(GpImage *image)
{
    UINT bytes=0;
    if(GdipGetPropertyItemSize(image,0x112,&bytes)||bytes<sizeof(PropertyItem)||bytes>65536)return;
    PropertyItem *item=malloc(bytes);if(!item)return;
    if(!GdipGetPropertyItem(image,0x112,bytes,item)&&item->length>=2){
        WORD value=*((WORD*)item->value);
        static const int turns[]={0,0,4,2,6,5,1,7,3};
        if(value>=2 && value<=8)GdipImageRotateFlip(image,turns[value]);
    }
    free(item);
}
static int frame_count(GpImage *image)
{
    UINT count=0,biggest=1;
    if(GdipImageGetFrameDimensionsCount(image,&count)||count==0||count>16)return 1;
    GUID ids[16];
    if(GdipImageGetFrameDimensionsList(image,ids,count))return 1;
    for(UINT i=0;i<count;i++){UINT n=1;if(!GdipImageGetFrameCount(image,&ids[i],&n)&&n>biggest)biggest=n;}
    return (int)biggest;
}
static int encode_to(GpImage *image,const WCHAR *path,int format,int quality)
{
    UINT count=0,bytes=0;
    int result=GdipGetImageEncodersSize(&count,&bytes);
    if(result||!bytes)return result?result:13;
    ImageCodecInfo *codecs=malloc(bytes);if(!codecs)return 3;
    result=GdipGetImageEncoders(count,bytes,codecs);
    GUID clsid;int found=0;
    if(!result)for(UINT i=0;i<count;i++)if(!wcscmp(codecs[i].MimeType,MIMES[format])){clsid=codecs[i].Clsid;found=1;break;}
    free(codecs);if(!found)return result?result:13;
    EncoderParameters params={0};ULONG q=(ULONG)quality;
    params.Count=1;params.Parameter[0].Guid=QUALITY_GUID;params.Parameter[0].NumberOfValues=1;params.Parameter[0].Type=4;params.Parameter[0].Value=&q;
    return GdipSaveImageToFile(image,path,&clsid,format==1?&params:0);
}
static void do_load(Job *job)
{
    WIN32_FILE_ATTRIBUTE_DATA info;
    if(!job->clipboardBitmap){
        if(!GetFileAttributesExW(job->path,0,&info)){job_error(job,(int)GetLastError(),L"无法读取图片文件");return;}
        if(info.sizeHigh||info.sizeLow>100u*1024*1024){job_error(job,0,L"单个图片文件最多支持 100 MB");return;}
    }
    GpImage *original=0;
    int result;
    if(job->clipboardBitmap)result=GdipCreateBitmapFromHBITMAP(job->clipboardBitmap,0,&original);
    else if(!_wcsicmp(extension(job->path),L"ico")){
        HICON icon=LoadImageW(0,job->path,1,256,256,0x10);
        if(!icon){job_error(job,(int)GetLastError(),L"无法读取 ICO 图标");return;}
        result=GdipCreateBitmapFromHICON(icon,(GpBitmap**)&original);DestroyIcon(icon);
    }else result=GdipLoadImageFromFileICM(job->path,&original);
    if(result){job_error(job,result,L"无法解码图片，请确认文件完整且格式受支持");return;}
    orient_image(original);
    UINT w=0,h=0;GdipGetImageWidth(original,&w);GdipGetImageHeight(original,&h);
    job->w=(int)w;job->h=(int)h;job->frames=frame_count(original);
    if(!w||!h||(uint64_t)w*h>40000000){job_error(job,0,L"原图总像素不能超过 4000 万");dispose_image(&original);return;}
    result=draw_scaled(original,w,h,w,h,1,0,&job->loaded);
    dispose_image(&original);
    if(!result)result=make_small(job->loaded,w,h,&job->preview);
    if(result){dispose_image(&job->loaded);dispose_image(&job->preview);job_error(job,result,L"无法生成图片预览");}
}
static void do_convert(Job *job)
{
    UINT w=0,h=0;GdipGetImageWidth(job->source,&w);GdipGetImageHeight(job->source,&h);
    GpImage *output=0,*encoded=0;
    int result=draw_scaled(job->source,w,h,job->w,job->h,job->stretch,job->transparent?0:job->background,&output);
    if(!result&&!temp_file(job->temporary))result=7;
    if(!result)result=encode_to(output,job->temporary,job->format,job->quality);
    dispose_image(&output);
    if(!result)result=GdipLoadImageFromFileICM(job->temporary,&encoded);
    if(!result)result=make_small(encoded,job->w,job->h,&job->preview);
    dispose_image(&encoded);
    if(result){if(job->temporary[0]){DeleteFileW(job->temporary);job->temporary[0]=0;}job_error(job,result,L"图片转换失败，请尝试减小尺寸或改用 PNG");}
}
static void do_save(Job *job)
{
    WCHAR directory[260],temp[260];textcopy(directory,260,job->path);
    size_t n=wcslen(directory);while(n && directory[n-1]!=L'\\' && directory[n-1]!=L'/')n--;
    if(!n){job_error(job,0,L"保存路径无效");return;}directory[n]=0;
    if(!GetTempFileNameW(directory,L"irt",0,temp)){job_error(job,(int)GetLastError(),L"无法写入所选目录");return;}
    int success=CopyFileW(job->temporary,temp,0);
    if(success)success=MoveFileExW(temp,job->path,1|8);
    if(!success){DWORD code=GetLastError();DeleteFileW(temp);job_error(job,(int)code,L"保存失败，请检查文件权限和可用空间");}
}
#include "base64_ui.h"

static DWORD worker(void *arg)
{
    Job *job=arg;
    if(job->kind==1)do_load(job);
    else if(job->kind==2)do_convert(job);
    else if(job->kind==4||job->kind==5)do_base64(job);
    else do_save(job);
    if(job->clipboardBitmap){DeleteObject(job->clipboardBitmap);job->clipboardBitmap=0;}
    if(job->deleteTemporary){DeleteFileW(job->temporary);job->temporary[0]=0;}
    if(job->inputTemporary){DeleteFileW(job->path);job->inputTemporary=0;}
    PostMessageW(window,WM_APP_DONE,0,(LPARAM)job);
    return 0;
}
static int start_job(Job *job)
{
    if(!job)return 0;
    busy=1;set_enabled();SetCursor(waitCursor);
    HANDLE thread=CreateThread(0,0,worker,job,0,0);
    if(!thread){busy=0;set_enabled();if(job->clipboardBitmap)DeleteObject(job->clipboardBitmap);if(job->inputTemporary)DeleteFileW(job->path);if(job->deleteTemporary)DeleteFileW(job->temporary);free(job->text);free(job);error_box(L"无法启动处理线程，请关闭部分程序后重试。");return 0;}
    CloseHandle(thread);return 1;
}
static void sync_preset(void)
{
    int w=number(WIDTH),h=number(HEIGHT),index=0;
    for(int i=0;i<PRESET_COUNT;i++)if(PRESETS[i].w==w&&PRESETS[i].h==h){index=i+1;break;}
    SendMessageW(fields[PRESET-WIDTH],CB_SETCURSEL,index,0);
}
static void invalidate_result(void)
{
    sync_preset();
    dispose_image(&outputSmall);
    if(resultPath[0]){DeleteFileW(resultPath);resultPath[0]=0;}
    resultW=resultH=0;
    SetWindowTextW(outText,L"转换结果");InvalidateRect(outView,0,0);
    if(sourceImage)status(L"设置已更改，请点击「转换并预览」。");
    set_enabled();
}
static void set_enabled(void)
{
    int ready=sourceImage!=0&&!busy;
    for(int i=0;i<3;i++)EnableWindow(tabs[i],!busy);
    for(int i=0;i<12;i++)if(b64Controls[i])EnableWindow(b64Controls[i],!busy);
    EnableWindow(b64Controls[2],ready);
    EnableWindow(b64Controls[3],!busy&&b64Text);EnableWindow(b64Controls[4],!busy&&b64Text);
    EnableWindow(b64Controls[9],!busy&&decodedPath[0]);EnableWindow(b64Controls[11],!busy&&decodedPath[0]);
    for(int id=WIDTH;id<=CLEAR;id++)if(fields[id-WIDTH])EnableWindow(fields[id-WIDTH],ready);
    EnableWindow(fields[SAVE-WIDTH],ready&&resultPath[0]);
    int fmt=selection(FORMAT);
    EnableWindow(fields[QUALITY-WIDTH],ready&&fmt==1);
    EnableWindow(fields[TRANSPARENT-WIDTH],ready&&(fmt==0||fmt==3));
    EnableWindow(fields[BACKGROUND-WIDTH],ready&&!checked(TRANSPARENT));
}
static void dimensions_changed(int id)
{
    if(syncing)return;
    int val=number(id);
    if(checked(LOCK)&&sourceImage&&val>0&&val<=16384){
        int other=id==WIDTH?HEIGHT:WIDTH;
        int a=id==WIDTH?srcW:srcH,b=id==WIDTH?srcH:srcW;
        int related=(int)(((int64_t)val*b+a/2)/a);if(related<1)related=1;
        if(related>16384){syncing=1;set_number(other,16384);set_number(id,(int)(((int64_t)16384*a+b/2)/b));syncing=0;}
        else{syncing=1;set_number(other,related);syncing=0;}
    }
    invalidate_result();
}
static void choose_image(void)
{
    if(busy)return;
    WCHAR path[260]={0};OPENFILENAMEW dialog={0};
    dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;dialog.lpstrFilter=OPEN_FILTER;
    dialog.lpstrFile=path;dialog.nMaxFile=260;dialog.lpstrTitle=L"选择图片";dialog.Flags=0x80000|0x1000|0x800|8;
    if(GetOpenFileNameW(&dialog)){
        Job *job=calloc(1,sizeof(Job));if(!job){error_box(L"可用内存不足。");return;}
        job->kind=1;textcopy(job->path,260,path);status(L"正在读取图片…");start_job(job);
    }else if(CommDlgExtendedError())error_box(L"无法打开文件选择窗口。");
}
static void paste_image(void)
{
    if(busy)return;
    if(!OpenClipboard(window)){error_box(L"剪贴板正被其他程序占用，请稍后重试。");return;}
    Job *job=calloc(1,sizeof(Job));
    if(!job){CloseClipboard();error_box(L"可用内存不足。");return;}
    job->kind=1;int ready=0;
    UINT png=RegisterClipboardFormatW(L"PNG");HANDLE data=png?GetClipboardData(png):0;
    if(data){
        size_t size=GlobalSize(data);BYTE *bytes=GlobalLock(data);
        if(bytes&&size>=8&&size<=100u*1024*1024&&bytes[0]==137&&bytes[1]==80&&bytes[2]==78&&bytes[3]==71&&temp_file(job->path)){
            HANDLE file=CreateFileW(job->path,0x40000000,0,0,2,0x80,0);DWORD written=0;
            if(file!=(HANDLE)(intptr_t)-1){ready=WriteFile(file,bytes,(DWORD)size,&written,0)&&written==size;CloseHandle(file);}
            if(ready){job->clipboard=1;job->inputTemporary=1;}else{DeleteFileW(job->path);job->path[0]=0;}
        }
        if(bytes)GlobalUnlock(data);
    }
    if(!ready){
        HBITMAP bitmap=GetClipboardData(2);WINBITMAP info={0};
        if(bitmap&&GetObjectW(bitmap,sizeof(info),&info)&&info.width>0&&info.height>0&&(uint64_t)info.width*info.height<=40000000){
            job->clipboardBitmap=CopyImage(bitmap,0,0,0,0x2000);
            if(job->clipboardBitmap){job->clipboard=1;ready=1;}
        }
    }
    if(!ready){HANDLE files=GetClipboardData(15);if(files){UINT length=DragQueryFileW(files,0,0,0);if(length>0&&length<260)ready=DragQueryFileW(files,0,job->path,260)>0;}}
    CloseClipboard();
    if(!ready){free(job);error_box(L"剪贴板中没有可读取的图片，或图片超过大小限制。请先复制图片、截图或图片文件；复制图片链接不能直接粘贴。");return;}
    status(L"正在读取剪贴板图片…");start_job(job);
}
static void convert_image(void)
{
    if(!sourceImage||busy)return;
    int w=number(WIDTH),h=number(HEIGHT),quality=number(QUALITY);
    if(!valid_dimensions(w,h)){error_box(L"宽高应为 1–16384 的整数，总像素不能超过 4000 万。");return;}
    if(selection(FORMAT)==1&&(quality<1||quality>100)){error_box(L"JPG 质量应为 1–100 的整数。");return;}
    Job *job=calloc(1,sizeof(Job));if(!job){error_box(L"可用内存不足。");return;}
    job->kind=2;job->source=sourceImage;job->w=w;job->h=h;job->format=selection(FORMAT);job->quality=quality;
    job->stretch=selection(MODE)==1;job->transparent=(job->format==0||job->format==3)&&checked(TRANSPARENT);job->background=argb(backgroundColor);
    invalidate_result();status(L"正在转换图片…");start_job(job);
}
static void save_image(void)
{
    if(busy||!resultPath[0])return;
    WCHAR path[260]={0},base[130],directory[260],fullpath[260],message[600];
    textcopy(base,130,sourcePath[0]?filename(sourcePath):L"剪贴板图片");
    for(size_t i=wcslen(base);i>0;i--)if(base[i-1]==L'.'){base[i-1]=0;break;}
    _snwprintf(path,260,L"%ls_%dx%d.%ls",base,resultW,resultH,EXTENSIONS[resultFmt]);path[259]=0;
    textcopy(directory,260,sourcePath);size_t n=wcslen(directory);while(n&&directory[n-1]!=L'\\')n--;directory[n]=0;
    OPENFILENAMEW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;dialog.lpstrFilter=SAVE_FILTERS[resultFmt];dialog.lpstrDefExt=EXTENSIONS[resultFmt];
    dialog.lpstrFile=path;dialog.nMaxFile=260;dialog.lpstrTitle=L"保存转换后的图片";dialog.lpstrInitialDir=directory;dialog.Flags=0x80000|0x800|2|8;
    if(!GetSaveFileNameW(&dialog)){if(CommDlgExtendedError())error_box(L"无法打开保存窗口。");return;}
    DWORD length=GetFullPathNameW(path,260,fullpath,0);if(!length||length>=260){error_box(L"保存路径过长或无效。");return;}
    if(!_wcsicmp(fullpath,sourcePath)){error_box(L"请使用新的文件名保存，以保留原始图片。");return;}
    const WCHAR *ext=extension(path);
    if(_wcsicmp(ext,EXTENSIONS[resultFmt])&&!(resultFmt==1&&!_wcsicmp(ext,L"jpeg"))&&!(resultFmt==3&&!_wcsicmp(ext,L"tif"))){
        _snwprintf(message,600,L"扩展名与输出格式不一致，请使用 .%ls，或重新选择输出格式后转换。",EXTENSIONS[resultFmt]);message[599]=0;error_box(message);return;
    }
    Job *job=calloc(1,sizeof(Job));if(!job){error_box(L"可用内存不足。");return;}
    job->kind=3;textcopy(job->path,260,fullpath);textcopy(job->temporary,260,resultPath);status(L"正在保存图片…");start_job(job);
}
static void clear_image(void)
{
    if(busy)return;
    free(b64Text);b64Text=0;b64Length=0;b64_output_preview();
    dispose_image(&sourceImage);dispose_image(&sourceSmall);sourcePath[0]=0;srcW=srcH=0;invalidate_result();
    syncing=1;set_number(WIDTH,1280);set_number(HEIGHT,720);set_number(QUALITY,92);SendMessageW(fields[LOCK-WIDTH],BM_SETCHECK,1,0);
    SendMessageW(fields[MODE-WIDTH],CB_SETCURSEL,0,0);SendMessageW(fields[FORMAT-WIDTH],CB_SETCURSEL,0,0);SendMessageW(fields[TRANSPARENT-WIDTH],BM_SETCHECK,0,0);
    backgroundColor=0xffffff;SetWindowTextW(fields[BACKGROUND-WIDTH],L"背景：白色");syncing=0;sync_preset();
    SetWindowTextW(srcText,L"原始图片");SetWindowTextW(fileText,L"PNG / JPG / BMP / TIFF / GIF / ICO");InvalidateRect(srcView,0,0);
    set_enabled();status(L"已清空。点击左侧图片区域选择新图片。");
}
static void completed(Job *job)
{
    busy=0;SetCursor(arrowCursor);
    if(!job->error&&job->kind==4){
        free(b64Text);b64Text=job->text;job->text=0;b64Length=job->textSize;b64Format=job->format;b64_output_preview();
        status(b64Length>16000?L"转换完成。文本框仅预览前 16000 字符，复制与保存包含完整结果。":L"Base64 转换完成，可复制完整结果或保存为 TXT。");
    }else if(!job->error&&job->kind==5){
        dispose_image(&decodedSmall);if(decodedPath[0])DeleteFileW(decodedPath);textcopy(decodedPath,260,job->temporary);job->temporary[0]=0;
        decodedSmall=job->preview;job->preview=0;decodedFormat=job->format;
        WCHAR label[100];_snwprintf(label,100,L"解码结果  %d × %d",job->w,job->h);SetWindowTextW(outText,label);
        status(L"解码完成，可保存原始图片，或继续调整分辨率。");InvalidateRect(outView,0,0);
    }
    if(job->error){status(job->errorText);error_box(job->errorText);}
    else if(job->kind==1){
        free(b64Text);b64Text=0;b64Length=0;b64_output_preview();
        dispose_image(&sourceImage);dispose_image(&sourceSmall);invalidate_result();
        sourceImage=job->loaded;sourceSmall=job->preview;job->loaded=job->preview=0;srcW=job->w;srcH=job->h;
        if(job->clipboard)sourcePath[0]=0;else GetFullPathNameW(job->path,260,sourcePath,0);
        int w=srcW,h=srcH;if(w>1280||h>1280){ImageRect r=fit_image(srcW,srcH,1280,1280,0);w=r.w;h=r.h;}
        syncing=1;set_number(WIDTH,w);set_number(HEIGHT,h);SendMessageW(fields[LOCK-WIDTH],BM_SETCHECK,1,0);syncing=0;sync_preset();
        WCHAR label[420];_snwprintf(label,420,L"%ls%ls",job->clipboard?L"剪贴板图片":filename(sourcePath),job->frames>1?L"  ·  多帧图片，仅处理第一帧":L"");label[419]=0;SetWindowTextW(fileText,label);
        _snwprintf(label,420,L"原图  %d × %d",srcW,srcH);SetWindowTextW(srcText,label);InvalidateRect(srcView,0,0);
        status(job->frames>1?L"已加载第一帧，转换后保存为静态图片。":L"图片已加载。可自定义分辨率，或点击左框重新选图。");
    }else if(job->kind==2){
        outputSmall=job->preview;job->preview=0;textcopy(resultPath,260,job->temporary);job->temporary[0]=0;
        resultW=job->w;resultH=job->h;resultFmt=job->format;
        WCHAR label[200];_snwprintf(label,200,L"结果  %d × %d",resultW,resultH);SetWindowTextW(outText,label);
        ImageRect r=fit_image(srcW,srcH,resultW,resultH,job->stretch);
        _snwprintf(label,200,L"转换完成：%d × %d 像素；画面 %d × %d。点击「另存为」保存。",resultW,resultH,r.w,r.h);label[199]=0;status(label);InvalidateRect(outView,0,0);
    }else if(job->kind==3){
        WCHAR label[600];_snwprintf(label,600,L"保存成功：%ls",job->path);label[599]=0;status(label);MessageBoxW(window,label,TITLE,0x40);
    }
    if(job->temporary[0]&&(job->kind==5||job->deleteTemporary))DeleteFileW(job->temporary);
    free(job->text);dispose_image(&job->loaded);dispose_image(&job->preview);free(job);set_enabled();
}
static void paint_preview(HWND view,HDC dc,int width,int height)
{
    RECT all={0,0,width,height};FillRect(dc,&all,whiteBrush);
    GpImage *image=view==srcView?sourceSmall:page==1?decodedSmall:outputSmall;
    int step=px(16);
    if(image)
    for(int y=0;y<height;y+=step)for(int x=0;x<width;x+=step)if(((x/step+y/step)&1)==0){RECT tile={x,y,x+step,y+step};FillRect(dc,&tile,checkerBrush);}
    if(image){
        UINT w=0,h=0;GdipGetImageWidth(image,&w);GdipGetImageHeight(image,&h);
        ImageRect r=fit_image(w,h,width-px(16),height-px(16),0);r.x+=px(8);r.y+=px(8);
        GpGraphics *g=0;if(!GdipCreateFromHDC(dc,&g)){GdipSetInterpolationMode(g,7);GdipDrawImageRectRectI(g,image,r.x,r.y,r.w,r.h,0,0,w,h,2,0,0,0);GdipDeleteGraphics(g);}
    }else{
        RECT panel={0,0,width,height};rounded(dc,panel,14,RGB(248,250,253),LINE);
        int cx=width/2,cy=height/2-px(25);
        RECT tile={cx-px(28),cy-px(32),cx+px(28),cy+px(24)};rounded(dc,tile,14,RGB(232,239,255),RGB(232,239,255));
        RECT pic={cx-px(13),cy-px(17),cx+px(13),cy+px(7)};rounded(dc,pic,4,RGB(232,239,255),ACCENT);
        line(dc,cx-px(12),cy+px(3),cx-px(4),cy-px(6),ACCENT,2);line(dc,cx-px(4),cy-px(6),cx+px(12),cy+px(4),ACCENT,2);
        RECT title={px(12),cy+px(43),width-px(12),cy+px(70)};
        label(dc,view==srcView?L"点击添加图片":L"等待转换结果",title,normalFont,INK,1|4|32);
        RECT hint={px(8),cy+px(77),width-px(8),cy+px(97)};
        label(dc,view==srcView?L"拖入文件 / Ctrl+V 粘贴":L"完成转换后在此预览",hint,smallFont,MUTED,1|4|32);
    }
    if(image)FrameRect(dc,&all,borderBrush);
}
static LRESULT preview_proc(HWND view,UINT msg,WPARAM wp,LPARAM lp)
{
    if(msg==WM_PAINT){
        PAINTSTRUCT paint;HDC screen=BeginPaint(view,&paint);RECT area;GetClientRect(view,&area);
        if(area.right>0&&area.bottom>0){HDC buffer=CreateCompatibleDC(screen);HBITMAP bmp=CreateCompatibleBitmap(screen,area.right,area.bottom);
            if(buffer&&bmp){HGDIOBJ old=SelectObject(buffer,bmp);paint_preview(view,buffer,area.right,area.bottom);BitBlt(screen,0,0,area.right,area.bottom,buffer,0,0,0x00cc0020);SelectObject(buffer,old);}
            else paint_preview(view,screen,area.right,area.bottom);
            if(bmp)DeleteObject(bmp);
            if(buffer)DeleteDC(buffer);
        }
        EndPaint(view,&paint);return 0;
    }
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==0x302&&view==srcView){paste_image();return 0;}
    if(msg==0x7b&&view==srcView&&!busy){
        HMENU menu=CreatePopupMenu();
        if(menu){POINT point={0};GetCursorPos(&point);AppendMenuW(menu,0,1,L"粘贴图片\tCtrl+V");UINT action=TrackPopupMenu(menu,0x100|2,point.x,point.y,0,window,0);DestroyMenu(menu);if(action==1)paste_image();}
        return 0;
    }
    if(msg==WM_LBUTTONUP&&view==srcView&&!busy){SetFocus(view);choose_image();return 0;}
    if(msg==WM_KEYDOWN&&view==srcView&&(wp==13||wp==32)&&!busy){choose_image();return 0;}
    if(msg==WM_SETCURSOR){SetCursor(busy?waitCursor:view==srcView?handCursor:arrowCursor);return 1;}
    return DefWindowProcW(view,msg,wp,lp);
}
static HWND child(const WCHAR *type,const WCHAR *text,DWORD style,int id)
{
    int custom=!wcscmp(type,L"BUTTON")&&id!=LOCK&&id!=TRANSPARENT&&id!=301;
    if(custom)style=(style&~15u)|11;
    if(!wcscmp(type,L"EDIT"))style&=~WS_BORDER;
    HWND control=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,0,0,1,1,window,(HMENU)(uintptr_t)id,instance,0);
    SendMessageW(control,WM_SETFONT,(WPARAM)normalFont,1);if(custom)SetWindowSubclass(control,button_proc,1,0);return control;
}
static void place(HWND control,int x,int y,int w,int h){MoveWindow(control,px(x),px(y),px(w),px(h),1);}
static void field_place(HWND control,int x,int y,int w,int h){
    fieldFrames[fieldCount++]=(RECT){px(x),px(y),px(x+w),px(y+h)};
    place(control,x+9,y+6,w-18,h-12);
}
static void resize_layout(void)
{
    if(!srcView)return;
    RECT rect;GetClientRect(window,&rect);int w=rect.right*96/dpi,h=rect.bottom*96/dpi;
    fieldCount=0;
    place(heading,87,18,230,34);
    const WCHAR *names[]={L"图片转 Base64",L"Base64 转图片",L"分辨率调整"};
    for(int i=0;i<3;i++){place(tabs[i],24+i*178,89,166,40);SetWindowTextW(tabs[i],names[i]);InvalidateRect(tabs[i],0,0);}
    ShowWindow(fileText,page!=1);ShowWindow(srcText,page!=1);ShowWindow(srcView,page!=1);
    ShowWindow(outText,page!=0);ShowWindow(outView,page!=0);ShowWindow(settingsTitle,page==2);
    for(int i=0;i<6;i++)ShowWindow(labels[i],page==2);
    for(int id=WIDTH;id<=CLEAR;id++)ShowWindow(fields[id-WIDTH],page==2);
    for(int i=0;i<12;i++)if(b64Controls[i])ShowWindow(b64Controls[i],i<6?page==0:page==1);
    ShowWindow(b64Hint,page!=2);
    if(page==2){
        int side=w-326,col=(side-56)/2,x=side+18,fw=124;
        surfaceCount=3;surface(0,24,154,col,h-218);surface(1,40+col,154,col,h-218);surface(2,side,154,302,h-218);
        place(fileText,24,132,side-40,20);
        place(srcText,42,174,col-36,26);place(outText,58+col,174,col-36,26);
        place(srcView,40,214,col-32,h-350);place(outView,56+col,214,col-32,h-350);
        place(settingsTitle,x,174,266,26);
        place(labels[0],x,213,124,20);place(labels[1],x+142,213,124,20);
        field_place(fields[WIDTH-WIDTH],x,239,fw,32);field_place(fields[HEIGHT-WIDTH],x+142,239,fw,32);
        place(fields[LOCK-WIDTH],x,280,266,24);
        place(labels[5],x,313,266,20);place(fields[PRESET-WIDTH],x,339,158,260);place(fields[PRESET_ORIGINAL-WIDTH],x+170,338,96,32);
        place(labels[2],x,381,78,22);place(fields[MODE-WIDTH],x+80,377,186,180);
        place(fields[BACKGROUND-WIDTH],x,416,136,32);place(fields[TRANSPARENT-WIDTH],x+146,420,120,25);
        place(labels[3],x,462,78,22);place(fields[FORMAT-WIDTH],x+80,458,186,210);
        place(labels[4],x,502,160,22);field_place(fields[QUALITY-WIDTH],x+176,496,90,32);
        place(fields[CONVERT-WIDTH],x,h-118,266,38);
        place(fields[SAVE-WIDTH],42,h-118,col-36,38);place(fields[CLEAR-WIDTH],58+col,h-118,col-36,38);
    }else if(page==0){
        int left=(w-64)*40/100,right=w-64-left,rx=40+left;
        surfaceCount=2;surface(0,24,154,left,h-218);surface(1,rx,154,right,h-218);
        place(srcText,44,176,left-40,25);place(fileText,44,208,left-40,22);
        place(srcView,42,246,left-36,h-386);
        place(b64Hint,rx+20,176,180,27);place(b64Controls[1],rx+right-204,176,184,27);
        field_place(b64Controls[0],rx+20,246,right-40,h-386);
        place(b64Controls[2],44,h-118,left-150,38);place(b64Controls[5],left-94,h-118,98,38);
        place(b64Controls[3],rx+20,h-118,(right-52)/2,38);place(b64Controls[4],rx+32+(right-52)/2,h-118,(right-52)/2,38);
    }else{
        int left=(w-64)*56/100,right=w-64-left,rx=40+left;
        surfaceCount=2;surface(0,24,154,left,h-218);surface(1,rx,154,right,h-218);
        place(b64Hint,44,176,left-40,27);place(outText,rx+20,176,right-40,27);
        field_place(b64Controls[6],44,246,left-40,h-430);place(outView,rx+18,246,right-36,h-430);
        place(b64Controls[8],44,h-118,left-150,38);place(b64Controls[10],left-94,h-118,98,38);
        place(b64Controls[9],rx+20,h-166,right-40,38);place(b64Controls[11],rx+20,h-118,right-40,38);
    }
    place(statusText,28,h-34,w-56,24);
    SetWindowTextW(b64Hint,page==0?L"Base64 结果":L"Base64 输入");
    SetWindowTextW(outText,page==1?L"图片预览":L"转换结果");
    InvalidateRect(window,0,1);InvalidateRect(srcView,0,0);InvalidateRect(outView,0,0);

}
static void build_ui(void)
{
    heading=child(L"STATIC",TITLE,0,0);SendMessageW(heading,WM_SETFONT,(WPARAM)titleFont,1);
    fileText=child(L"STATIC",L"PNG / JPG / BMP / TIFF / GIF / ICO",0x4000,0);
    srcText=child(L"STATIC",L"原始图片",0,0);outText=child(L"STATIC",L"转换结果",0,0);
    settingsTitle=child(L"STATIC",L"输出设置",0,0);SendMessageW(settingsTitle,WM_SETFONT,(WPARAM)boldFont,1);
    srcView=child(L"IRT_IMAGE",L"点击选择原图",WS_TABSTOP,0);outView=child(L"IRT_IMAGE",L"转换后的图片",0,0);
    labels[0]=child(L"STATIC",L"宽度 / 像素",0,0);labels[1]=child(L"STATIC",L"高度 / 像素",0,0);
    labels[2]=child(L"STATIC",L"画面适配",0,0);labels[3]=child(L"STATIC",L"输出格式",0,0);labels[4]=child(L"STATIC",L"JPG 质量  1–100",0,0);
    syncing=1;
    fields[WIDTH-WIDTH]=child(L"EDIT",L"1280",WS_TABSTOP|WS_BORDER|0x2000|0x80,WIDTH);
    fields[HEIGHT-WIDTH]=child(L"EDIT",L"720",WS_TABSTOP|WS_BORDER|0x2000|0x80,HEIGHT);
    fields[LOCK-WIDTH]=child(L"BUTTON",L"锁定原图宽高比例",WS_TABSTOP|3,LOCK);SendMessageW(fields[LOCK-WIDTH],BM_SETCHECK,1,0);
    labels[5]=child(L"STATIC",L"预设分辨率",0,0);
    fields[PRESET-WIDTH]=child(L"COMBOBOX",L"",WS_TABSTOP|3|0x200000,PRESET);
    SendMessageW(fields[PRESET-WIDTH],CB_ADDSTRING,0,(LPARAM)L"自定义");
    for(int i=0;i<PRESET_COUNT;i++)SendMessageW(fields[PRESET-WIDTH],CB_ADDSTRING,0,(LPARAM)PRESETS[i].label);
    sync_preset();
    fields[PRESET_ORIGINAL-WIDTH]=child(L"BUTTON",L"原图尺寸",WS_TABSTOP,PRESET_ORIGINAL);
    fields[MODE-WIDTH]=child(L"COMBOBOX",L"",WS_TABSTOP|3|0x200000,MODE);
    SendMessageW(fields[MODE-WIDTH],CB_ADDSTRING,0,(LPARAM)L"等比例补边");SendMessageW(fields[MODE-WIDTH],CB_ADDSTRING,0,(LPARAM)L"拉伸铺满");SendMessageW(fields[MODE-WIDTH],CB_SETCURSEL,0,0);
    fields[BACKGROUND-WIDTH]=child(L"BUTTON",L"背景：白色",WS_TABSTOP,BACKGROUND);fields[TRANSPARENT-WIDTH]=child(L"BUTTON",L"透明背景",WS_TABSTOP|3,TRANSPARENT);
    fields[FORMAT-WIDTH]=child(L"COMBOBOX",L"",WS_TABSTOP|3|0x200000,FORMAT);
    const WCHAR *formats[]={L"PNG · 透明",L"JPG / JPEG",L"BMP",L"TIFF · 透明",L"GIF · 静态"};
    for(int i=0;i<5;i++)SendMessageW(fields[FORMAT-WIDTH],CB_ADDSTRING,0,(LPARAM)formats[i]);
    SendMessageW(fields[FORMAT-WIDTH],CB_SETCURSEL,0,0);
    fields[QUALITY-WIDTH]=child(L"EDIT",L"92",WS_TABSTOP|WS_BORDER|0x2000|0x80,QUALITY);
    fields[CONVERT-WIDTH]=child(L"BUTTON",L"转换并预览",WS_TABSTOP|1,CONVERT);
    fields[SAVE-WIDTH]=child(L"BUTTON",L"另存为图片…",WS_TABSTOP,SAVE);fields[CLEAR-WIDTH]=child(L"BUTTON",L"清空",WS_TABSTOP,CLEAR);
    statusText=child(L"STATIC",L"点击左框选择图片，或按 Ctrl+V / 右键粘贴图片。",0x4000,0);
    for(int i=0;i<3;i++)tabs[i]=child(L"BUTTON",L"",WS_TABSTOP|0x1000|9,200+i);
    b64Hint=child(L"STATIC",L"",0,0);
    b64Controls[0]=child(L"EDIT",L"",WS_TABSTOP|WS_BORDER|0x4|0x80|0x800|0x200000|0x100000,300);
    SendMessageW(b64Controls[0],0xc5,TEXT_LIMIT,0);
    b64Controls[1]=child(L"BUTTON",L"添加 Data URI 前缀",WS_TABSTOP|3,301);
    b64Controls[2]=child(L"BUTTON",L"转换为 Base64",WS_TABSTOP,302);
    b64Controls[3]=child(L"BUTTON",L"复制完整结果",WS_TABSTOP,303);
    b64Controls[4]=child(L"BUTTON",L"保存为 TXT",WS_TABSTOP,304);
    b64Controls[5]=child(L"BUTTON",L"清空",WS_TABSTOP,305);
    b64Controls[6]=child(L"EDIT",L"",WS_TABSTOP|WS_BORDER|0x4|0x40|0x80|0x200000|0x100000|0x1000,306);
    SendMessageW(b64Controls[6],0xc5,TEXT_LIMIT,0);
    b64Controls[8]=child(L"BUTTON",L"解码并预览",WS_TABSTOP,308);
    b64Controls[9]=child(L"BUTTON",L"保存图片…",WS_TABSTOP,309);
    b64Controls[10]=child(L"BUTTON",L"清空",WS_TABSTOP,310);
    b64Controls[11]=child(L"BUTTON",L"继续调整分辨率",WS_TABSTOP,311);
    SendMessageW(srcText,WM_SETFONT,(WPARAM)boldFont,1);SendMessageW(outText,WM_SETFONT,(WPARAM)boldFont,1);
    SendMessageW(b64Hint,WM_SETFONT,(WPARAM)boldFont,1);SendMessageW(fileText,WM_SETFONT,(WPARAM)smallFont,1);SendMessageW(statusText,WM_SETFONT,(WPARAM)smallFont,1);
    SendMessageW(b64Controls[0],WM_SETFONT,(WPARAM)monoFont,1);SendMessageW(b64Controls[6],WM_SETFONT,(WPARAM)monoFont,1);
    syncing=0;set_enabled();resize_layout();
}
static void on_command(int id,int event)
{
    if(syncing||busy)return;
    if(id>=200&&id<=202&&!event){page=id-200;resize_layout();return;}
    if(id>=300&&id<=311){
        if(id==306&&event==EN_CHANGE){dispose_image(&decodedSmall);if(decodedPath[0])DeleteFileW(decodedPath);decodedPath[0]=0;InvalidateRect(outView,0,0);}
        else if(!event)b64_command(id);
        set_enabled();return;
    }
    if((id==WIDTH||id==HEIGHT)&&event==EN_CHANGE){dimensions_changed(id);return;}
    if(id==QUALITY&&event==EN_CHANGE){invalidate_result();return;}
    if((id==MODE||id==FORMAT)&&event==1){
        if(id==FORMAT){int fmt=selection(FORMAT);if(fmt!=0&&fmt!=3)SendMessageW(fields[TRANSPARENT-WIDTH],BM_SETCHECK,0,0);}
        invalidate_result();return;
    }
    if(sourceImage&&((id==PRESET&&event==1)||(id==PRESET_ORIGINAL&&event==0))){
        int w=srcW,h=srcH;
        if(id==PRESET){
            int index=selection(PRESET)-1;
            if(index<0||index>=PRESET_COUNT)return;
            w=PRESETS[index].w;h=PRESETS[index].h;
        }
        if(!valid_dimensions(w,h)){error_box(L"原图超过输出尺寸上限，请输入较小尺寸。");return;}
        syncing=1;set_number(WIDTH,w);set_number(HEIGHT,h);SendMessageW(fields[LOCK-WIDTH],BM_SETCHECK,(int64_t)w*srcH==(int64_t)h*srcW,0);syncing=0;invalidate_result();return;
    }
    if(event)return;
    if(id==LOCK){dimensions_changed(WIDTH);return;}
    if(id==CONVERT){convert_image();return;}if(id==SAVE){save_image();return;}if(id==CLEAR){clear_image();return;}
    if(id==BACKGROUND){
        static COLORREF custom[16];CHOOSECOLORW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;dialog.rgbResult=backgroundColor;dialog.lpCustColors=custom;dialog.Flags=1|2;
        if(ChooseColorW(&dialog)){backgroundColor=dialog.rgbResult;WCHAR text[40];_snwprintf(text,40,L"背景：#%02X%02X%02X",backgroundColor&255,(backgroundColor>>8)&255,(backgroundColor>>16)&255);SetWindowTextW(fields[BACKGROUND-WIDTH],text);invalidate_result();}return;
    }
    if(id==TRANSPARENT)invalidate_result();
}
static LRESULT main_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    switch(msg){
    case WM_CREATE:window=hwnd;build_ui();DragAcceptFiles(hwnd,1);return 0;
    case WM_SIZE:resize_layout();return 0;
    case WM_GETMINMAXINFO:{MINMAXINFO *info=(MINMAXINFO*)lp;info->ptMinTrackSize.x=px(980);info->ptMinTrackSize.y=px(700);return 0;}
    case WM_COMMAND:on_command((int)(wp&65535),(int)((wp>>16)&65535));return 0;
    case WM_DROPFILES:{HANDLE drop=(HANDLE)wp;if(!busy){Job *job=calloc(1,sizeof(Job));if(job){job->kind=1;if(DragQueryFileW(drop,0,job->path,260)>0){status(L"正在读取图片…");start_job(job);}else free(job);}}DragFinish(drop);return 0;}
    case WM_APP_DONE:completed((Job*)lp);return 0;
    case WM_PAINT:{PAINTSTRUCT paint;HDC dc=BeginPaint(hwnd,&paint);paint_chrome(dc);EndPaint(hwnd,&paint);return 0;}
    case 43:paint_button((const DRAWITEMSTRUCT*)lp);return 1;
    case 307:SetTextColor((HDC)wp,INK);SetBkColor((HDC)wp,RGB(248,250,253));return (LRESULT)editBrush;
    case WM_CTLCOLORSTATIC:{
        HWND childWindow=(HWND)lp;int base=childWindow==heading||childWindow==statusText||(childWindow==fileText&&page==2);
        int edit=childWindow==b64Controls[0]||childWindow==fields[WIDTH-WIDTH]||childWindow==fields[HEIGHT-WIDTH]||childWindow==fields[QUALITY-WIDTH];
        SetTextColor((HDC)wp,childWindow==fileText||childWindow==statusText?MUTED:INK);
        SetBkColor((HDC)wp,edit?RGB(248,250,253):base?CANVAS:0xffffff);SetBkMode((HDC)wp,1);
        return (LRESULT)(edit?editBrush:base?pageBrush:whiteBrush);
    }
    case WM_SETCURSOR:if(busy){SetCursor(waitCursor);return 1;}break;
    case WM_CLOSE:if(busy){status(L"正在处理图片，请稍后关闭。");return 0;}DestroyWindow(hwnd);return 0;
    case WM_DESTROY:free(b64Text);dispose_image(&decodedSmall);if(decodedPath[0])DeleteFileW(decodedPath);dispose_image(&sourceImage);dispose_image(&sourceSmall);dispose_image(&outputSmall);if(resultPath[0])DeleteFileW(resultPath);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

void app_entry(void)
{
    instance=GetModuleHandleW(0);
    SetProcessDPIAware();CoInitializeEx(0,2);
    GdiplusStartupInput startup={1,0,0,0};
    int code=GdiplusStartup(&gdipToken,&startup,0);
    if(code){MessageBoxW(0,L"Windows 图像组件初始化失败。",TITLE,0x10);ExitProcess(1);}
    HDC dc=GetDC(0);if(dc){dpi=GetDeviceCaps(dc,88);ReleaseDC(0,dc);}if(dpi<72)dpi=96;
    normalFont=CreateFontW(-px(15),0,0,0,400,0,0,0,1,0,0,5,0,L"Microsoft YaHei UI");
    titleFont=CreateFontW(-px(24),0,0,0,600,0,0,0,1,0,0,5,0,L"Microsoft YaHei UI");
    boldFont=CreateFontW(-px(17),0,0,0,600,0,0,0,1,0,0,5,0,L"Microsoft YaHei UI");
    smallFont=CreateFontW(-px(12),0,0,0,400,0,0,0,1,0,0,5,0,L"Microsoft YaHei UI");
    monoFont=CreateFontW(-px(14),0,0,0,400,0,0,0,1,0,0,5,0,L"Consolas");
    editBrush=CreateSolidBrush(RGB(248,250,253));
    pageBrush=CreateSolidBrush(CANVAS);whiteBrush=CreateSolidBrush(0xffffff);checkerBrush=CreateSolidBrush(RGB(241,244,248));borderBrush=CreateSolidBrush(LINE);
    arrowCursor=LoadCursorW(0,RES(32512));handCursor=LoadCursorW(0,RES(32649));waitCursor=LoadCursorW(0,RES(32514));
    WNDCLASSEXW wc={0};wc.cbSize=sizeof(wc);wc.lpfnWndProc=main_proc;wc.hInstance=instance;wc.hIcon=LoadIconW(instance,RES(1));wc.hIconSm=wc.hIcon;wc.hCursor=arrowCursor;wc.hbrBackground=pageBrush;wc.lpszClassName=L"IRT_MAIN_110";
    if(!RegisterClassExW(&wc)){MessageBoxW(0,L"窗口初始化失败。",TITLE,0x10);ExitProcess(2);}
    wc.lpfnWndProc=preview_proc;wc.lpszClassName=L"IRT_IMAGE";wc.hbrBackground=0;wc.style=3;RegisterClassExW(&wc);
    int screenW=GetSystemMetrics(0),screenH=GetSystemMetrics(1),w=px(1180),h=px(838);
    if(w>screenW)w=screenW;
    if(h>screenH-40)h=screenH-40;
    window=CreateWindowExW(0,L"IRT_MAIN_110",TITLE,0x00cf0000|0x02000000,(screenW-w)/2,(screenH-h)/2,w,h,0,0,instance,0);
    if(!window){MessageBoxW(0,L"无法创建主窗口。",TITLE,0x10);ExitProcess(3);}
    ShowWindow(window,1);UpdateWindow(window);
    MSG message;int got;
    while((got=GetMessageW(&message,0,0,0))>0){
        if(message.message==WM_KEYDOWN&&message.wParam=='V'&&(GetKeyState(0x11)&0x8000)&&page!=1&&message.hwnd!=b64Controls[0]&&message.hwnd!=fields[WIDTH-WIDTH]&&message.hwnd!=fields[HEIGHT-WIDTH]&&message.hwnd!=fields[QUALITY-WIDTH]){paste_image();continue;}
        if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    }
    DeleteObject(smallFont);DeleteObject(monoFont);DeleteObject(editBrush);DeleteObject(normalFont);DeleteObject(titleFont);DeleteObject(boldFont);DeleteObject(pageBrush);DeleteObject(whiteBrush);DeleteObject(checkerBrush);DeleteObject(borderBrush);
    GdiplusShutdown(gdipToken);CoUninitialize();ExitProcess(got<0?1:0);
}
