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
#include <inttypes.h>
#include "ethercattype.h"
#include "osal.h"

char IOmap[4096];

int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;
ec_ODlistt ODlist;
ec_OElistt OElist;
boolean printSDO = FALSE;
boolean printMAP = FALSE;
char usdo[128];
#define EC_TIMEOUTMON 500

#define OTYPE_VAR 0x0007
#define OTYPE_ARRAY 0x0008
#define OTYPE_RECORD 0x0009

#define ATYPE_Rpre 0x01
#define ATYPE_Rsafe 0x02
#define ATYPE_Rop 0x04
#define ATYPE_Wpre 0x08
#define ATYPE_Wsafe 0x10
#define ATYPE_Wop 0x20

char* dtype2string(uint16 dtype, uint16 bitlen)
{
   static char str[32] = {0};

   switch (dtype) {
      case ECT_BOOLEAN:
         sprintf(str, "BOOLEAN");
         break;
      case ECT_INTEGER8:
         sprintf(str, "INTEGER8");
         break;
      case ECT_INTEGER16:
         sprintf(str, "INTEGER16");
         break;
      case ECT_INTEGER32:
         sprintf(str, "INTEGER32");
         break;
      case ECT_INTEGER24:
         sprintf(str, "INTEGER24");
         break;
      case ECT_INTEGER64:
         sprintf(str, "INTEGER64");
         break;
      case ECT_UNSIGNED8:
         sprintf(str, "UNSIGNED8");
         break;
      case ECT_UNSIGNED16:
         sprintf(str, "UNSIGNED16");
         break;
      case ECT_UNSIGNED32:
         sprintf(str, "UNSIGNED32");
         break;
      case ECT_UNSIGNED24:
         sprintf(str, "UNSIGNED24");
         break;
      case ECT_UNSIGNED64:
         sprintf(str, "UNSIGNED64");
         break;
      case ECT_REAL32:
         sprintf(str, "REAL32");
         break;
      case ECT_REAL64:
         sprintf(str, "REAL64");
         break;
      case ECT_BIT1:
         sprintf(str, "BIT1");
         break;
      case ECT_BIT2:
         sprintf(str, "BIT2");
         break;
      case ECT_BIT3:
         sprintf(str, "BIT3");
         break;
      case ECT_BIT4:
         sprintf(str, "BIT4");
         break;
      case ECT_BIT5:
         sprintf(str, "BIT5");
         break;
      case ECT_BIT6:
         sprintf(str, "BIT6");
         break;
      case ECT_BIT7:
         sprintf(str, "BIT7");
         break;
      case ECT_BIT8:
         sprintf(str, "BIT8");
         break;
      case ECT_VISIBLE_STRING:
         sprintf(str, "VISIBLE_STR(%d)", bitlen);
         break;
      case ECT_OCTET_STRING:
         sprintf(str, "OCTET_STR(%d)", bitlen);
         break;
      default:
         sprintf(str, "dt:0x%4.4X (%d)", dtype, bitlen);
   }
   return str;
}

char* otype2string(uint16 otype)
{
   static char str[32] = {0};

   switch (otype) {
      case OTYPE_VAR:
         sprintf(str, "VAR");
         break;
      case OTYPE_ARRAY:
         sprintf(str, "ARRAY");
         break;
      case OTYPE_RECORD:
         sprintf(str, "RECORD");
         break;
      default:
         sprintf(str, "ot:0x%4.4X", otype);
   }
   return str;
}

char* access2string(uint16 access)
{
   static char str[32] = {0};

   sprintf(str, "%s%s%s%s%s%s", ((access & ATYPE_Rpre) != 0 ? "R" : "_"),
           ((access & ATYPE_Wpre) != 0 ? "W" : "_"),
           ((access & ATYPE_Rsafe) != 0 ? "R" : "_"),
           ((access & ATYPE_Wsafe) != 0 ? "W" : "_"),
           ((access & ATYPE_Rop) != 0 ? "R" : "_"),
           ((access & ATYPE_Wop) != 0 ? "W" : "_"));
   return str;
}

