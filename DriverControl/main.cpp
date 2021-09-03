#include <windows.h>
#include <stdio.h>
#include "driver_control.h"

char *opts[] = {
        "/install",
        "/uninstall",
        "/start",
        "/stop"
};

void UseInfo()
{
    printf("parameters info:\n");
    printf("\t/install <filepath>;\n");
    printf("\t/uninstall <filepath>;\n");
    printf("\t/start <service>;\n");
    printf("\t/stop <service>;\n");
}

bool isValidOptions(const char *opt)
{
    int optsSize = sizeof(opts);
    for (int i = 0; i < optsSize; i++)
    {
        if (strcmp(opt, opts[i]) == 0)
            return true;
    }
    return false;
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        goto show_usage;
    }
    else
    {
        if (!isValidOptions(argv[1]))
        {
            goto show_usage;
        }
        else
        {
            if(strcmp("/install", argv[1]) == 0)
            {
                DriverInstall(argv[2]);
            }
            else if(strcmp("/uninstall", argv[1]) == 0)
            {
                DriverUninstall(argv[2]);
            }
            else if(strcmp("/start", argv[1]) == 0)
            {
                DriverStart(argv[2]);
            }
            else if(strcmp("/stop", argv[1]) == 0)
            {
                DriverStop(argv[2]);
            }
            goto out;
        }

    }
show_usage:
    UseInfo();
out:
    return 0;
}