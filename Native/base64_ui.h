/* Integrated Base64 pages; all file/codec work runs on the shared worker. */
#define TEXT_LIMIT (140u*1024*1024)
static int detect_format(const BYTE *p,size_t n){
 if(n>=8&&p[0]==137&&p[1]=='P'&&p[2]=='N'&&p[3]=='G')return 0;
 if(n>=3&&p[0]==255&&p[1]==216&&p[2]==255)return 1;
 if(n>=2&&p[0]=='B'&&p[1]=='M')return 2;
 if(n>=4&&((p[0]=='I'&&p[1]=='I'&&p[2]==42&&p[3]==0)||(p[0]=='M'&&p[1]=='M'&&p[2]==0&&p[3]==42)))return 3;
 if(n>=6&&p[0]=='G'&&p[1]=='I'&&p[2]=='F')return 4;
 return -1;
}
static BYTE *read_bytes(const WCHAR *path,size_t *n){
 WIN32_FILE_ATTRIBUTE_DATA info;if(!GetFileAttributesExW(path,0,&info)||info.sizeHigh||!info.sizeLow||info.sizeLow>100u*1024*1024)return 0;
 *n=info.sizeLow;BYTE *p=malloc(*n);if(!p)return 0;
 HANDLE f=CreateFileW(path,0x80000000,1,0,3,0x80,0);DWORD got=0;
 int ok=f!=(HANDLE)(intptr_t)-1&&ReadFile(f,p,(DWORD)*n,&got,0)&&got==*n;
 if(f!=(HANDLE)(intptr_t)-1)CloseHandle(f);
 if(!ok){free(p);return 0;}return p;
}
static int write_bytes(const WCHAR *path,const BYTE *p,size_t n){
 HANDLE f=CreateFileW(path,0x40000000,0,0,2,0x80,0);DWORD got=0;
 if(f==(HANDLE)(intptr_t)-1)return 0;
 int ok=WriteFile(f,p,(DWORD)n,&got,0)&&got==n;CloseHandle(f);return ok;
}
static void do_base64(Job *job){
 if(job->kind==4){
  size_t n=0;BYTE *bytes=0;WCHAR temp[260]={0};
  if(job->path[0])bytes=read_bytes(job->path,&n);
  else if(temp_file(temp)&&!encode_to(job->source,temp,0,92))bytes=read_bytes(temp,&n);
  if(temp[0])DeleteFileW(temp);
  if(!bytes){job_error(job,0,L"无法读取图片，文件须小于 100 MB");return;}
  job->format=detect_format(bytes,n);
  if(job->format<0){free(bytes);job_error(job,0,L"Base64 转换支持 PNG、JPG、BMP、TIFF、GIF；ICO 请先转换为 PNG");return;}
  job->text=malloc(4*((n+2)/3)+1);
  if(!job->text){free(bytes);job_error(job,0,L"可用内存不足");return;}
  job->textSize=b64_encode(bytes,n,job->text);free(bytes);
 }else{
  char *p=job->text;size_t n=job->textSize;int declared=-1;
  if(n>5&&(p[0]=='d'||p[0]=='D')&&(p[1]=='a'||p[1]=='A')&&(p[2]=='t'||p[2]=='T')&&(p[3]=='a'||p[3]=='A')&&p[4]==':'){
   size_t comma=0;while(comma<n&&p[comma]!=',')comma++;
   if(comma>=100||comma==n){job_error(job,0,L"Data URI 格式无效");return;}
   char meta[100];for(size_t i=0;i<comma;i++)meta[i]=p[i]>='A'&&p[i]<='Z'?p[i]+32:p[i];meta[comma]=0;
   const char *types[]={"data:image/png;base64","data:image/jpeg;base64","data:image/bmp;base64","data:image/tiff;base64","data:image/gif;base64","data:image/jpg;base64","data:image/x-ms-bmp;base64"};
   for(int i=0;i<7;i++){size_t j=0;while(types[i][j]&&types[i][j]==meta[j])j++;if(!types[i][j]&&!meta[j])declared=i<5?i:i==5?1:2;}
   if(declared<0){job_error(job,0,L"Data URI 类型不受支持或缺少 base64 标记");return;}p+=comma+1;n-=comma+1;
  }
  BYTE *bytes=malloc(n/4*3+3);size_t length=0;
  if(!bytes){job_error(job,0,L"可用内存不足");return;}
  int ok=b64_decode(p,n,bytes,&length);int fmt=ok?detect_format(bytes,length):-1;
  if(!ok||fmt<0||(declared>=0&&declared!=fmt)||length>100u*1024*1024){free(bytes);job_error(job,0,L"Base64 无效、图片类型不匹配或超过 100 MB");return;}
  job->format=fmt;
  if(!temp_file(job->temporary)||!write_bytes(job->temporary,bytes,length)){free(bytes);job_error(job,0,L"无法写入临时图片");return;}free(bytes);
  textcopy(job->path,260,job->temporary);do_load(job);
 }
}
static void b64_output_preview(void){
 WCHAR preview[16400];size_t j=0;
 if(b64Text&&SendMessageW(b64Controls[1],BM_GETCHECK,0,0)){
  _snwprintf(preview,100,L"data:%ls;base64,",MIMES[b64Format]);j=wcslen(preview);
 }
 for(size_t i=0;b64Text&&i<b64Length&&i<16000;i++)preview[j++]=(unsigned char)b64Text[i];
 preview[j]=0;
 SetWindowTextW(b64Controls[0],preview);
 SetWindowTextW(b64Controls[3],L"复制完整结果");
}
static void b64_copy(void){
 if(!b64Text)return;
 WCHAR prefix[100]={0};if(SendMessageW(b64Controls[1],BM_GETCHECK,0,0))_snwprintf(prefix,100,L"data:%ls;base64,",MIMES[b64Format]);
 size_t pre=wcslen(prefix);HANDLE mem=GlobalAlloc(2,(pre+b64Length+1)*2);if(!mem){error_box(L"可用内存不足。");return;}
 WCHAR *p=GlobalLock(mem);if(!p){GlobalFree(mem);return;}memcpy(p,prefix,pre*2);for(size_t i=0;i<b64Length;i++)p[pre+i]=(unsigned char)b64Text[i];p[pre+b64Length]=0;GlobalUnlock(mem);
 if(!OpenClipboard(window)){GlobalFree(mem);error_box(L"剪贴板正被占用，请重试。");return;}
 int ok=EmptyClipboard()&&SetClipboardData(13,mem)!=0;CloseClipboard();if(!ok){GlobalFree(mem);error_box(L"复制失败，请重试。");return;}
 SetWindowTextW(b64Controls[3],L"✓ 复制成功");status(L"完整 Base64 已复制到剪贴板。");
}
static void b64_save(int decoded){
 if(decoded?!decodedPath[0]:!b64Text)return;
 WCHAR path[260]=L"image-base64.txt";if(decoded)_snwprintf(path,260,L"decoded-image.%ls",EXTENSIONS[decodedFormat]);
 OPENFILENAMEW d={0};d.lStructSize=sizeof(d);d.hwndOwner=window;d.lpstrFile=path;d.nMaxFile=260;d.Flags=0x80000|0x800|2|8;
 d.lpstrFilter=decoded?SAVE_FILTERS[decodedFormat]:L"文本文件\0*.txt\0\0";d.lpstrDefExt=decoded?EXTENSIONS[decodedFormat]:L"txt";
 if(!GetSaveFileNameW(&d))return;
 WCHAR full[260];DWORD count=GetFullPathNameW(path,260,full,0);
 if(!count||count>=260|| (sourcePath[0]&&!_wcsicmp(full,sourcePath))){error_box(L"请使用新的有效文件名保存。");return;}
 if(decoded&&_wcsicmp(extension(path),EXTENSIONS[decodedFormat])&&!(decodedFormat==1&&!_wcsicmp(extension(path),L"jpeg"))&&!(decodedFormat==3&&!_wcsicmp(extension(path),L"tif"))){error_box(L"文件扩展名与图片格式不一致。");return;}
 Job *job=calloc(1,sizeof(Job));if(!job)return;job->kind=3;textcopy(job->path,260,full);
 if(decoded)textcopy(job->temporary,260,decodedPath);
 else{
  WCHAR prefix[100]={0};if(SendMessageW(b64Controls[1],BM_GETCHECK,0,0))_snwprintf(prefix,100,L"data:%ls;base64,",MIMES[b64Format]);size_t pre=wcslen(prefix);
  BYTE *bytes=malloc(pre+b64Length);if(!bytes){free(job);return;}for(size_t i=0;i<pre;i++)bytes[i]=(BYTE)prefix[i];memcpy(bytes+pre,b64Text,b64Length);
  int ok=temp_file(job->temporary)&&write_bytes(job->temporary,bytes,pre+b64Length);free(bytes);
  if(!ok){if(job->temporary[0])DeleteFileW(job->temporary);free(job);error_box(L"无法准备文本文件。");return;}job->deleteTemporary=1;
 }
 status(L"正在保存…");start_job(job);
}
static void b64_command(int id){
 if(id==303){b64_copy();return;}if(id==304){b64_save(0);return;}if(id==309){b64_save(1);return;}
 if(id==301){b64_output_preview();return;}
 if(id==305){free(b64Text);b64Text=0;b64Length=0;b64_output_preview();clear_image();return;}
 if(id==310){SetWindowTextW(b64Controls[6],L"");dispose_image(&decodedSmall);if(decodedPath[0])DeleteFileW(decodedPath);decodedPath[0]=0;InvalidateRect(outView,0,0);status(L"已清空 Base64 输入和解码结果。");return;}
 if(id==311){if(!decodedPath[0])return;Job *job=calloc(1,sizeof(Job));if(!job)return;job->kind=1;job->clipboard=1;textcopy(job->path,260,decodedPath);page=2;resize_layout();start_job(job);return;}
 if(id==302){if(!sourceImage)return;Job *job=calloc(1,sizeof(Job));if(!job)return;job->kind=4;job->source=sourceImage;textcopy(job->path,260,sourcePath);status(L"正在生成 Base64…");start_job(job);return;}
 if(id==308){
  int n=GetWindowTextLengthW(b64Controls[6]);if(n<=0||(unsigned)n>TEXT_LIMIT){error_box(L"请粘贴有效的 Base64 内容。");return;}
  WCHAR *wide=malloc(((size_t)n+1)*2);Job *job=calloc(1,sizeof(Job));if(!wide||!job){free(wide);free(job);return;}
  GetWindowTextW(b64Controls[6],wide,n+1);job->text=malloc((size_t)n+1);if(!job->text){free(wide);free(job);return;}
  size_t j=0;for(int i=0;i<n;i++){if(wide[i]>127){free(wide);free(job->text);free(job);error_box(L"Base64 中含有非 ASCII 字符。");return;}if(wide[i]!=' '&&wide[i]!='\t'&&wide[i]!='\r'&&wide[i]!='\n')job->text[j++]=(char)wide[i];}
  free(wide);job->text[j]=0;job->textSize=j;job->kind=5;status(L"正在解码图片…");start_job(job);
 }
}