char* SDO2string(uint16 slave, uint16 index, uint8 subidx, uint16 dtype)
{
   int l = sizeof(usdo) - 1, i;
   uint8* u8;
   int8* i8;
   uint16* u16;
   int16* i16;
   uint32* u32;
   int32* i32;
   uint64* u64;
   int64* i64;
   float* sr;
   double* dr;
   char es[32];

   memset(&usdo, 0, 128);
   ec_SDOread(slave, index, subidx, FALSE, &l, &usdo, EC_TIMEOUTRXM);
   if (EcatError) {
      return ec_elist2string();
   }
   else {
      static char str[64] = {0};
      switch (dtype) {
         case ECT_BOOLEAN:
            u8 = (uint8*)&usdo[0];
            if (*u8)
               sprintf(str, "TRUE");
            else
               sprintf(str, "FALSE");
            break;
         case ECT_INTEGER8:
            i8 = (int8*)&usdo[0];
            sprintf(str, "0x%2.2x / %d", *i8, *i8);
            break;
         case ECT_INTEGER16:
            i16 = (int16*)&usdo[0];
            sprintf(str, "0x%4.4x / %d", *i16, *i16);
            break;
         case ECT_INTEGER32:
         case ECT_INTEGER24:
            i32 = (int32*)&usdo[0];
            sprintf(str, "0x%8.8x / %d", *i32, *i32);
            break;
         case ECT_INTEGER64:
            i64 = (int64*)&usdo[0];
            sprintf(str, "0x%16.16" PRIx64 " / %" PRId64, *i64, *i64);
            break;
         case ECT_UNSIGNED8:
            u8 = (uint8*)&usdo[0];
            sprintf(str, "0x%2.2x / %u", *u8, *u8);
            break;
         case ECT_UNSIGNED16:
            u16 = (uint16*)&usdo[0];
            sprintf(str, "0x%4.4x / %u", *u16, *u16);
            break;
         case ECT_UNSIGNED32:
         case ECT_UNSIGNED24:
            u32 = (uint32*)&usdo[0];
            sprintf(str, "0x%8.8x / %u", *u32, *u32);
            break;
         case ECT_UNSIGNED64:
            u64 = (uint64*)&usdo[0];
            sprintf(str, "0x%16.16" PRIx64 " / %" PRIu64, *u64, *u64);
            break;
         case ECT_REAL32:
            sr = (float*)&usdo[0];
            sprintf(str, "%f", *sr);
            break;
         case ECT_REAL64:
            dr = (double*)&usdo[0];
            sprintf(str, "%f", *dr);
            break;
         case ECT_BIT1:
         case ECT_BIT2:
         case ECT_BIT3:
         case ECT_BIT4:
         case ECT_BIT5:
         case ECT_BIT6:
         case ECT_BIT7:
         case ECT_BIT8:
            u8 = (uint8*)&usdo[0];
            sprintf(str, "0x%x / %u", *u8, *u8);
            break;
         case ECT_VISIBLE_STRING:
            strcpy(str, "\"");
            strcat(str, usdo);
            strcat(str, "\"");
            break;
         case ECT_OCTET_STRING:
            str[0] = 0x00;
            for (i = 0; i < l; i++) {
               sprintf(es, "0x%2.2x ", usdo[i]);
               strcat(str, es);
            }
            break;
         default:
            sprintf(str, "Unknown type");
      }
      return str;
   }
}

/** Read PDO assign structure */
int si_PDOassign(uint16 slave, uint16 PDOassign, int mapoffset, int bitoffset)
{
   uint16 idxloop, nidx, subidxloop, rdat, idx, subidx;
   uint8 subcnt;
   int wkc, bsize = 0, rdl;
   int32 rdat2;
   uint8 bitlen, obj_subidx;
   uint16 obj_idx;
   int abs_offset, abs_bit;

   rdl = sizeof(rdat);
   rdat = 0;
   /* read PDO assign subindex 0 ( = number of PDO's) */
   wkc = ec_SDOread(slave, PDOassign, 0x00, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);
   rdat = etohs(rdat);
   /* positive result from slave ? */
   if ((wkc > 0) && (rdat > 0)) {
      /* number of available sub indexes */
      nidx = rdat;
      bsize = 0;
      /* read all PDO's */
      for (idxloop = 1; idxloop <= nidx; idxloop++) {
         rdl = sizeof(rdat);
         rdat = 0;
         /* read PDO assign */
         wkc = ec_SDOread(slave, PDOassign, (uint8)idxloop, FALSE, &rdl, &rdat,
                          EC_TIMEOUTRXM);
         /* result is index of PDO */
         idx = etohs(rdat);
         if (idx > 0) {
            rdl = sizeof(subcnt);
            subcnt = 0;
            /* read number of subindexes of PDO */
            wkc = ec_SDOread(slave, idx, 0x00, FALSE, &rdl, &subcnt,
                             EC_TIMEOUTRXM);
            subidx = subcnt;
            /* for each subindex */
            for (subidxloop = 1; subidxloop <= subidx; subidxloop++) {
               rdl = sizeof(rdat2);
               rdat2 = 0;
               /* read SDO that is mapped in PDO */
               wkc = ec_SDOread(slave, idx, (uint8)subidxloop, FALSE, &rdl,
                                &rdat2, EC_TIMEOUTRXM);
               rdat2 = etohl(rdat2);
               /* extract bitlength of SDO */
               bitlen = LO_BYTE(rdat2);
               bsize += bitlen;
               obj_idx = (uint16)(rdat2 >> 16);
               obj_subidx = (uint8)((rdat2 >> 8) & 0x000000ff);
               abs_offset = mapoffset + (bitoffset / 8);
               abs_bit = bitoffset % 8;
               ODlist.Slave = slave;
               ODlist.Index[0] = obj_idx;
               OElist.Entries = 0;
               wkc = 0;
               /* read object entry from dictionary if not a filler (0x0000:0x00) */
               if (obj_idx || obj_subidx)
                  wkc = ec_readOEsingle(0, obj_subidx, &ODlist, &OElist);
               printf("  [0x%4.4X.%1d] 0x%4.4X:0x%2.2X 0x%2.2X", abs_offset,
                      abs_bit, obj_idx, obj_subidx, bitlen);
               if ((wkc > 0) && OElist.Entries) {
                  printf(" %-12s %s\n",
                         dtype2string(OElist.DataType[obj_subidx], bitlen),
                         OElist.Name[obj_subidx]);
               }
               else
                  printf("\n");
               bitoffset += bitlen;
            };
         };
      };
   };
   /* return total found bitlength (PDO) */
   return bsize;
}

