/*
 * Licensed under the GNU General Public License version 2 with exceptions. See
 * LICENSE file in the project root for full license information
 */

#ifndef _simple_test_
#define _simple_test_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include "ethercat.h"
#include "osal.h"

   struct PDO_out
   {
      int32_t target_vel;
      uint16_t controlword;
   };

   struct PDO_in
   {
      uint16_t statusword;
      int8_t mode_of_op;
      int32_t vel_demand;
      int32_t vel_act;
   };

   extern char IOmap[4096];
   extern pthread_t thread1, thread2;
   extern int expectedWKC;
   extern boolean needlf;
   extern volatile int wkc;
   extern boolean inOP;
   extern uint8 currentgroup;
   extern boolean forceByteAlignment;
   extern ec_ODlistt ODlist;
   extern ec_OElistt OElist;
   extern boolean printSDO;
   extern boolean printMAP;
   extern char usdo[128];

   int DELTAsetup(uint16 slave);
   void simpletest(char* ifname);
   OSAL_THREAD_FUNC ecatcheck(void* ptr);
   OSAL_THREAD_FUNC_RT ecatthread(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
