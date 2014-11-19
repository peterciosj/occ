// @file amec_sensors_fw.c
// @brief AMEC FW Sensor Calculations
/**
 *      @page ChangeLogs Change Logs
 *      @section _amec_sensors_power_c amec_sensors_power.c
 *      @verbatim
 *      
 *   Flag     Def/Fea    Userid    Date      Description
 *   -------- ---------- --------  --------  --------------------------------------
 *   @th00b              thallet   02/24/2012  New file
 *   @pb00E              pbavari   03/11/2012  Added correct include file
 *   @nh001              neilhsu   05/23/2012  Add missing error log tags 
 *   @gm008  SW226989    milesg    09/30/2013  Sapphire initial support
 *   @gm030    917444    milesg    03/06/2014  Add FFDC for GPE jobs timing out
 *
 *  @endverbatim
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
//@pb00Ec - changed from common.h to occ_common.h for ODE support
#include <occ_common.h>
#include <ssx.h>         
#include <errl.h>               // Error logging
#include "sensor.h"
#include "rtls.h"
#include "occ_sys_config.h"
#include "occ_service_codes.h"  // for SSX_GENERIC_FAILURE
#include "dcom.h"
#include "proc_data.h"
#include "amec_smh.h"
#include "amec_slave_smh.h"
#include <trac.h>
#include "amec_sys.h"
#include "sensor_enum.h"
#include "amec_service_codes.h"
#include <amec_sensors_fw.h>


// Function Specification
//
// Name: amec_slv_update_smh_sensors
//
// Description: Update FW Sensors with Amec Slave Timings.
//
// Flow:              FN= None
//
// End Function Specification
void amec_slv_update_smh_sensors(int i_smh_state, uint32_t i_duration)
{
    // Update the duration in the fw timing table
    G_fw_timing.amess_state = i_smh_state;
    G_fw_timing.amess_dur   = i_duration;
}


// Function Specification
//
// Name: amec_slv_update_gpe_sensors
//
// Description: Update FW Sensors with GPE Engine Timings.  Called from
//              callback on GPE routine completion.
//
// Flow:              FN= None
//
// End Function Specification
void amec_slv_update_gpe_sensors(uint8_t i_gpe_engine)
{
    // Update the duration in the fw timing table
    G_fw_timing.gpe_dur[i_gpe_engine] = DURATION_IN_US_UNTIL_NOW_FROM(G_fw_timing.rtl_start_gpe);
}


// Function Specification
//
// Name: amec_update_fw_sensors  
//
// Description: Updates sensors related to the OCC FW Timings
//              
//
// Flow: 02/01/2012   FN= amec_update_fw_sensors
//
// Thread: RealTime Loop
//
// Task Flags: 
//
// End Function Specification
#define MAX_CONSEC_TRACE 4
void amec_update_fw_sensors(void)
{
    errlHndl_t l_err                = NULL;
    int rc                          = 0; 
    int rc2                         = 0;
    static bool l_first_call        = TRUE;
    bool l_gpe0_idle, l_gpe1_idle;       //gm030
    static int L_consec_trace_count = 0; //gm030

    // ------------------------------------------------------
    // Update OCC Firmware Sensors from last tick
    // ------------------------------------------------------
    int l_last_state = G_fw_timing.amess_state;
    // RTLtickdur    = duration of last tick's RTL ISR (max = 250us)
    sensor_update( AMECSENSOR_PTR(RTLtickdur),                     G_fw_timing.rtl_dur);
    // AMEintdur     = duration of last tick's AMEC portion of RTL ISR
    sensor_update( AMECSENSOR_PTR(AMEintdur),                      G_fw_timing.ameint_dur);
    // AMESSdurX     = duration of last tick's AMEC state
    if(l_last_state >= NUM_AMEC_SMH_STATES)
    {
        // Sanity check.  Trace this out, even though it should never happen.
        TRAC_INFO("AMEC State Invalid, Sensor Not Updated");
    }
    else
    {
        // AMESSdurX     = duration of last tick's AMEC state
        sensor_update( AMECSENSOR_ARRAY_PTR(AMESSdur0, l_last_state),  G_fw_timing.amess_dur);
    }

    // ------------------------------------------------------
    // Kick off GPE programs to track WorstCase time in GPE
    // and update the sensors.
    // ------------------------------------------------------
    if( (NULL != G_fw_timing.gpe0_timing_request)
        && (NULL != G_fw_timing.gpe1_timing_request) )
    {
        //Check if both GPE engines were able to complete the last GPE job on the queue
        //within 1 tick.
        l_gpe0_idle = async_request_is_idle(&G_fw_timing.gpe0_timing_request->request); //gm030
        l_gpe1_idle = async_request_is_idle(&G_fw_timing.gpe1_timing_request->request); //gm030
        if(l_gpe0_idle && l_gpe1_idle)
        {
            //reset the consecutive trace count
            L_consec_trace_count = 0;

            //Both GPE engines finished on time.  Now check if they were successful too.
            if( async_request_completed(&(G_fw_timing.gpe0_timing_request->request))
                && async_request_completed(&(G_fw_timing.gpe1_timing_request->request)) )
            {
                // GPEtickdur0     = duration of last tick's PORE-GPE0 duration
                sensor_update( AMECSENSOR_PTR(GPEtickdur0), G_fw_timing.gpe_dur[0]);
                // GPEtickdur1     = duration of last tick's PORE-GPE1 duration
                sensor_update( AMECSENSOR_PTR(GPEtickdur1), G_fw_timing.gpe_dur[1]);
            }
            else
            {
                //This case is expected on the first call of the function.  After that,
                //this should not happen.
                if(!l_first_call) //@gm008
                {
                    //Note: FFDC for this case is gathered by each task responsible for
                    //      a GPE job.
                    TRAC_INFO("GPE task idle but GPE task did not complete");
                }
                l_first_call = FALSE;
            }  
  
            // Update Time used to measure GPE duration.
            G_fw_timing.rtl_start_gpe = G_fw_timing.rtl_start;
      
            // Schedule the GPE Routines that will run and update the worst
            // case timings (via callback) after they complete.  These GPE
            // routines are the last GPE routines added to the queue
            // during the RTL tick.
            rc  = pore_flex_schedule(G_fw_timing.gpe0_timing_request);
            rc2 = pore_flex_schedule(G_fw_timing.gpe1_timing_request);
    
            if(rc || rc2)
            {
                /* @
                 * @errortype
                 * @moduleid    AMEC_UPDATE_FW_SENSORS
                 * @reasoncode  SSX_GENERIC_FAILURE
                 * @userdata1   return code - gpe0
                 * @userdata2   return code - gpe1
                 * @userdata4   OCC_NO_EXTENDED_RC
                 * @devdesc     Failure to schedule PORE-GPE poreFlex object for FW timing
                 *              analysis. 
                 */
                l_err = createErrl(
                    AMEC_UPDATE_FW_SENSORS,             //modId
                    SSX_GENERIC_FAILURE,                //reasoncode
                    OCC_NO_EXTENDED_RC,                 //Extended reason code
                    ERRL_SEV_INFORMATIONAL,             //Severity
                    NULL,                               //Trace Buf
                    DEFAULT_TRACE_SIZE,                 //Trace Size
                    rc,                                 //userdata1
                    rc2);                               //userdata2
  
                // commit error log
                commitErrl( &l_err );
            }
        }
        else if(L_consec_trace_count < MAX_CONSEC_TRACE) //gm030
        {
            uint64_t l_dbg1;

            // gm030
            // Reset will eventually be requested due to not having power measurement
            // data after X ticks, but add some additional FFDC to the trace that
            // will tell us what GPE job is currently executing.
            if(!l_gpe0_idle)
            {
                l_dbg1 = in64(PORE_GPE0_DBG1);
                TRAC_ERR("GPE0 programs did not complete within one tick. DBG1[0x%08x%08x]",
                          l_dbg1 >> 32, 
                          l_dbg1 & 0x00000000ffffffffull);
            }
            if(!l_gpe1_idle)
            {
                l_dbg1 = in64(PORE_GPE1_DBG1);
                TRAC_ERR("GPE1 programs did not complete within one tick. DBG1[0x%08x%08x]",
                          l_dbg1 >> 32, 
                          l_dbg1 & 0x00000000ffffffffull);
            }
            L_consec_trace_count++;
        }
    }
}


