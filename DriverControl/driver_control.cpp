//
// Created by CHM on 2021/9/2.
//
#include "driver_control.h"
#include <winsvc.h>

// 从路径中获取无后缀的文件名
// 注意：此函数会修改输入的路径
// 返回值：
//		成功：文件名
//		失败：NULL
char* GetFileNameNoSuffixFromPath(char* pPath)
{
    char* pFileName = NULL;
    int nLen = strlen(pPath);
    int nIndex = nLen - 1;
    for (; nIndex >= 0; nIndex--)
    {
        if (pPath[nIndex] == '\\' || pPath[nIndex] == '/')
        {
            break;
        }
    }
    if (nIndex >= 0)
    {
        pFileName = pPath + nIndex + 1;
        int nSuffixIndex = strlen(pFileName) - 1;
        for (; nSuffixIndex >= 0; nSuffixIndex--)
        {
            if (pFileName[nSuffixIndex] == '.')
            {
                pFileName[nSuffixIndex] = '\0';
                break;
            }
        }
    }
    return pFileName;
}


/**
 * @brief
 * @param driverFilePath
 * @return
 */
bool DriverInstall(char *driverFilePath) {
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    char *serviceName = NULL;

    schSCManager = OpenSCManagerA(NULL,
                                  NULL,
                                  SC_MANAGER_ALL_ACCESS);

    if (schSCManager == NULL)
    {
        printf("OpenSCManagerA failed, error code: %d\n", GetLastError());
        return false;
    }
	char tmpDriverFilePath[256] = { 0 };
	strcpy_s(tmpDriverFilePath, 256, driverFilePath);

	serviceName = GetFileNameNoSuffixFromPath(tmpDriverFilePath);
    if (serviceName == NULL)
    {
        printf("GetFileNameNoSuffixFromPath failed\n");
        return false;
    }

    schService = CreateServiceA(schSCManager,
                                serviceName,
                                serviceName,
                                SERVICE_ALL_ACCESS,
                                SERVICE_KERNEL_DRIVER,
                                SERVICE_SYSTEM_START,
                                SERVICE_ERROR_NORMAL,
								driverFilePath,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

    if (schService == NULL)
    {
        printf("CreateServiceA failed, error code = %d\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return false;
    }

    printf("CreateServiceA success!\n");
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

/**
 * @brief
 * @param driverFilePath
 * @return
 */
bool DriverUninstall(char *driverFilePath) {
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    char *serviceName = NULL;

    schSCManager = OpenSCManagerA(NULL,
                                  NULL,
                                  SC_MANAGER_ALL_ACCESS);

    if (schSCManager == NULL)
    {
        printf("OpenSCManagerA failed, error code: %d\n", GetLastError());
        return false;
    }
	char tmpDriverFilePath[256] = { 0 };
	strcpy_s(tmpDriverFilePath, 256, driverFilePath);

	serviceName = GetFileNameNoSuffixFromPath(tmpDriverFilePath);
    if (serviceName == NULL)
    {
        printf("GetFileNameNoSuffixFromPath failed\n");
        return false;
    }

    schService = OpenServiceA(schSCManager,
                              serviceName,
                              DELETE);
    if (schService == NULL)
    {
        printf("OpenServiceA failed, error code: %d\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return false;
    }

    if (!DeleteService(schService))
    {
        printf("DeleteService failed, error code: %d\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return false;
    }

    printf("DeleteService success!\n");
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

/**
 * @brief
 * @param serviceName
 * @return
 */
bool DriverStart(char *serviceName) {
    SC_HANDLE schManager;

    schManager = OpenSCManagerA(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);

    if (schManager == NULL)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return false;
    }

    SC_HANDLE schService;

    schService = OpenServiceA(schManager,
                             serviceName,
                             SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(schManager);
        return false;
    }

    DWORD dwBytesNeeded;
    SERVICE_STATUS_PROCESS ssStatus;
    // 查询驱动当前状态
    if (!QueryServiceStatusEx(schService,
                              SC_STATUS_PROCESS_INFO,
                              (LPBYTE)&ssStatus,
                              sizeof(SERVICE_STATUS_PROCESS),
                              &dwBytesNeeded))
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return false;
    }

    // 如果驱动是非停止状态和停止挂起状态，现在启动会失败
    if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        printf("Cannot start the service because it is already running\n");
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return false;
    }

    // 如果驱动处于stop pending状态 到等到变成stop状态才可以
    DWORD dwWaitTime;
    DWORD dwStartTickCount = GetTickCount();
    DWORD dwOldCheckPoint = ssStatus.dwCheckPoint;
    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is
        // one-tenth of the wait hint but not less than 1 second
        // and not more than 10 seconds.

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status until the service is no longer stop pending.

        if (!QueryServiceStatusEx(
                schService,                     // handle to service
                SC_STATUS_PROCESS_INFO,         // information level
                (LPBYTE)&ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded))              // size needed if buffer is too small
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schManager);
            return false;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                printf("Timeout waiting for service to stop\n");
                CloseServiceHandle(schService);
                CloseServiceHandle(schManager);
                return false;
            }
        }
    }

    // 尝试去启动驱动
    if (!StartService(schService,
                      0,
                      NULL))
    {
        printf("StartService failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return false;
    }
    else
    {
        printf("Service start pending...\n");
    }

    if (!QueryServiceStatusEx(
            schService,                     // handle to service
            SC_STATUS_PROCESS_INFO,         // info level
            (LPBYTE)&ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // if buffer too small
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return false;
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is
        // one-tenth the wait hint, but no less than 1 second and no
        // more than 10 seconds.

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status again.

        if (!QueryServiceStatusEx(
                schService,             // handle to service
                SC_STATUS_PROCESS_INFO, // info level
                (LPBYTE)&ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded))              // if buffer too small
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            break;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    }

    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
        printf("Service started successfully.\n");
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return true;
    }
    else
    {
        printf("Service not started. \n");
        printf("  Current State: %d\n", ssStatus.dwCurrentState);
        printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
        printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
        printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);

        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return false;
    }
    return true;
}

