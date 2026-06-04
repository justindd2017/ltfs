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
#include "spti_scsi_tape.h"
#include "spti_tape.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Forward declarations from spti_tape.c
 */
int spti_send_scsi_command(void *device, int direction, unsigned char *cdb,
                          unsigned int cdb_len, unsigned char *data,
                          unsigned int *data_len, unsigned char *sense,
                          unsigned int *sense_len, int timeout);

/**
 * Test Unit Ready command
 * Checks if device is ready to accept commands
 */
int spti_cdb_test_unit_ready(void *device)
{
    unsigned char cdb[6] = {SCSI_TEST_UNIT_READY, 0, 0, 0, 0, 0};
    unsigned char sense_data[32];
    unsigned int sense_len = sizeof(sense_data);

    return spti_send_scsi_command(device, SCSI_DATA_NONE, cdb, sizeof(cdb),
                                 NULL, NULL, sense_data, &sense_len, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Inquiry command
 * Retrieves device identification and capabilities
 */
int spti_cdb_inquiry(void *device, unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[6] = {SCSI_INQUIRY, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    cdb[4] = (unsigned char)bufsize;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Request Sense command
 * Retrieves error information from device
 */
int spti_cdb_request_sense(void *device, unsigned char *buf, unsigned char size)
{
    unsigned char cdb[6] = {SCSI_REQUEST_SENSE, 0, 0, 0, 0, 0};
    unsigned int data_len = size;

    cdb[4] = size;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Read Buffer command
 * Reads device buffer for diagnostics
 */
int spti_cdb_read_buffer(void *device, int id, unsigned char *buf,
                        size_t offset, size_t len, int type)
{
    unsigned char cdb[10] = {SCSI_READ_BUFFER, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int data_len = len;

    cdb[1] = type;
    cdb[2] = id;
    cdb[3] = (offset >> 16) & 0xFF;
    cdb[4] = (offset >> 8) & 0xFF;
    cdb[5] = offset & 0xFF;
    cdb[6] = (len >> 16) & 0xFF;
    cdb[7] = (len >> 8) & 0xFF;
    cdb[8] = len & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_LONG_TIMEOUT);
}

/**
 * Mode Sense command
 * Reads device parameters and modes
 */
int spti_cdb_mode_sense(void *device, unsigned char page_code,
                       unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[6] = {SCSI_MODE_SENSE, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    cdb[2] = page_code;
    cdb[4] = (unsigned char)bufsize;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Load/Unload command
 * Loads or unloads the tape cartridge
 */
int spti_cdb_load_unload(void *device, bool load)
{
    unsigned char cdb[6] = {SCSI_LOAD_UNLOAD, 0, 0, 0, 0, 0};

    cdb[4] = load ? 0x01 : 0x00;

    return spti_send_scsi_command(device, SCSI_DATA_NONE, cdb, sizeof(cdb),
                                 NULL, NULL, NULL, NULL, SPTI_LONG_TIMEOUT);
}

/**
 * Read command
 * Reads data from tape
 */
int spti_cdb_read(void *device, char *buf, size_t size, bool sili)
{
    unsigned char cdb[6] = {SCSI_READ, 0, 0, 0, 0, 0};
    unsigned int data_len = size;

    cdb[1] = sili ? 0x04 : 0x00;
    cdb[2] = (size >> 16) & 0xFF;
    cdb[3] = (size >> 8) & 0xFF;
    cdb[4] = size & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 (unsigned char *)buf, &data_len, NULL, NULL,
                                 SPTI_LONG_TIMEOUT);
}

/**
 * Write command
 * Writes data to tape
 */
int spti_cdb_write(void *device, unsigned char *buf, size_t size)
{
    unsigned char cdb[6] = {SCSI_WRITE, 0, 0, 0, 0, 0};
    unsigned int data_len = size;

    cdb[2] = (size >> 16) & 0xFF;
    cdb[3] = (size >> 8) & 0xFF;
    cdb[4] = size & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_OUT, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL,
                                 SPTI_LONG_TIMEOUT);
}

/**
 * Rewind command
 * Rewinds tape to beginning
 */
int spti_cdb_rewind(void *device)
{
    unsigned char cdb[6] = {SCSI_REWIND, 0, 0, 0, 0, 0};

    return spti_send_scsi_command(device, SCSI_DATA_NONE, cdb, sizeof(cdb),
                                 NULL, NULL, NULL, NULL, SPTI_LONG_TIMEOUT);
}

/**
 * Space command
 * Moves tape position (forward/backward, blocks/files)
 */
int spti_cdb_space(void *device, int code, long count)
{
    unsigned char cdb[6] = {SCSI_SPACE, 0, 0, 0, 0, 0};

    cdb[1] = code & 0x07;
    cdb[2] = (count >> 16) & 0xFF;
    cdb[3] = (count >> 8) & 0xFF;
    cdb[4] = count & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_NONE, cdb, sizeof(cdb),
                                 NULL, NULL, NULL, NULL, SPTI_LONG_TIMEOUT);
}

/**
 * Read Position command
 * Retrieves current tape position
 */
int spti_cdb_read_position(void *device, unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[10] = {SCSI_READ_POSITION, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    cdb[1] = 0x00;
    cdb[8] = bufsize;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Log Sense command
 * Retrieves device logs and statistics
 */
int spti_cdb_log_sense(void *device, unsigned char page_code,
                      unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[10] = {SCSI_LOG_SENSE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    cdb[2] = page_code | 0x40;
    cdb[7] = (bufsize >> 8) & 0xFF;
    cdb[8] = bufsize & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Persistent Reserve In command
 * Reads persistent reservation information
 */
int spti_cdb_persistent_reserve_in(void *device, unsigned char action,
                                  unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[10] = {SCSI_PERSISTENT_RESERVE_IN, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    cdb[1] = action;
    cdb[7] = (bufsize >> 8) & 0xFF;
    cdb[8] = bufsize & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Persistent Reserve Out command
 * Sets persistent reservation
 */
int spti_cdb_persistent_reserve_out(void *device, unsigned char action,
                                   unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[10] = {SCSI_PERSISTENT_RESERVE_OUT, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    cdb[1] = action;
    cdb[7] = (bufsize >> 8) & 0xFF;
    cdb[8] = bufsize & 0xFF;

    return spti_send_scsi_command(device, SCSI_DATA_OUT, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}

/**
 * Read Block Limits command
 * Retrieves tape block size limits
 */
int spti_cdb_read_block_limits(void *device, unsigned char *buf, size_t bufsize)
{
    unsigned char cdb[6] = {SCSI_READ_BLOCK_LIMITS, 0, 0, 0, 0, 0};
    unsigned int data_len = bufsize;

    return spti_send_scsi_command(device, SCSI_DATA_IN, cdb, sizeof(cdb),
                                 buf, &data_len, NULL, NULL, SPTI_DEFAULT_TIMEOUT);
}
