/*
**
**  OO_Copyright_BEGIN
**
**
**  Copyright 2010, 2026 IBM Corp. All rights reserved.
**
**  Redistribution and use in source and binary forms, with or without
**   modification, are permitted provided that the following conditions
**  are met:
**  1. Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**  2. Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in the
**  documentation and/or other materials provided with the distribution.
**  3. Neither the name of the copyright holder nor the names of its
**     contributors may be used to endorse or promote products derived from
**     this software without specific prior written permission.
**
**  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
**  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
**  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
**  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
**  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
**  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
**  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
**  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
**  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
**  POSSIBILITY OF SUCH DAMAGE.
**
**
**  OO_Copyright_END
**
*/

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libltfs/ltfslogging.h"
#include "libltfs/tape_ops.h"
#include "libltfs/ltfs.h"
#include "spti_tape.h"
#include "spti_scsi_tape.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Enumerate available tape devices via Windows registry and device interface
 */
int spti_enumerate_devices(char ***device_names, int *count)
{
    DWORD dwDriveNumber = 0;
    HANDLE hDevice;
    char device_path[256];
    char **devices = NULL;
    int device_count = 0;
    int max_devices = 10;

    ltfsmsg(LTFS_DEBUG, 11082D, "SPTI enumerate_devices called");

    devices = (char **)malloc(max_devices * sizeof(char *));
    if (!devices) {
        ltfsmsg(LTFS_ERR, 11083E, "Failed to allocate device list memory");
        return -ENOMEM;
    }

    for (dwDriveNumber = 0; dwDriveNumber < 10; dwDriveNumber++) {
        sprintf_s(device_path, sizeof(device_path), "\\\\.\\TAPE%lu", dwDriveNumber);

        hDevice = CreateFileA(
            device_path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(hDevice);

            devices[device_count] = (char *)malloc(strlen(device_path) + 1);
            if (!devices[device_count]) {
                ltfsmsg(LTFS_ERR, 11083E, "Failed to allocate device name memory");
                goto out_enumerate_error;
            }

            strcpy_s(devices[device_count], strlen(device_path) + 1, device_path);
            device_count++;

            if (device_count >= max_devices) {
                max_devices *= 2;
                char **tmp = (char **)realloc(devices, max_devices * sizeof(char *));
                if (!tmp) {
                    ltfsmsg(LTFS_ERR, 11083E, "Failed to resize device list");
                    goto out_enumerate_error;
                }
                devices = tmp;
            }
        }
    }

    *device_names = devices;
    *count = device_count;

    ltfsmsg(LTFS_DEBUG, 11084D, "Found %d tape devices", device_count);
    return 0;

out_enumerate_error:
    for (int i = 0; i < device_count; i++) {
        free(devices[i]);
    }
    free(devices);
    return -ENOMEM;
}

/**
 * Open SPTI device and initialize tape operations
 */
int spti_open_device(const char *devname, void **handle)
{
    struct spti_data *priv = NULL;
    HANDLE hDevice;
    int ret = 0;

    ltfsmsg(LTFS_DEBUG, 11082D, "SPTI open_device: %s", devname);

    if (!devname || !handle) {
        ltfsmsg(LTFS_ERR, 11083E, "Invalid parameters to open_device");
        return -EINVAL;
    }

    priv = (struct spti_data *)malloc(sizeof(struct spti_data));
    if (!priv) {
        ltfsmsg(LTFS_ERR, 11083E, "Failed to allocate SPTI device structure");
        return -ENOMEM;
    }

    memset(priv, 0, sizeof(struct spti_data));

    hDevice = CreateFileA(
        devname,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        ltfsmsg(LTFS_ERR, 11083E, "Failed to open device %s: error 0x%lx", devname, dwError);
        ret = -EIO;
        goto out_open_error;
    }

    priv->device_handle = hDevice;
    priv->devname = (char *)malloc(strlen(devname) + 1);
    if (!priv->devname) {
        ltfsmsg(LTFS_ERR, 11083E, "Failed to allocate device name memory");
        ret = -ENOMEM;
        goto out_open_error;
    }

    strcpy_s(priv->devname, strlen(devname) + 1, devname);

    priv->loaded = false;
    priv->is_reserved = false;
    priv->use_sili = true;
    priv->force_writeperm = DEFAULT_WRITEPERM_THRESHOLD;
    priv->force_readperm = DEFAULT_READPERM_THRESHOLD;

    *handle = (void *)priv;

    ltfsmsg(LTFS_INFO, 11085I, "SPTI device opened successfully: %s", devname);
    return 0;

out_open_error:
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    if (priv) {
        if (priv->devname) free(priv->devname);
        free(priv);
    }
    return ret;
}

