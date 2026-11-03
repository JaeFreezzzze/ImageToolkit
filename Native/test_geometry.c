#include <assert.h>
#include <stdio.h>
#include "geometry.h"
int main(void)
{
    ImageRect r=fit_image(1920,1080,1280,720,0);
    assert(r.x==0&&r.y==0&&r.w==1280&&r.h==720);
    r=fit_image(1920,1080,640,640,0);
    assert(r.x==0&&r.y==140&&r.w==640&&r.h==360);
    r=fit_image(1080,1920,640,640,0);
    assert(r.x==140&&r.y==0&&r.w==360&&r.h==640);
    r=fit_image(1920,1080,640,640,1);
    assert(r.x==0&&r.y==0&&r.w==640&&r.h==640);
    r=fit_image(1,10000,640,640,0);
    assert(r.w==1&&r.h==640);
    assert(valid_dimensions(1,1)&&valid_dimensions(8000,5000));
    assert(!valid_dimensions(0,1)&&!valid_dimensions(-1,1080));
    assert(!valid_dimensions(16385,1)&&!valid_dimensions(10000,10000));
    puts("Dimension and aspect-preserving placement checks passed.");
    return 0;
}
