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

#ifndef __spti_tape_h
#define __spti_tape_h

#include <windows.h>

struct spti_data {
    HANDLE               device_handle;
    struct sg_tape       dev;
    bool                 loaded;
    bool                 loadfailed;
    bool                 is_reserved;
    bool                 is_tape_locked;
    bool                 is_reconnecting;
    char                 drive_serial[255];
    long                 fetch_sec_acq_loss_w;
    bool                 dirty_acq_loss_w;
    float                acq_loss_w;
    uint64_t             tape_alert;
    unsigned char        dki[12];
    bool                 use_sili;
    int                  vendor;
    int                  drive_type;
    bool                 clear_by_pc;
    uint64_t             force_writeperm;
    uint64_t             force_readperm;
    uint64_t             write_counter;
    uint64_t             read_counter;
    int                  force_errortype;
    char                 *devname;
    unsigned char        key[KEYLEN];
    bool                 is_worm;
    unsigned char        cart_type;
    unsigned char        density_code;
    crc_enc              f_crc_enc;
    crc_check            f_crc_check;
    struct timeout_tape  *timeouts;
    struct tc_drive_info info;
    FILE*                profiler;
    int                  recursive_counter;
};

struct spti_global_data {
    char     *str_crc_checking;
    unsigned crc_checking;
    unsigned strict_drive;
    unsigned disable_auto_dump;
    unsigned capacity_offset;
};

#endif /* __spti_tape_h */