/**
 * Close SPTI device
 */
int spti_close_device(void **handle)
{
    struct spti_data *priv = NULL;

    if (!handle || !*handle) {
        return -EINVAL;
    }

    priv = (struct spti_data *)*handle;

    ltfsmsg(LTFS_DEBUG, 11082D, "SPTI close_device: %s", priv->devname);

    if (priv->device_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(priv->device_handle);
    }

    if (priv->devname) {
        free(priv->devname);
    }

    if (priv->profiler) {
        fclose(priv->profiler);
    }

    free(priv);
    *handle = NULL;

    ltfsmsg(LTFS_INFO, 11086I, "SPTI device closed");
    return 0;
}

/**
 * Issue SCSI command via Windows SPTI
 * This is the core function that interfaces with Windows SCSI Pass-Through
 */
int spti_send_scsi_command(void *device, int direction, unsigned char *cdb,
                          unsigned int cdb_len, unsigned char *data,
                          unsigned int *data_len, unsigned char *sense,
                          unsigned int *sense_len, int timeout)
{
    struct spti_data *priv = (struct spti_data *)device;
    SCSI_PASS_THROUGH_DIRECT sptd;
    DWORD dwReturned = 0;
    BOOL bResult;
    int ret = 0;

    if (!priv || !cdb) {
        return -EINVAL;
    }

    memset(&sptd, 0, sizeof(SCSI_PASS_THROUGH_DIRECT));

    sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptd.ScsiStatus = 0;
    sptd.PathId = 0;
    sptd.TargetId = 0;
    sptd.Lun = 0;
    sptd.CdbLength = cdb_len;
    sptd.SenseInfoLength = 32;
    sptd.DataIn = (direction == SCSI_DATA_IN) ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT;
    sptd.DataTransferLength = (data_len) ? *data_len : 0;
    sptd.TimeOutValue = (timeout > 0) ? timeout : SPTI_DEFAULT_TIMEOUT;
    sptd.DataBuffer = data;
    sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT, SenseArea);

    memcpy(sptd.Cdb, cdb, MIN(cdb_len, 16));

    bResult = DeviceIoControl(
        priv->device_handle,
        IOCTL_SCSI_PASS_THROUGH_DIRECT,
        &sptd,
        sizeof(SCSI_PASS_THROUGH_DIRECT),
        &sptd,
        sizeof(SCSI_PASS_THROUGH_DIRECT),
        &dwReturned,
        NULL);

    if (!bResult) {
        DWORD dwError = GetLastError();
        ltfsmsg(LTFS_DEBUG, 11082D, "SPTI DeviceIoControl failed: 0x%lx", dwError);
        ret = -EIO;
    }

    if (sense && sense_len) {
        unsigned int copy_len = MIN(32, *sense_len);
        memcpy(sense, sptd.SenseArea, copy_len);
        *sense_len = copy_len;
    }

    if (data_len) {
        *data_len = sptd.DataTransferLength;
    }

    if (sptd.ScsiStatus != SCSISTAT_GOOD && sptd.ScsiStatus != SCSISTAT_CHECK_CONDITION) {
        ret = -EIO;
    }

    return ret;
}

/**
 * Get backend name
 */
const char *spti_get_backend_name(void)
{
    return "winfsp";
}

/**
 * Get list of supported devices
 */
int spti_get_supported_devices(struct supported_device **devices, int *count)
{
    static struct supported_device supported_devices[] = {
        {"IBM", "LTO", "5"},
        {"IBM", "LTO", "6"},
        {"IBM", "LTO", "7"},
        {"IBM", "LTO", "8"},
        {"IBM", "LTO", "9"},
        {"IBM", "TS1140", ""},
        {"IBM", "TS1150", ""},
        {"IBM", "TS1155", ""},
        {"IBM", "TS1160", ""},
        {"HP", "LTO", "5"},
        {"HP", "LTO", "6"},
        {"HP", "LTO", "7"},
        {"HP", "LTO", "8"},
        {"HP", "LTO", "9"},
        {"Quantum", "LTO", "5"},
        {"Quantum", "LTO", "6"},
        {"Quantum", "LTO", "7"},
        {"Quantum", "LTO", "8"},
        {"Quantum", "LTO", "9"},
    };

    *devices = supported_devices;
    *count = sizeof(supported_devices) / sizeof(supported_devices[0]);
    return 0;
}
