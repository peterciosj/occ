/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/occ/amec/amec_wof.h $                                     */
/*                                                                        */
/* OpenPOWER OnChipController Project                                     */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015                             */
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

#ifndef _AMEC_WOF_H
#define _AMEC_WOF_H

//*************************************************************************
// Includes
//*************************************************************************
#include <sensor.h>
#include <occ_common.h>
#include <occ_sys_config.h> //MAX_NUM_CORES
#include <ssx_api.h> // SsxSemaphore
#include <pstates.h> // GlobalPstateTable

//*************************************************************************
// Externs
//*************************************************************************

//WOF parameters defined in amec_wof.c
extern sensor_t g_amec_wof_ceff_ratio_sensor;
extern sensor_t g_amec_wof_core_wake_sensor;
extern sensor_t g_amec_wof_vdd_sense_sensor;
extern uint64_t g_amec_wof_wake_mask;
extern uint64_t g_amec_wof_wake_mask_save;
extern uint8_t g_amec_wof_make_check;
extern uint8_t g_amec_wof_check;
extern GlobalPstateTable g_amec_wof_pstate_table_0;
extern GlobalPstateTable g_amec_wof_pstate_table_1;
extern uint8_t g_amec_wof_current_pstate_table;
extern uint8_t g_amec_wof_pstate_table_ready;
extern uint16_t G_amec_wof_thread_counter;
extern SsxSemaphore G_amecWOFThreadWakeupSem;

//*************************************************************************
// Macros
//*************************************************************************

//*************************************************************************
// Defines/Enums
//*************************************************************************
typedef enum
{
    AMEC_WOF_ERROR_NONE,
    AMEC_WOF_ERROR_SCOM_1,
    AMEC_WOF_ERROR_SCOM_2,
    AMEC_WOF_ERROR_SCOM_3,
    AMEC_WOF_ERROR_SCOM_4,
    AMEC_WOF_ERROR_CORE_COUNT,
    AMEC_WOF_ERROR_UNKNOWN_STATE
} AMEC_WOF_ERROR_ENUM;

typedef enum
{
    AMEC_WOF_NO_CORE_CHANGE,
    AMEC_WOF_CORE_REQUEST_TURN_ON,
    AMEC_WOF_TRANSITION,
    AMEC_WOF_CORE_REQUEST_TURN_OFF
} AMEC_WOF_STATE_MACHINE;

//Definition of types of wof tables
typedef enum {
    AMEC_WOF_CORE_FREQ_TBL = 0x00,
    AMEC_WOF_VRM_EFF_TBL   = 0x01
} wof_tbl_type_t;

//*************************************************************************
// Structures
//*************************************************************************

//Structure used in g_amec
typedef struct amec_wof
{
    // Total loadline resistance in micro-ohm (R_ll + R_drop)
    uint16_t            loadline;
    // Vdd regulator efficiency in 0.01% units
    uint16_t            vdd_eff;
    // Chip Vdd current in 0.01 A (out of regulator)
    uint16_t            cur_out;
    // Last Vdd current accumulator to compute 2ms average
    uint32_t            cur_out_last;
    // Voltage at chip silicon (Vreg - V_loadline_droop)
    uint16_t            v_chip;
    // First index into iddq table for interpolation
    uint8_t             iddq_i;
    // Check interpolation of iddq table
    uint16_t            iddq85c;
    // I_DC_extracted is the estimated temperature-corrected leakage current in which units????
    uint16_t            iddq;
    // I_AC extracted in which units????
    uint16_t            ac;
    // Effective capacitance for TDP workload @ Turbo in which units???
    uint32_t            ceff_tdp;
    // Effective capacitance right now.
    uint32_t            ceff;
    // Effective capacitance old.
    uint32_t            ceff_old;
    // Effective capacitance ratio
    uint16_t            ceff_ratio;
    // Uplift frequency adjustment
    int16_t             f_uplift;
    // Frequency vote. Lowest vote, until WOF is initialized with safe Turbo freq.
    uint16_t            f_vote;
    // Voltage set at regulator associated with wof vote
    uint16_t            vote_vreg;
    // Voltage at chip associated with wof vote at present current.
    uint16_t            vote_vchip;
    // Non-zero is a WOF error flag
    uint8_t             error;
    // The WOF algorithm can be selected (and enabled/disabled) by setting
    // g_amec->wof.enable_parm (either automatically from frequency data
    // packet or manually from the Amester parameter interface).
    // OCC will check this against the current setting (g_amec->wof.algo_type)
    // and within 250us, do initialization for the next setting and start the
    // new WOF algorithm.
    uint8_t             enable_parm;
    // Current algorithm type: 0xFF=invalid (will cause init of enable_parm
    // setting)
    uint8_t             algo_type;
    // Count number of cores on
    uint8_t             cores_on;
    // WOF state
    uint8_t             state;
    // pmstate for debugging
    uint8_t             pm_state[MAX_NUM_CORES];

} amec_wof_t;

//*************************************************************************
// Globals
//*************************************************************************

//*************************************************************************
// Function Prototypes
//*************************************************************************
void amec_wof_main(void);
void amec_wof_helper(void);
void amec_wof_init(void) INIT_SECTION;
void amec_update_wof_sensors(void);
uint8_t amec_wof_set_algorithm(const uint8_t i_algorithm);
void amec_wof_alg_v2(void);
void amec_wof_alg_v3(void);
void amec_wof_common_steps(void);
void amec_wof_helper_v2(void);
void amec_wof_helper_v3(void);
uint32_t amec_wof_compute_c_eff(void);

void amec_wof_writeToTable(wof_tbl_type_t i_tblType,
                          const uint16_t i_size,
                          const uint8_t i_clmnCount,
                          uint8_t *i_data_ptr);

void amec_wof_store_core_freq(const uint8_t i_max_good_cores,
                              const uint16_t i_size,
                              const uint8_t i_clmnCount,
                              uint8_t *i_data_ptr);

void amec_wof_store_vrm_eff( const uint16_t i_size,
                             const uint8_t i_clmnCount,
                             uint8_t *i_data_ptr);

void amec_wof_vdd_current_out(uint16_t i_power_in,
                              uint16_t i_v_set,
                              uint16_t *o_current_out,
                              uint16_t *o_v_sense);

#endif