int si_map_sdo(int slave)
{
   int wkc, rdl;
   int retVal = 0;
   uint8 nSM, iSM, tSM;
   int Tsize, outputs_bo, inputs_bo;
   uint8 SMt_bug_add;

   printf("PDO mapping according to CoE :\n");
   SMt_bug_add = 0;
   outputs_bo = 0;
   inputs_bo = 0;
   rdl = sizeof(nSM);
   nSM = 0;
   /* read SyncManager Communication Type object count */
   wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, 0x00, FALSE, &rdl, &nSM,
                    EC_TIMEOUTRXM);
   /* positive result from slave ? */
   if ((wkc > 0) && (nSM > 2)) {
      /* make nSM equal to number of defined SM */
      nSM--;
      /* limit to maximum number of SM defined, if true the slave can't be configured */
      if (nSM > EC_MAXSM)
         nSM = EC_MAXSM;
      /* iterate for every SM type defined */
      for (iSM = 2; iSM <= nSM; iSM++) {
         rdl = sizeof(tSM);
         tSM = 0;
         /* read SyncManager Communication Type */
         wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, iSM + 1, FALSE, &rdl, &tSM,
                          EC_TIMEOUTRXM);
         if (wkc > 0) {
            if ((iSM == 2) &&
                (tSM ==
                 2))  // SM2 has type 2 == mailbox out, this is a bug in the slave!
            {
               SMt_bug_add =
                   1;  // try to correct, this works if the types are 0 1 2 3 and should be 1 2 3 4
               printf(
                   "Activated SM type workaround, possible incorrect "
                   "mapping.\n");
            }
            if (tSM)
               tSM += SMt_bug_add;  // only add if SMt > 0

            if (tSM == 3)  // outputs
            {
               /* read the assign RXPDO */
               printf(
                   "  SM%1d outputs\n     addr b   index: sub bitl data_type   "
                   " name\n",
                   iSM);
               Tsize = si_PDOassign(
                   slave, ECT_SDO_PDOASSIGN + iSM,
                   (int)(ec_slave[slave].outputs - (uint8*)&IOmap[0]),
                   outputs_bo);
               outputs_bo += Tsize;
            }
            if (tSM == 4)  // inputs
            {
               /* read the assign TXPDO */
               printf(
                   "  SM%1d inputs\n     addr b   index: sub bitl data_type    "
                   "name\n",
                   iSM);
               Tsize = si_PDOassign(
                   slave, ECT_SDO_PDOASSIGN + iSM,
                   (int)(ec_slave[slave].inputs - (uint8*)&IOmap[0]),
                   inputs_bo);
               inputs_bo += Tsize;
            }
         }
      }
   }

   /* found some I/O bits ? */
   if ((outputs_bo > 0) || (inputs_bo > 0))
      retVal = 1;
   return retVal;
}

