#include <windows.h>
void app_entry(void);
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command, int show)
{
    (void)instance; (void)previous; (void)command; (void)show;
    app_entry();
    return 0;
}
