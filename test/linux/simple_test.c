/** \file
 * \brief Example code for Simple Open EtherCAT master
 *
 * Usage : simple_test [ifname1]
 * ifname is NIC interface, f.e. eth0
 *
 * This is a minimal test.
 *
 * (c)Arthur Ketels 2010 - 2011
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ethercat.h"

#define EC_TIMEOUTMON 500

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;
boolean forceByteAlignment = FALSE;

struct PDO_out {
   int32_t target_vel;
   uint16_t controlword;
};
struct PDO_in {
   uint16_t statusword;
   int8_t mode_of_op;
   int32_t vel_demand;
   int32_t vel_act;
};

int DELTAsetup(uint16 slave)
{
   int retval;
   uint8 u8val;
   uint16 u16val;
   uint32 u32val;

   retval = 0;

   u8val = 0x00;
   retval += ec_SDOwrite(slave,0x1c12,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);
   u8val = 0x00;
   retval += ec_SDOwrite(slave,0x1c13,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);
   
   //RxPDO1
   u8val = 0x00;
   retval += ec_SDOwrite(slave,0x1600,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);
   
   u32val = 0x60FF0020;
   retval += ec_SDOwrite(slave,0x1600,01,FALSE,sizeof(u32val),&u32val,EC_TIMEOUTRXM);
   u32val = 0x60400010;
   retval += ec_SDOwrite(slave,0x1600,02,FALSE,sizeof(u32val),&u32val,EC_TIMEOUTRXM);
   u8val = 0x02;
   retval += ec_SDOwrite(slave,0x1600,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);
   
   //TxPDO1
   u8val = 0x00;
   retval += ec_SDOwrite(slave,0x1a00,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);
   u32val = 0x60410010;
   retval += ec_SDOwrite(slave,0x1a00,01,FALSE,sizeof(u32val),&u32val,EC_TIMEOUTRXM);
   u32val = 0x60610008;
   retval += ec_SDOwrite(slave,0x1a00,02,FALSE,sizeof(u32val),&u32val,EC_TIMEOUTRXM);
   u32val = 0x606B0020;
   retval += ec_SDOwrite(slave,0x1a00,03,FALSE,sizeof(u32val),&u32val,EC_TIMEOUTRXM);
   u32val = 0x606C0020;
   retval += ec_SDOwrite(slave,0x1a00,04,FALSE,sizeof(u32val),&u32val,EC_TIMEOUTRXM);
   u8val = 0x04;
   retval += ec_SDOwrite(slave,0x1a00,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);

   u16val = 0x1600;
   retval += ec_SDOwrite(slave,0x1c12,01,FALSE,sizeof(u16val),&u16val,EC_TIMEOUTRXM);
   u8val = 0x01;
   retval += ec_SDOwrite(slave,0x1c12,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);
   u16val = 0x1a00;
   retval += ec_SDOwrite(slave,0x1c13,01,FALSE,sizeof(u16val),&u16val,EC_TIMEOUTRXM);
   u8val = 0x01;
   retval += ec_SDOwrite(slave,0x1c13,00,FALSE,sizeof(u8val),&u8val,EC_TIMEOUTRXM);

   // set to profile velocity mode
   u8val = 0x03;
   retval += ec_SDOwrite(slave, 0x6060, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
   // set profile acceleration and deacceleration to 200
   u32val = 200;
   retval += ec_SDOwrite(slave, 0x6083, 0x00, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
   retval += ec_SDOwrite(slave, 0x6084, 0x00, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);

   while(EcatError) printf("%s", ec_elist2string());

   printf("delta slave %d set, retval = %d\n", slave, retval);
   return 1;
}

void simpletest(char *ifname)
{
    int i, j, oloop, iloop, chk;
    needlf = FALSE;
    inOP = FALSE;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */


       if ( ec_config_init(FALSE) > 0 )
      {
         printf("%d slaves found and configured.\n",ec_slavecount);

         for(slc = 1; slc <= ec_slavecount; slc++)
         {
            // link slave specific setup to preop->safeop hook
            printf("ec slave name :%s \n",ec_slave[slc].name);
            ec_slave[slc].PO2SOconfig = &DELTAsetup;
         }

         if (forceByteAlignment)
         {
            ec_config_map_aligned(&IOmap);
         }
         else
         {
            ec_config_map(&IOmap);
         }

         // ec_configdc();

         printf("Slaves mapped, state to SAFE_OP.\n");
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 4);

         oloop = ec_slave[0].Obytes;
         if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
         if (oloop > 8) oloop = 8;
         iloop = ec_slave[0].Ibytes;
         if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
         if (iloop > 8) iloop = 8;

         printf("segments : %d : %d %d %d %d\n",ec_group[0].nsegments ,ec_group[0].IOsegment[0],ec_group[0].IOsegment[1],ec_group[0].IOsegment[2],ec_group[0].IOsegment[3]);

         struct PDO_out *slave_out;
         slave_out = (struct PDO_out *)(ec_slave[1].outputs);
         // slave in
         struct PDO_in *slave_in;
         slave_in = (struct PDO_in *)(ec_slave[1].inputs);

         printf("Request operational state for all slaves\n");
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* send one valid process data to make outputs in slaves happy*/
         ec_send_processdata();
         ec_receive_processdata(EC_TIMEOUTRET);

         /* start RT thread as periodic MM timer */
         mmResult = timeSetEvent(1, 0, RTthread, 0, TIME_PERIODIC);

         /* request OP state for all slaves */
         ec_writestate(0);
         chk = 200;
         /* wait for all slaves to reach OP state */
         do
         {
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);
            ec_statecheck(0, EC_STATE_OPERATIONAL, 1000);
         }
         while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
         if (ec_slave[0].state == EC_STATE_OPERATIONAL )
         {
            printf("Operational state reached for all slaves.\n");
            inOP = TRUE;

            slave_out->target_vel = 0;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            // slave_out->controlword = 0x0006;
            // ec_send_processdata();
            // wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            // slave_out->controlword = 0x0007;
            // ec_send_processdata();
            // wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            // slave_out->controlword = 0x000f;
            // ec_send_processdata();
            // wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            slave_out->target_vel = 0x07d0;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);

                /* cyclic loop */
            for(i = 1; i <= 500; i++)
            {
               ec_send_processdata();
               wkc = ec_receive_processdata(EC_TIMEOUTRET);

               if(wkc >= expectedWKC)
               {
                  printf("Processdata cycle %4d, WKC %d , O:", i, wkc);

                  for(j = 0 ; j < oloop; j++)
                  {
                        printf(" %2.2x", *(ec_slave[0].outputs + j));
                  }

                  printf(" I:");
                  for(j = 0 ; j < iloop; j++)
                  {
                        printf(" %2.2x", *(ec_slave[0].inputs + j));
                  }
                  printf(" T:%"PRId64"\r",ec_DCtime);
                  needlf = TRUE;
               }
               osal_usleep(5000);

               }
               inOP = FALSE;
            }
            else
            {
                printf("Not all slaves reached operational state.\n");
                ec_readstate();
                for(i = 1; i<=ec_slavecount ; i++)
                {
                    if(ec_slave[i].state != EC_STATE_OPERATIONAL)
                    {
                        printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                            i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                    }
                }
            }
            printf("\nRequest init state for all slaves\n");

            /* stop RT thread */
            timeKillEvent(mmResult);

            ec_slave[0].state = EC_STATE_INIT;
            /* request INIT state for all slaves */
            ec_writestate(0);
        }
        else
        {
            printf("No slaves found!\n");
        }
        printf("End simple test, close socket\n");
        /* stop SOEM, close socket */
        ec_close();
    }
    else
    {
        printf("No socket connection on %s\nExecute as root\n",ifname);
    }
}