int si_siiPDO(uint16 slave, uint8 t, int mapoffset, int bitoffset)
{
   uint16 a, w, c, e, er;
   uint8 eectl;
   uint16 obj_idx;
   uint8 obj_subidx;
   uint8 obj_name;
   uint8 obj_datatype;
   uint8 bitlen;
   int totalsize;
   ec_eepromPDOt eepPDO;
   ec_eepromPDOt* PDO;
   int abs_offset, abs_bit;
   char str_name[EC_MAXNAME + 1];

   eectl = ec_slave[slave].eep_pdi;

   totalsize = 0;
   PDO = &eepPDO;
   PDO->nPDO = 0;
   PDO->Length = 0;
   PDO->Index[1] = 0;
   for (c = 0; c < EC_MAXSM; c++)
      PDO->SMbitsize[c] = 0;
   if (t > 1)
      t = 1;
   PDO->Startpos = ec_siifind(slave, ECT_SII_PDO + t);
   if (PDO->Startpos > 0) {
      a = PDO->Startpos;
      w = ec_siigetbyte(slave, a++);
      w += (ec_siigetbyte(slave, a++) << 8);
      PDO->Length = w;
      c = 1;
      /* traverse through all PDOs */
      do {
         PDO->nPDO++;
         PDO->Index[PDO->nPDO] = ec_siigetbyte(slave, a++);
         PDO->Index[PDO->nPDO] += (ec_siigetbyte(slave, a++) << 8);
         PDO->BitSize[PDO->nPDO] = 0;
         c++;
         /* number of entries in PDO */
         e = ec_siigetbyte(slave, a++);
         PDO->SyncM[PDO->nPDO] = ec_siigetbyte(slave, a++);
         a++;
         obj_name = ec_siigetbyte(slave, a++);
         a += 2;
         c += 2;
         if (PDO->SyncM[PDO->nPDO] < EC_MAXSM) /* active and in range SM? */
         {
            str_name[0] = 0;
            if (obj_name)
               ec_siistring(str_name, slave, obj_name);
            if (t)
               printf("  SM%1d RXPDO 0x%4.4X %s\n", PDO->SyncM[PDO->nPDO],
                      PDO->Index[PDO->nPDO], str_name);
            else
               printf("  SM%1d TXPDO 0x%4.4X %s\n", PDO->SyncM[PDO->nPDO],
                      PDO->Index[PDO->nPDO], str_name);
            printf("     addr b   index: sub bitl data_type    name\n");
            /* read all entries defined in PDO */
            for (er = 1; er <= e; er++) {
               c += 4;
               obj_idx = ec_siigetbyte(slave, a++);
               obj_idx += (ec_siigetbyte(slave, a++) << 8);
               obj_subidx = ec_siigetbyte(slave, a++);
               obj_name = ec_siigetbyte(slave, a++);
               obj_datatype = ec_siigetbyte(slave, a++);
               bitlen = ec_siigetbyte(slave, a++);
               abs_offset = mapoffset + (bitoffset / 8);
               abs_bit = bitoffset % 8;

               PDO->BitSize[PDO->nPDO] += bitlen;
               a += 2;

               /* skip entry if filler (0x0000:0x00) */
               if (obj_idx || obj_subidx) {
                  str_name[0] = 0;
                  if (obj_name)
                     ec_siistring(str_name, slave, obj_name);

                  printf("  [0x%4.4X.%1d] 0x%4.4X:0x%2.2X 0x%2.2X", abs_offset,
                         abs_bit, obj_idx, obj_subidx, bitlen);
                  printf(" %-12s %s\n", dtype2string(obj_datatype, bitlen),
                         str_name);
               }
               bitoffset += bitlen;
               totalsize += bitlen;
            }
            PDO->SMbitsize[PDO->SyncM[PDO->nPDO]] += PDO->BitSize[PDO->nPDO];
            c++;
         }
         else /* PDO deactivated because SM is 0xff or > EC_MAXSM */
         {
            c += 4 * e;
            a += 8 * e;
            c++;
         }
         if (PDO->nPDO >= (EC_MAXEEPDO - 1))
            c = PDO->Length; /* limit number of PDO entries in buffer */
      } while (c < PDO->Length);
   }
   if (eectl)
      ec_eeprom2pdi(
          slave); /* if eeprom control was previously pdi then restore */
   return totalsize;
}

int si_map_sii(int slave)
{
   int retVal = 0;
   int Tsize, outputs_bo, inputs_bo;

   printf("PDO mapping according to SII :\n");

   outputs_bo = 0;
   inputs_bo = 0;
   /* read the assign RXPDOs */
   Tsize = si_siiPDO(slave, 1, (int)(ec_slave[slave].outputs - (uint8*)&IOmap),
                     outputs_bo);
   outputs_bo += Tsize;
   /* read the assign TXPDOs */
   Tsize = si_siiPDO(slave, 0, (int)(ec_slave[slave].inputs - (uint8*)&IOmap),
                     inputs_bo);
   inputs_bo += Tsize;
   /* found some I/O bits ? */
   if ((outputs_bo > 0) || (inputs_bo > 0))
      retVal = 1;
   return retVal;
}