/**
 * @brief
 * @param serviceName
 * @return
 */
bool DriverStop(char *serviceName) {

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out
    DWORD dwWaitTime;

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManagerA(
            NULL,                    // local computer
            NULL,                    // ServicesActive database
            SC_MANAGER_ALL_ACCESS);  // full access rights

    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return false;
    }

    schService = OpenServiceA(
            schSCManager,         // SCM database
            serviceName,            // name of service
            SERVICE_STOP |
            SERVICE_QUERY_STATUS);

    if (schService == NULL)
    {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return false;
    }

    if (!QueryServiceStatusEx(
            schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssStatus,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    if (ssStatus.dwCurrentState == SERVICE_STOPPED)
    {
        printf("Service is already stopped.\n");
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        printf("Service stop pending...\n");

        // Do not wait longer than the wait hint. A good interval is
        // one-tenth of the wait hint but not less than 1 second
        // and not more than 10 seconds.

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssStatus,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return false;
        }

        if (ssStatus.dwCurrentState == SERVICE_STOPPED)
        {
            printf("Service stopped successfully.\n");
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return true;
        }

        if (GetTickCount() - dwStartTime > dwTimeout)
        {
            printf("Service stop timed out.\n");
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return false;
        }
    }

    if (!ControlService(
            schService,
            SERVICE_CONTROL_STOP,
            (LPSERVICE_STATUS)&ssStatus))
    {
        printf("ControlService failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }


    while (ssStatus.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(ssStatus.dwWaitHint);
        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssStatus,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return false;
        }

        if (ssStatus.dwCurrentState == SERVICE_STOPPED)
            break;

        if (GetTickCount() - dwStartTime > dwTimeout)
        {
            printf("Wait timed out\n");
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return false;
        }
    }
    printf("Service stopped successfully\n");

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}
