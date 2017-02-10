/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/common/gpe_export.h $                                     */
/*                                                                        */
/* OpenPOWER OnChipController Project                                     */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2011,2017                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */

#ifndef _GPE_EXPORT_H
#define _GPE_EXPORT_H

#include "gpe_err.h"

// GPE Error structure (common to both GPEs)
typedef struct {
     union
     {
       struct {
         uint32_t rc;
         uint32_t addr;
       };
       uint64_t error;
     };
     uint64_t ffdc;
} GpeErrorStruct;

// Arguments for doing a SCOM from GPE0
typedef struct ipc_scom_op
{
    GpeErrorStruct  error;  // Error of SCOM operation
    uint32_t        addr;   // Register address
    uint64_t        data;   // Data for read/write
    uint32_t        size;   // Size of data buffer
    uint8_t         read;   // Read (1) or write (0)
} ipc_scom_op_t;

typedef struct nop
{
    GpeErrorStruct  error;  // Error of operation
} nop_t;

typedef struct gpe_shared_data
{
    uint32_t    nest_freq_div; // Nest freq / 4
    uint32_t    reserved[63];
} gpe_shared_data_t;

#endif //_GPE_EXPORT_H