OSAL_THREAD_FUNC ecatcheck( void *ptr )
{
    int slave;
    (void)ptr;                  /* Not used */

    while(1)
    {
        if( inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
        {
            if (needlf)
            {
               needlf = FALSE;
               printf("\n");
            }
            /* one ore more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
               if ((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
               {
                  ec_group[currentgroup].docheckstate = TRUE;
                  if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                  {
                     printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                     ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state == EC_STATE_SAFE_OP)
                  {
                     printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                     ec_slave[slave].state = EC_STATE_OPERATIONAL;
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state > EC_STATE_NONE)
                  {
                     if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d reconfigured\n",slave);
                     }
                  }
                  else if(!ec_slave[slave].islost)
                  {
                     /* re-check state */
                     ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                     if (ec_slave[slave].state == EC_STATE_NONE)
                     {
                        ec_slave[slave].islost = TRUE;
                        printf("ERROR : slave %d lost\n",slave);
                     }
                  }
               }
               if (ec_slave[slave].islost)
               {
                  if(ec_slave[slave].state == EC_STATE_NONE)
                  {
                     if (ec_recover_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d recovered\n",slave);
                     }
                  }
                  else
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d found\n",slave);
                  }
               }
            }
            if(!ec_group[currentgroup].docheckstate)
               printf("OK : all slaves resumed OPERATIONAL.\n");
        }
        osal_usleep(10000);
    }
}

int main(int argc, char *argv[])
{
   printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

   if (argc > 1)
   {
      /* create thread to handle slave error handling in OP */
      osal_thread_create(&thread1, 128000, &ecatcheck, NULL);
      /* start cyclic part */
      simpletest(argv[1]);
   }
   else
   {
      ec_adaptert * adapter = NULL;
      printf("Usage: simple_test ifname1\nifname = eth0 for example\n");

      printf ("\nAvailable adapters:\n");
      adapter = ec_find_adapters ();
      while (adapter != NULL)
      {
         printf ("    - %s  (%s)\n", adapter->name, adapter->desc);
         adapter = adapter->next;
      }
      ec_free_adapters(adapter);
   }

   printf("End program\n");
   return (0);
}