void si_sdo(int cnt)
{
   int i, j;

   ODlist.Entries = 0;
   memset(&ODlist, 0, sizeof(ODlist));
   if (ec_readODlist(cnt, &ODlist)) {
      printf(" CoE Object Description found, %d entries.\n", ODlist.Entries);
      for (i = 0; i < ODlist.Entries; i++) {
         uint8_t max_sub;
         char name[128] = {0};

         ec_readODdescription(i, &ODlist);
         while (EcatError)
            printf(" - %s\n", ec_elist2string());
         snprintf(name, sizeof(name) - 1, "\"%s\"", ODlist.Name[i]);
         if (ODlist.ObjectCode[i] == OTYPE_VAR) {
            printf("0x%04x      %-40s      [%s]\n", ODlist.Index[i], name,
                   otype2string(ODlist.ObjectCode[i]));
         }
         else {
            printf("0x%04x      %-40s      [%s  maxsub(0x%02x / %d)]\n",
                   ODlist.Index[i], name, otype2string(ODlist.ObjectCode[i]),
                   ODlist.MaxSub[i], ODlist.MaxSub[i]);
         }
         memset(&OElist, 0, sizeof(OElist));
         ec_readOE(i, &ODlist, &OElist);
         while (EcatError)
            printf("- %s\n", ec_elist2string());

         if (ODlist.ObjectCode[i] != OTYPE_VAR) {
            int l = sizeof(max_sub);
            ec_SDOread(cnt, ODlist.Index[i], 0, FALSE, &l, &max_sub,
                       EC_TIMEOUTRXM);
         }
         else {
            max_sub = ODlist.MaxSub[i];
         }

         for (j = 0; j < max_sub + 1; j++) {
            if ((OElist.DataType[j] > 0) && (OElist.BitLength[j] > 0)) {
               snprintf(name, sizeof(name) - 1, "\"%s\"", OElist.Name[j]);
               printf("    0x%02x      %-40s      [%-16s %6s]      ", j, name,
                      dtype2string(OElist.DataType[j], OElist.BitLength[j]),
                      access2string(OElist.ObjAccess[j]));
               if ((OElist.ObjAccess[j] & 0x0007)) {
                  printf("%s", SDO2string(cnt, ODlist.Index[i], j,
                                          OElist.DataType[j]));
               }
               printf("\n");
            }
         }
      }
   }
   else {
      while (EcatError)
         printf("%s", ec_elist2string());
   }
}

