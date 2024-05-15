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

#include "simple_test.h"

pthread_t thread1, thread2;
#define stack64k (64 * 1024)

int main(int argc, char* argv[])
{
   printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

   if (argc > 1) {
      if ((argc > 2) && (strncmp(argv[2], "-sdo", sizeof("-sdo")) == 0))
         printSDO = TRUE;
      if ((argc > 2) && (strncmp(argv[2], "-map", sizeof("-map")) == 0))
         printMAP = TRUE;

      /* create RT thread */
      osal_thread_create_rt(&thread1, stack64k * 2, &ecatthread, NULL);

      /* create thread to handle slave error handling in OP */
      osal_thread_create(&thread2, stack64k * 4, &ecatcheck, NULL);

      /* start cyclic part */
      simpletest(argv[1]);
   }
   else {
      ec_adaptert* adapter = NULL;
      printf("Usage: simple_test ifname1\nifname = eth0 for example\n");

      printf("\nAvailable adapters:\n");
      adapter = ec_find_adapters();
      while (adapter != NULL) {
         printf("    - %s  (%s)\n", adapter->name, adapter->desc);
         adapter = adapter->next;
      }
      ec_free_adapters(adapter);
   }

   printf("End program\n");
   return (0);
}
