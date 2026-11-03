#ifndef BASE64_CODEC_H
#define BASE64_CODEC_H
#include <stddef.h>
static const char B64_ALPHABET[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static size_t b64_encode(const unsigned char *in,size_t n,char *out) {
 size_t j=0;
 for(size_t i=0;i<n;i+=3){unsigned v=(unsigned)in[i]<<16;if(i+1<n)v|=(unsigned)in[i+1]<<8;if(i+2<n)v|=in[i+2];
 out[j++]=B64_ALPHABET[v>>18];out[j++]=B64_ALPHABET[(v>>12)&63];out[j++]=i+1<n?B64_ALPHABET[(v>>6)&63]:'=';out[j++]=i+2<n?B64_ALPHABET[v&63]:'=';}
 out[j]=0;return j;
}
static int b64_value(unsigned c){if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;if(c=='+')return 62;if(c=='/')return 63;return -1;}
/* Strict padding / trailing-bit validation; whitespace and unpadded input accepted. */
static int b64_decode(const char *in,size_t n,unsigned char *out,size_t *length){
 unsigned acc=0;int bits=0,pad=0;size_t count=0,j=0;
 for(size_t i=0;i<n;i++){unsigned c=(unsigned char)in[i];if(c==' '||c=='\r'||c=='\n'||c=='\t')continue;
 if(c=='='){if(++pad>2)return 0;continue;}int v=b64_value(c);if(v<0||pad)return 0;
 acc=(acc<<6)|(unsigned)v;bits+=6;count++;if(bits>=8){bits-=8;out[j++]=(unsigned char)(acc>>bits);}}
 if(!count||count%4==1||(pad&&((count+pad)%4||pad!=(int)((4-count%4)%4)))||(bits&&(acc&((1u<<bits)-1))))return 0;
 *length=j;return 1;
}
#endif