int DELTAsetup(uint16 slave)
{
   int retval;
   uint8 u8val;
   uint16 u16val;
   uint32 u32val;

   retval = 0;

   u8val = 0x00;
   retval += ec_SDOwrite(slave, 0x1c12, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);
   u8val = 0x00;
   retval += ec_SDOwrite(slave, 0x1c13, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);

   //RxPDO1
   u8val = 0x00;
   retval += ec_SDOwrite(slave, 0x1600, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);

   u32val = 0x60FF0020;
   retval += ec_SDOwrite(slave, 0x1600, 01, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   u32val = 0x60400010;
   retval += ec_SDOwrite(slave, 0x1600, 02, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   u8val = 0x02;
   retval += ec_SDOwrite(slave, 0x1600, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);

   //TxPDO1
   u8val = 0x00;
   retval += ec_SDOwrite(slave, 0x1a00, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);
   u32val = 0x60410010;
   retval += ec_SDOwrite(slave, 0x1a00, 01, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   u32val = 0x60610008;
   retval += ec_SDOwrite(slave, 0x1a00, 02, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   u32val = 0x606B0020;
   retval += ec_SDOwrite(slave, 0x1a00, 03, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   u32val = 0x606C0020;
   retval += ec_SDOwrite(slave, 0x1a00, 04, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   u8val = 0x04;
   retval += ec_SDOwrite(slave, 0x1a00, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);

   u16val = 0x1600;
   retval += ec_SDOwrite(slave, 0x1c12, 01, FALSE, sizeof(u16val), &u16val,
                         EC_TIMEOUTRXM);
   u8val = 0x01;
   retval += ec_SDOwrite(slave, 0x1c12, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);
   u16val = 0x1a00;
   retval += ec_SDOwrite(slave, 0x1c13, 01, FALSE, sizeof(u16val), &u16val,
                         EC_TIMEOUTRXM);
   u8val = 0x01;
   retval += ec_SDOwrite(slave, 0x1c13, 00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);

   // set to profile velocity mode
   u8val = 0x03;
   retval += ec_SDOwrite(slave, 0x6060, 0x00, FALSE, sizeof(u8val), &u8val,
                         EC_TIMEOUTRXM);
   // set profile acceleration and deacceleration to 200
   u32val = 200;
   retval += ec_SDOwrite(slave, 0x6083, 0x00, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);
   retval += ec_SDOwrite(slave, 0x6084, 0x00, FALSE, sizeof(u32val), &u32val,
                         EC_TIMEOUTRXM);

   while (EcatError)
      printf("%s", ec_elist2string());

   printf("delta slave %d set, retval = %d\n", slave, retval);
   return 1;
}

void simpletest(char* ifname)
{
   int i, j, oloop, iloop, chk, slc, cnt, nSM;
   uint16 ssigen;
   needlf = FALSE;
   inOP = FALSE;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname)) {
      printf("ec_init on %s succeeded.\n", ifname);
      /* find and auto-config slaves */

      if (ec_config_init(FALSE) > 0) {
         printf("%d slaves found and configured.\n", ec_slavecount);

         for (slc = 1; slc <= ec_slavecount; slc++) {
            // link slave specific setup to preop->safeop hook
            printf("ec slave name :%s \n", ec_slave[slc].name);
            ec_slave[slc].PO2SOconfig = &DELTAsetup;
         }

         ec_config_map(&IOmap);

         // ec_configdc();

         printf("Slaves mapped, state to SAFE_OP.\n");
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);
         // check if enter into safe op
         printf("Slave 1 State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                ec_slave[1].state, ec_slave[1].ALstatuscode,
                ec_ALstatuscode2string(ec_slave[i].ALstatuscode));

         oloop = ec_slave[0].Obytes;
         if ((oloop == 0) && (ec_slave[0].Obits > 0))
            oloop = 1;
         if (oloop > 8)
            oloop = 8;
         iloop = ec_slave[0].Ibytes;
         if ((iloop == 0) && (ec_slave[0].Ibits > 0))
            iloop = 1;
         if (iloop > 8)
            iloop = 8;

         printf("segments : %d : %d %d %d %d\n", ec_group[0].nsegments,
                ec_group[0].IOsegment[0], ec_group[0].IOsegment[1],
                ec_group[0].IOsegment[2], ec_group[0].IOsegment[3]);

         ec_readstate();
         for (cnt = 1; cnt <= ec_slavecount; cnt++) {
            printf(
                "\nSlave:%d\n Name:%s\n Output size: %dbits\n Input size: "
                "%dbits\n State: %d\n Delay: %d[ns]\n Has DC: %d\n",
                cnt, ec_slave[cnt].name, ec_slave[cnt].Obits,
                ec_slave[cnt].Ibits, ec_slave[cnt].state, ec_slave[cnt].pdelay,
                ec_slave[cnt].hasdc);
            if (ec_slave[cnt].hasdc)
               printf(" DCParentport:%d\n", ec_slave[cnt].parentport);
            printf(" Activeports:%d.%d.%d.%d\n",
                   (ec_slave[cnt].activeports & 0x01) > 0,
                   (ec_slave[cnt].activeports & 0x02) > 0,
                   (ec_slave[cnt].activeports & 0x04) > 0,
                   (ec_slave[cnt].activeports & 0x08) > 0);
            printf(" Configured address: %4.4x\n", ec_slave[cnt].configadr);
            printf(" Man: %8.8x ID: %8.8x Rev: %8.8x\n",
                   (int)ec_slave[cnt].eep_man, (int)ec_slave[cnt].eep_id,
                   (int)ec_slave[cnt].eep_rev);
            for (nSM = 0; nSM < EC_MAXSM; nSM++) {
               if (ec_slave[cnt].SM[nSM].StartAddr > 0)
                  printf(" SM%1d A:%4.4x L:%4d F:%8.8x Type:%d\n", nSM,
                         etohs(ec_slave[cnt].SM[nSM].StartAddr),
                         etohs(ec_slave[cnt].SM[nSM].SMlength),
                         etohl(ec_slave[cnt].SM[nSM].SMflags),
                         ec_slave[cnt].SMtype[nSM]);
            }
            for (j = 0; j < ec_slave[cnt].FMMUunused; j++) {
               printf(
                   " FMMU%1d Ls:%8.8x Ll:%4d Lsb:%d Leb:%d Ps:%4.4x Psb:%d "
                   "Ty:%2.2x Act:%2.2x\n",
                   j, etohl(ec_slave[cnt].FMMU[j].LogStart),
                   etohs(ec_slave[cnt].FMMU[j].LogLength),
                   ec_slave[cnt].FMMU[j].LogStartbit,
                   ec_slave[cnt].FMMU[j].LogEndbit,
                   etohs(ec_slave[cnt].FMMU[j].PhysStart),
                   ec_slave[cnt].FMMU[j].PhysStartBit,
                   ec_slave[cnt].FMMU[j].FMMUtype,
                   ec_slave[cnt].FMMU[j].FMMUactive);
            }
            printf(" FMMUfunc 0:%d 1:%d 2:%d 3:%d\n", ec_slave[cnt].FMMU0func,
                   ec_slave[cnt].FMMU1func, ec_slave[cnt].FMMU2func,
                   ec_slave[cnt].FMMU3func);
            printf(" MBX length wr: %d rd: %d MBX protocols : %2.2x\n",
                   ec_slave[cnt].mbx_l, ec_slave[cnt].mbx_rl,
                   ec_slave[cnt].mbx_proto);
            ssigen = ec_siifind(cnt, ECT_SII_GENERAL);
            /* SII general section */
            if (ssigen) {
               ec_slave[cnt].CoEdetails = ec_siigetbyte(cnt, ssigen + 0x07);
               ec_slave[cnt].FoEdetails = ec_siigetbyte(cnt, ssigen + 0x08);
               ec_slave[cnt].EoEdetails = ec_siigetbyte(cnt, ssigen + 0x09);
               ec_slave[cnt].SoEdetails = ec_siigetbyte(cnt, ssigen + 0x0a);
               if ((ec_siigetbyte(cnt, ssigen + 0x0d) & 0x02) > 0) {
                  ec_slave[cnt].blockLRW = 1;
                  ec_slave[0].blockLRW++;
               }
               ec_slave[cnt].Ebuscurrent = ec_siigetbyte(cnt, ssigen + 0x0e);
               ec_slave[cnt].Ebuscurrent += ec_siigetbyte(cnt, ssigen + 0x0f)
                                            << 8;
               ec_slave[0].Ebuscurrent += ec_slave[cnt].Ebuscurrent;
            }
            printf(
                " CoE details: %2.2x FoE details: %2.2x EoE details: %2.2x SoE "
                "details: %2.2x\n",
                ec_slave[cnt].CoEdetails, ec_slave[cnt].FoEdetails,
                ec_slave[cnt].EoEdetails, ec_slave[cnt].SoEdetails);
            printf(" Ebus current: %d[mA]\n only LRD/LWR:%d\n",
                   ec_slave[cnt].Ebuscurrent, ec_slave[cnt].blockLRW);
            if ((ec_slave[cnt].mbx_proto & ECT_MBXPROT_COE) && printSDO)
               si_sdo(cnt);
            if (printMAP) {
               if (ec_slave[cnt].mbx_proto & ECT_MBXPROT_COE)
                  si_map_sdo(cnt);
               else
                  si_map_sii(cnt);
            }
         }

         struct PDO_out* slave_out;
         slave_out = (struct PDO_out*)(ec_slave[1].outputs);
         // slave in
         struct PDO_in* slave_in;
         slave_in = (struct PDO_in*)(ec_slave[1].inputs);

         printf("Request operational state for all slaves\n");
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* send one valid process data to make outputs in slaves happy*/
         ec_send_processdata();
         ec_receive_processdata(EC_TIMEOUTRET);
         /* request OP state for all slaves */
         ec_writestate(0);
         chk = 200;
         /* wait for all slaves to reach OP state */
         do {
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);
            ec_statecheck(0, EC_STATE_OPERATIONAL, 1000);
         } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
         ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);
         printf("ec state check and to op state");
         if (ec_slave[0].state == EC_STATE_OPERATIONAL) {
            printf("Operational state reached for all slaves.\n");
            inOP = TRUE;
            printf("Slave 1 State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                   ec_slave[1].state, ec_slave[1].ALstatuscode,
                   ec_ALstatuscode2string(ec_slave[i].ALstatuscode));

            slave_out->target_vel = 0;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET);
            osal_usleep(1000);
            slave_out->controlword = 0x0006;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET);
            osal_usleep(1000);
            slave_out->controlword = 0x0007;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET);
            osal_usleep(1000);
            slave_out->controlword = 0x000f;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET);
            osal_usleep(1000);
            slave_out->target_vel = 0x07d0;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET);
            osal_usleep(1000);

            /* cyclic loop */
            for (i = 1; i <= 5000; i++) {
               //ec_send_processdata();
               //wkc = ec_receive_processdata(EC_TIMEOUTRET);

               if (wkc >= expectedWKC) {
                  printf("Processdata cycle %4d, WKC %d , O:", i, wkc);

                  for (j = 0; j < oloop; j++) {
                     printf(" %2.2x", *(ec_slave[0].outputs + j));
                  }

                  printf(" I:");
                  for (j = 0; j < iloop; j++) {
                     printf(" %2.2x", *(ec_slave[0].inputs + j));
                  }
                  // printf("status word: %d; modes of operation: %d; vel demand value: %d; vel actual value: %d",slave_in->statusword, slave_in->mode_of_op, slave_in->vel_demand, slave_in->vel_act);
                  //printf("Slave 1 State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                  //       ec_slave[1].state, ec_slave[1].ALstatuscode,
                  //       ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                  printf(" T:%" PRId64 "\r", ec_DCtime);
                  needlf = TRUE;
               }
               osal_usleep(5000);
            }
            inOP = FALSE;
         }
         else {
            printf("Not all slaves reached operational state.\n");
            ec_readstate();
            for (i = 1; i <= ec_slavecount; i++) {
               if (ec_slave[i].state != EC_STATE_OPERATIONAL) {
                  printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n", i,
                         ec_slave[i].state, ec_slave[i].ALstatuscode,
                         ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
               }
            }
         }
         printf("\nRequest init state for all slaves\n");

         ec_slave[0].state = EC_STATE_INIT;
         /* request INIT state for all slaves */
         ec_writestate(0);
      }
      else {
         printf("No slaves found!\n");
      }
      printf("End simple test, close socket\n");

      /* stop SOEM, close socket */
      ec_close();
   }
   else {
      printf("No socket connection on %s\nExecute as root\n", ifname);
   }
}

