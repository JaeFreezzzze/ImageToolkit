#ifndef IMAGE_GEOMETRY_H
#define IMAGE_GEOMETRY_H
typedef struct { int x,y,w,h; } ImageRect;
static int valid_dimensions(int w,int h) { return w>0 && h>0 && w<=16384 && h<=16384 && (long long)w*h<=40000000; }
static ImageRect fit_image(int sw,int sh,int tw,int th,int stretch)
{
    ImageRect r={0,0,tw,th};
    if (!stretch) {
        if ((long long)sw*th>(long long)sh*tw) r.h=(int)(((long long)sh*tw+sw/2)/sw);
        else r.w=(int)(((long long)sw*th+sh/2)/sh);
        if (r.w<1) r.w=1;
        if (r.h<1) r.h=1;
        r.x=(tw-r.w)/2; r.y=(th-r.h)/2;
    }
    return r;
}
#endif
