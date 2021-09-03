//
// Created by CHM on 2021/9/2.
//

#ifndef DRIVERCONTROL_DRIVER_CONTROL_H
#define DRIVERCONTROL_DRIVER_CONTROL_H

#include <windows.h>
#include <stdio.h>


bool DriverInstall(char *driverFilePath);
bool DriverUninstall(char *driverFilePath);

bool DriverStart(char *serviceName);
bool DriverStop(char *serviceName);

#endif //DRIVERCONTROL_DRIVER_CONTROL_H