OSAL_THREAD_FUNC_RT ecatthread(void* ptr)
{
   while (1) {
      ec_send_processdata();
      wkc += ec_receive_processdata(EC_TIMEOUTRET);
      osal_usleep(1000);
   }
}

OSAL_THREAD_FUNC ecatcheck(void* ptr)
{
   int slave;
   (void)ptr; /* Not used */

   while (1) {
      if (inOP &&
          ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate)) {
         if (needlf) {
            needlf = FALSE;
            printf("\n");
         }
         /* one ore more slaves are not responding */
         ec_group[currentgroup].docheckstate = FALSE;
         ec_readstate();
         for (slave = 1; slave <= ec_slavecount; slave++) {
            if ((ec_slave[slave].group == currentgroup) &&
                (ec_slave[slave].state != EC_STATE_OPERATIONAL)) {
               ec_group[currentgroup].docheckstate = TRUE;
               if (ec_slave[slave].state ==
                   (EC_STATE_SAFE_OP + EC_STATE_ERROR)) {
                  printf(
                      "ERROR : slave %d is in SAFE_OP + ERROR, attempting "
                      "ack.\n",
                      slave);
                  ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                  ec_writestate(slave);
               }
               else if (ec_slave[slave].state == EC_STATE_SAFE_OP) {
                  printf(
                      "WARNING : slave %d is in SAFE_OP, change to "
                      "OPERATIONAL.\n",
                      slave);
                  ec_slave[slave].state = EC_STATE_OPERATIONAL;
                  ec_writestate(slave);
               }
               else if (ec_slave[slave].state > EC_STATE_NONE) {
                  if (ec_reconfig_slave(slave, EC_TIMEOUTMON)) {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d reconfigured\n", slave);
                  }
               }
               else if (!ec_slave[slave].islost) {
                  /* re-check state */
                  ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                  if (ec_slave[slave].state == EC_STATE_NONE) {
                     ec_slave[slave].islost = TRUE;
                     printf("ERROR : slave %d lost\n", slave);
                  }
               }
            }
            if (ec_slave[slave].islost) {
               if (ec_slave[slave].state == EC_STATE_NONE) {
                  if (ec_recover_slave(slave, EC_TIMEOUTMON)) {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d recovered\n", slave);
                  }
               }
               else {
                  ec_slave[slave].islost = FALSE;
                  printf("MESSAGE : slave %d found\n", slave);
               }
            }
         }
         if (!ec_group[currentgroup].docheckstate)
            printf("OK : all slaves resumed OPERATIONAL.\n");
      }
      osal_usleep(10000);
   }
}
