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
//#include <Mmsystem.h>

#include "osal.h"
#include "ethercat.h"

#define EC_TIMEOUTMON 500

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
volatile int rtcnt;
boolean inOP;
uint8 currentgroup = 0;
ec_ODlistt ODlist;
ec_OElistt OElist;
char usdo[128];
char hstr[1024];
boolean printSDO = FALSE;
boolean printMAP = FALSE;
boolean printFMMU = FALSE;

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

char* dtype2string(uint16 dtype)
{
    switch(dtype)
    {
        case ECT_BOOLEAN:
            sprintf(hstr, "BOOLEAN");
            break;
        case ECT_INTEGER8:
            sprintf(hstr, "INTEGER8");
            break;
        case ECT_INTEGER16:
            sprintf(hstr, "INTEGER16");
            break;
        case ECT_INTEGER32:
            sprintf(hstr, "INTEGER32");
            break;
        case ECT_INTEGER24:
            sprintf(hstr, "INTEGER24");
            break;
        case ECT_INTEGER64:
            sprintf(hstr, "INTEGER64");
            break;
        case ECT_UNSIGNED8:
            sprintf(hstr, "UNSIGNED8");
            break;
        case ECT_UNSIGNED16:
            sprintf(hstr, "UNSIGNED16");
            break;
        case ECT_UNSIGNED32:
            sprintf(hstr, "UNSIGNED32");
            break;
        case ECT_UNSIGNED24:
            sprintf(hstr, "UNSIGNED24");
            break;
        case ECT_UNSIGNED64:
            sprintf(hstr, "UNSIGNED64");
            break;
        case ECT_REAL32:
            sprintf(hstr, "REAL32");
            break;
        case ECT_REAL64:
            sprintf(hstr, "REAL64");
            break;
        case ECT_BIT1:
            sprintf(hstr, "BIT1");
            break;
        case ECT_BIT2:
            sprintf(hstr, "BIT2");
            break;
        case ECT_BIT3:
            sprintf(hstr, "BIT3");
            break;
        case ECT_BIT4:
            sprintf(hstr, "BIT4");
            break;
        case ECT_BIT5:
            sprintf(hstr, "BIT5");
            break;
        case ECT_BIT6:
            sprintf(hstr, "BIT6");
            break;
        case ECT_BIT7:
            sprintf(hstr, "BIT7");
            break;
        case ECT_BIT8:
            sprintf(hstr, "BIT8");
            break;
        case ECT_VISIBLE_STRING:
            sprintf(hstr, "VISIBLE_STRING");
            break;
        case ECT_OCTET_STRING:
            sprintf(hstr, "OCTET_STRING");
            break;
        default:
            sprintf(hstr, "Type 0x%4.4X", dtype);
    }
    return hstr;
}

char* SDO2string(uint16 slave, uint16 index, uint8 subidx, uint16 dtype)
{
   int l = sizeof(usdo) - 1, i;
   uint8 *u8;
   int8 *i8;
   uint16 *u16;
   int16 *i16;
   uint32 *u32;
   int32 *i32;
   uint64 *u64;
   int64 *i64;
   float *sr;
   double *dr;
   char es[32];

   memset(&usdo, 0, 128);
   ec_SDOread(slave, index, subidx, FALSE, &l, &usdo, EC_TIMEOUTRXM);
   if (EcatError)
   {
      return ec_elist2string();
   }
   else
   {
      switch(dtype)
      {
         case ECT_BOOLEAN:
            u8 = (uint8*) &usdo[0];
            if (*u8) sprintf(hstr, "TRUE");
             else sprintf(hstr, "FALSE");
            break;
         case ECT_INTEGER8:
            i8 = (int8*) &usdo[0];
            sprintf(hstr, "0x%2.2x %d", *i8, *i8);
            break;
         case ECT_INTEGER16:
            i16 = (int16*) &usdo[0];
            sprintf(hstr, "0x%4.4x %d", *i16, *i16);
            break;
         case ECT_INTEGER32:
         case ECT_INTEGER24:
            i32 = (int32*) &usdo[0];
            sprintf(hstr, "0x%8.8x %d", *i32, *i32);
            break;
         case ECT_INTEGER64:
            i64 = (int64*) &usdo[0];
            sprintf(hstr, "0x%16.16"PRIx64" %"PRId64, *i64, *i64);
            break;
         case ECT_UNSIGNED8:
            u8 = (uint8*) &usdo[0];
            sprintf(hstr, "0x%2.2x %u", *u8, *u8);
            break;
         case ECT_UNSIGNED16:
            u16 = (uint16*) &usdo[0];
            sprintf(hstr, "0x%4.4x %u", *u16, *u16);
            break;
         case ECT_UNSIGNED32:
         case ECT_UNSIGNED24:
            u32 = (uint32*) &usdo[0];
            sprintf(hstr, "0x%8.8x %u", *u32, *u32);
            break;
         case ECT_UNSIGNED64:
            u64 = (uint64*) &usdo[0];
            sprintf(hstr, "0x%16.16"PRIx64" %"PRIu64, *u64, *u64);
            break;
         case ECT_REAL32:
            sr = (float*) &usdo[0];
            sprintf(hstr, "%f", *sr);
            break;
         case ECT_REAL64:
            dr = (double*) &usdo[0];
            sprintf(hstr, "%f", *dr);
            break;
         case ECT_BIT1:
         case ECT_BIT2:
         case ECT_BIT3:
         case ECT_BIT4:
         case ECT_BIT5:
         case ECT_BIT6:
         case ECT_BIT7:
         case ECT_BIT8:
            u8 = (uint8*) &usdo[0];
            sprintf(hstr, "0x%x", *u8);
            break;
         case ECT_VISIBLE_STRING:
            strcpy(hstr, usdo);
            break;
         case ECT_OCTET_STRING:
            hstr[0] = 0x00;
            for (i = 0 ; i < l ; i++)
            {
               sprintf(es, "0x%2.2x ", usdo[i]);
               strcat( hstr, es);
            }
            break;
         default:
            sprintf(hstr, "Unknown type");
      }
      return hstr;
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

    rdl = sizeof(rdat); rdat = 0;
    /* read PDO assign subindex 0 ( = number of PDO's) */
    wkc = ec_SDOread(slave, PDOassign, 0x00, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);
    rdat = etohs(rdat);
    /* positive result from slave ? */
    if ((wkc > 0) && (rdat > 0))
    {
        /* number of available sub indexes */
        nidx = rdat;
        bsize = 0;
        /* read all PDO's */
        for (idxloop = 1; idxloop <= nidx; idxloop++)
        {
            rdl = sizeof(rdat); rdat = 0;
            /* read PDO assign */
            wkc = ec_SDOread(slave, PDOassign, (uint8)idxloop, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);
            /* result is index of PDO */
            idx = etohl(rdat);
            if (idx > 0)
            {
                rdl = sizeof(subcnt); subcnt = 0;
                /* read number of subindexes of PDO */
                wkc = ec_SDOread(slave,idx, 0x00, FALSE, &rdl, &subcnt, EC_TIMEOUTRXM);
                subidx = subcnt;
                /* for each subindex */
                for (subidxloop = 1; subidxloop <= subidx; subidxloop++)
                {
                    rdl = sizeof(rdat2); rdat2 = 0;
                    /* read SDO that is mapped in PDO */
                    wkc = ec_SDOread(slave, idx, (uint8)subidxloop, FALSE, &rdl, &rdat2, EC_TIMEOUTRXM);
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
                    if(obj_idx || obj_subidx)
                        wkc = ec_readOEsingle(0, obj_subidx, &ODlist, &OElist);
                    printf("  [0x%4.4X.%1d] 0x%4.4X:0x%2.2X 0x%2.2X", abs_offset, abs_bit, obj_idx, obj_subidx, bitlen);
                    if((wkc > 0) && OElist.Entries)
                    {
                        printf(" %-12s %s\n", dtype2string(OElist.DataType[obj_subidx]), OElist.Name[obj_subidx]);
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

void si_sdo(int cnt)
{
   int i, j;

   ODlist.Entries = 0;
   memset(&ODlist, 0, sizeof(ODlist));
   if( ec_readODlist(cnt, &ODlist))
   {
      printf(" CoE Object Description found, %d entries.\n",ODlist.Entries);
      for( i = 0 ; i < ODlist.Entries ; i++)
      {
         ec_readODdescription(i, &ODlist);
         while(EcatError) printf("%s", ec_elist2string());
         printf(" Index: %4.4x Datatype: %4.4x Objectcode: %2.2x Name: %s\n",
               ODlist.Index[i], ODlist.DataType[i], ODlist.ObjectCode[i], ODlist.Name[i]);
         memset(&OElist, 0, sizeof(OElist));
         ec_readOE(i, &ODlist, &OElist);
         while(EcatError) printf("%s", ec_elist2string());
         for( j = 0 ; j < ODlist.MaxSub[i]+1 ; j++)
         {
               if ((OElist.DataType[j] > 0) && (OElist.BitLength[j] > 0))
               {
                  printf("  Sub: %2.2x Datatype: %4.4x Bitlength: %4.4x Obj.access: %4.4x Name: %s\n",
                     j, OElist.DataType[j], OElist.BitLength[j], OElist.ObjAccess[j], OElist.Name[j]);
                  if ((OElist.ObjAccess[j] & 0x0007))
                  {
                     printf("          Value :%s\n", SDO2string(cnt, ODlist.Index[i], j, OElist.DataType[j]));
                  }
               }
         }
      }
   }
   else
   {
      while(EcatError) printf("%s", ec_elist2string());
   }
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
    rdl = sizeof(nSM); nSM = 0;
    /* read SyncManager Communication Type object count */
    wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, 0x00, FALSE, &rdl, &nSM, EC_TIMEOUTRXM);
    /* positive result from slave ? */
    if ((wkc > 0) && (nSM > 2))
    {
        /* make nSM equal to number of defined SM */
        nSM--;
        /* limit to maximum number of SM defined, if true the slave can't be configured */
        if (nSM > EC_MAXSM)
            nSM = EC_MAXSM;
        /* iterate for every SM type defined */
        for (iSM = 2 ; iSM <= nSM ; iSM++)
        {
            rdl = sizeof(tSM); tSM = 0;
            /* read SyncManager Communication Type */
            wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, iSM + 1, FALSE, &rdl, &tSM, EC_TIMEOUTRXM);
            if (wkc > 0)
            {
                if((iSM == 2) && (tSM == 2)) // SM2 has type 2 == mailbox out, this is a bug in the slave!
                {
                    SMt_bug_add = 1; // try to correct, this works if the types are 0 1 2 3 and should be 1 2 3 4
                    printf("Activated SM type workaround, possible incorrect mapping.\n");
                }
                if(tSM)
                    tSM += SMt_bug_add; // only add if SMt > 0

                if (tSM == 3) // outputs
                {
                    /* read the assign RXPDO */
                    printf("  SM%1d outputs\n     addr b   index: sub bitl data_type    name\n", iSM);
                    Tsize = si_PDOassign(slave, ECT_SDO_PDOASSIGN + iSM, (int)(ec_slave[slave].outputs - (uint8 *)&IOmap[0]), outputs_bo );
                    outputs_bo += Tsize;
                }
                if (tSM == 4) // inputs
                {
                    /* read the assign TXPDO */
                    printf("  SM%1d inputs\n     addr b   index: sub bitl data_type    name\n", iSM);
                    Tsize = si_PDOassign(slave, ECT_SDO_PDOASSIGN + iSM, (int)(ec_slave[slave].inputs - (uint8 *)&IOmap[0]), inputs_bo );
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

int si_map_sii(int slave)
{
   int retVal = 0;
   int Tsize, outputs_bo, inputs_bo;

   printf("PDO mapping according to SII :\n");

   outputs_bo = 0;
   inputs_bo = 0;
   /* read the assign RXPDOs */
   Tsize = si_siiPDO(slave, 1, (int)(ec_slave[slave].outputs - (uint8*)&IOmap), outputs_bo );
   outputs_bo += Tsize;
   /* read the assign TXPDOs */
   Tsize = si_siiPDO(slave, 0, (int)(ec_slave[slave].inputs - (uint8*)&IOmap), inputs_bo );
   inputs_bo += Tsize;
   /* found some I/O bits ? */
   if ((outputs_bo > 0) || (inputs_bo > 0))
      retVal = 1;
   return retVal;
}


/* most basic RT thread for process data, just does IO transfer */
void CALLBACK RTthread(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1,  DWORD_PTR dw2)
{
    IOmap[0]++;

    ec_send_processdata();
    wkc = ec_receive_processdata(EC_TIMEOUTRET);
    rtcnt++;
    /* do RT control stuff here */
}

int si_siiPDO(uint16 slave, uint8 t, int mapoffset, int bitoffset)
{
    uint16 a , w, c, e, er, Size;
    uint8 eectl;
    uint16 obj_idx;
    uint8 obj_subidx;
    uint8 obj_name;
    uint8 obj_datatype;
    uint8 bitlen;
    int totalsize;
    ec_eepromPDOt eepPDO;
    ec_eepromPDOt *PDO;
    int abs_offset, abs_bit;
    char str_name[EC_MAXNAME + 1];

    eectl = ec_slave[slave].eep_pdi;
    Size = 0;
    totalsize = 0;
    PDO = &eepPDO;
    PDO->nPDO = 0;
    PDO->Length = 0;
    PDO->Index[1] = 0;
    for (c = 0 ; c < EC_MAXSM ; c++) PDO->SMbitsize[c] = 0;
    if (t > 1)
        t = 1;
    PDO->Startpos = ec_siifind(slave, ECT_SII_PDO + t);
    if (PDO->Startpos > 0)
    {
        a = PDO->Startpos;
        w = ec_siigetbyte(slave, a++);
        w += (ec_siigetbyte(slave, a++) << 8);
        PDO->Length = w;
        c = 1;
        /* traverse through all PDOs */
        do
        {
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
                if(obj_name)
                  ec_siistring(str_name, slave, obj_name);
                if (t)
                  printf("  SM%1d RXPDO 0x%4.4X %s\n", PDO->SyncM[PDO->nPDO], PDO->Index[PDO->nPDO], str_name);
                else
                  printf("  SM%1d TXPDO 0x%4.4X %s\n", PDO->SyncM[PDO->nPDO], PDO->Index[PDO->nPDO], str_name);
                printf("     addr b   index: sub bitl data_type    name\n");
                /* read all entries defined in PDO */
                for (er = 1; er <= e; er++)
                {
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

                    str_name[0] = 0;
                    if(obj_name)
                      ec_siistring(str_name, slave, obj_name);

                    printf("  [0x%4.4X.%1d] 0x%4.4X:0x%2.2X 0x%2.2X", abs_offset, abs_bit, obj_idx, obj_subidx, bitlen);
                    printf(" %-12s %s\n", dtype2string(obj_datatype), str_name);
                    bitoffset += bitlen;
                    totalsize += bitlen;
                }
                PDO->SMbitsize[ PDO->SyncM[PDO->nPDO] ] += PDO->BitSize[PDO->nPDO];
                Size += PDO->BitSize[PDO->nPDO];
                c++;
            }
            else /* PDO deactivated because SM is 0xff or > EC_MAXSM */
            {
                c += 4 * e;
                a += 8 * e;
                c++;
            }
            if (PDO->nPDO >= (EC_MAXEEPDO - 1)) c = PDO->Length; /* limit number of PDO entries in buffer */
        }
        while (c < PDO->Length);
    }
    if (eectl) ec_eeprom2pdi(slave); /* if eeprom control was previously pdi then restore */
    return totalsize;
}

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

   // free run
   // ec_dcsync0(slave, TRUE, 5 * 1000, 0);


   while(EcatError) printf("%s", ec_elist2string());

   printf("delta slave %d set, retval = %d\n", slave, retval);
   return 1;
}

int ec_slave_read()
{
   uint16 u16val = 0;
   int u16size = sizeof(u16val);
   int i8val = 0;
   int i8size = sizeof(i8val);
   int i32val = 0;
   int i32size = sizeof(i32val);
   ec_SDOread(1, 0x6041, 0, FALSE, &u16size, &u16val, EC_TIMEOUTRXM);
   printf("sdo read status word 6041: %d\n", u16val);
   ec_SDOread(1, 0x6061, 0, FALSE, &i8size, &i8val, EC_TIMEOUTRXM);
   printf("sdo read display modes of operation 6061: %d\n", i8val);

   system("pause");

   ec_SDOread(1, 0x606C, 0, FALSE, &i32size, &i32val, EC_TIMEOUTRXM);
   printf("sdo read velocity actual value 606C: %d\n", i32val);
   ec_SDOread(1, 0x606B, 0, FALSE, &i32size, &i32val, EC_TIMEOUTRXM);
   printf("sdo read velocity demand value 606B: %d\n", i32val);
   ec_SDOread(1, 0x60FF, 0, FALSE, &i32size, &i32val, EC_TIMEOUTRXM);
   printf("sdo read target velocity 60FF: %d\n", i32val);

   uint8 u8val = 0x00;
   int retval = 0;
   retval += ec_SDOwrite(1, 0x6040, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

}


void simpletest(char *ifname)
{
   int i, j, oloop, iloop, wkc_count, chk, slc, cnt, nSM;
   uint16 ssigen;
   UINT mmResult;

   needlf = FALSE;
   inOP = FALSE;
   int usedmem;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */


       if ( ec_config_init(FALSE) > 0 )
       {
            printf("%d slaves found and configured.\n",ec_slavecount);
            // ec_configdc();

            for(slc = 1; slc <= ec_slavecount; slc++)
            {
                // beckhoff EL7031, using ec_slave[].name is not very reliable
                
                    // link slave specific setup to preop->safeop hook
                    printf("ec slave name :%s \n",ec_slave[slc].name);
                    ec_slave[slc].PO2SOconfig = &DELTAsetup;
                    // int res = DELTAsetup(slc);
                    // printf("res: %d\n", res);
            }


         usedmem = ec_config_map(&IOmap);

         printf("usedmem :%d sizeofIOmap %d\n\r",usedmem,sizeof(IOmap));


         

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
            ec_statecheck(0, EC_STATE_OPERATIONAL, 1000);
         }
         while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
         if (ec_slave[0].state == EC_STATE_OPERATIONAL )
         {
            printf("Operational state reached for all slaves.\n");

            slave_out->target_vel = 0;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            slave_out->controlword = 0x0006;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            slave_out->controlword = 0x0007;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            slave_out->controlword = 0x000f;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);
            slave_out->target_vel = 0x07d0;
            ec_send_processdata();
            wkc += ec_receive_processdata(EC_TIMEOUTRET); osal_usleep(1000);

            for(i = 1; i <= 500; i++)
            {
               if(wkc >= expectedWKC)
               {
                  
                  printf("Processdata cycle %4d, WKC %d , O:", rtcnt, wkc);

                  for(j = 0 ; j < oloop; j++)
                  {
                        printf(" %2.2x", *(ec_slave[0].outputs + j));
                  }

                  printf(" I:");
                  for(j = 0 ; j < iloop; j++)
                  {
                        printf(" %2.2x", *(ec_slave[0].inputs + j));
                  }
                  printf(" T:%lld\r",ec_DCtime);
                  needlf = TRUE;
               }
               osal_usleep(50000);

            }
            inOP = FALSE;

            // wkc_count = 0;
            // inOP = TRUE;

            // slave_out->target_vel = 0;
            // slave_out->controlword = 0x0006;
            // slave_out->controlword = 0x0007;
            // slave_out->controlword = 0x000f;
            // slave_out->target_vel = 2000;
            // ec_send_processdata();
            // wkc_count += ec_receive_processdata(EC_TIMEOUTRET);

            // printf("profile velocity mode complete wkc count : %d\n", wkc_count);

            // printf("status word output ; status:%d mode:%d velocity demand:%d velocity actual:%d\n", slave_in->statusword, slave_in->mode_of_op, slave_in->vel_demand, slave_in->vel_act);
            // system("pause");
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

         if (printFMMU)
         {
            ec_readstate();
            for( cnt = 1 ; cnt <= ec_slavecount ; cnt++)
            {
               printf("\nSlave:%d\n Name:%s\n Output size: %dbits\n Input size: %dbits\n State: %d\n Delay: %d[ns]\n Has DC: %d\n",
                     cnt, ec_slave[cnt].name, ec_slave[cnt].Obits, ec_slave[cnt].Ibits,
                     ec_slave[cnt].state, ec_slave[cnt].pdelay, ec_slave[cnt].hasdc);
               if (ec_slave[cnt].hasdc) printf(" DCParentport:%d\n", ec_slave[cnt].parentport);
               printf(" Activeports:%d.%d.%d.%d\n", (ec_slave[cnt].activeports & 0x01) > 0 ,
                                          (ec_slave[cnt].activeports & 0x02) > 0 ,
                                          (ec_slave[cnt].activeports & 0x04) > 0 ,
                                          (ec_slave[cnt].activeports & 0x08) > 0 );
               printf(" Configured address: %4.4x\n", ec_slave[cnt].configadr);
               printf(" Man: %8.8x ID: %8.8x Rev: %8.8x\n", (int)ec_slave[cnt].eep_man, (int)ec_slave[cnt].eep_id, (int)ec_slave[cnt].eep_rev);
               for(nSM = 0 ; nSM < EC_MAXSM ; nSM++)
               {
                  if(ec_slave[cnt].SM[nSM].StartAddr > 0)
                     printf(" SM%1d A:%4.4x L:%4d F:%8.8x Type:%d\n",nSM, ec_slave[cnt].SM[nSM].StartAddr, ec_slave[cnt].SM[nSM].SMlength,
                           (int)ec_slave[cnt].SM[nSM].SMflags, ec_slave[cnt].SMtype[nSM]);
               }
               for(j = 0 ; j < ec_slave[cnt].FMMUunused ; j++)
               {
                  printf(" FMMU%1d Ls:%8.8x Ll:%4d Lsb:%d Leb:%d Ps:%4.4x Psb:%d Ty:%2.2x Act:%2.2x\n", j,
                        (int)ec_slave[cnt].FMMU[j].LogStart, ec_slave[cnt].FMMU[j].LogLength, ec_slave[cnt].FMMU[j].LogStartbit,
                        ec_slave[cnt].FMMU[j].LogEndbit, ec_slave[cnt].FMMU[j].PhysStart, ec_slave[cnt].FMMU[j].PhysStartBit,
                        ec_slave[cnt].FMMU[j].FMMUtype, ec_slave[cnt].FMMU[j].FMMUactive);
               }
               printf(" FMMUfunc 0:%d 1:%d 2:%d 3:%d\n",
                        ec_slave[cnt].FMMU0func, ec_slave[cnt].FMMU1func, ec_slave[cnt].FMMU2func, ec_slave[cnt].FMMU3func);
               printf(" MBX length wr: %d rd: %d MBX protocols : %2.2x\n", ec_slave[cnt].mbx_l, ec_slave[cnt].mbx_rl, ec_slave[cnt].mbx_proto);
               ssigen = ec_siifind(cnt, ECT_SII_GENERAL);
               /* SII general section */
               if (ssigen)
               {
                  ec_slave[cnt].CoEdetails = ec_siigetbyte(cnt, ssigen + 0x07);
                  ec_slave[cnt].FoEdetails = ec_siigetbyte(cnt, ssigen + 0x08);
                  ec_slave[cnt].EoEdetails = ec_siigetbyte(cnt, ssigen + 0x09);
                  ec_slave[cnt].SoEdetails = ec_siigetbyte(cnt, ssigen + 0x0a);
                  if((ec_siigetbyte(cnt, ssigen + 0x0d) & 0x02) > 0)
                  {
                     ec_slave[cnt].blockLRW = 1;
                     ec_slave[0].blockLRW++;
                  }
                  ec_slave[cnt].Ebuscurrent = ec_siigetbyte(cnt, ssigen + 0x0e);
                  ec_slave[cnt].Ebuscurrent += ec_siigetbyte(cnt, ssigen + 0x0f) << 8;
                  ec_slave[0].Ebuscurrent += ec_slave[cnt].Ebuscurrent;
               }
               printf(" CoE details: %2.2x FoE details: %2.2x EoE details: %2.2x SoE details: %2.2x\n",
                     ec_slave[cnt].CoEdetails, ec_slave[cnt].FoEdetails, ec_slave[cnt].EoEdetails, ec_slave[cnt].SoEdetails);
               printf(" Ebus current: %d[mA]\n only LRD/LWR:%d\n",
                     ec_slave[cnt].Ebuscurrent, ec_slave[cnt].blockLRW);
               if ((ec_slave[cnt].mbx_proto & ECT_MBXPROT_COE) && printSDO)
                  si_sdo(cnt);
               if (printMAP)
               {
                  if (ec_slave[cnt].mbx_proto & ECT_MBXPROT_COE)
                     si_map_sdo(cnt);
                  else
                     si_map_sii(cnt);
               }
            }
         }

         /* stop RT thread */
         timeKillEvent(mmResult);
         // ec_slave_read(); 

         printf("\nRequest init state for all slaves\n");
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
        printf("No socket connection on %s\nExcecute as root\n",ifname);
    }
}

//DWORD WINAPI ecatcheck( LPVOID lpParam )
OSAL_THREAD_FUNC ecatcheck(void *lpParam)
{
    int slave;

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
        osal_usleep(50000);
    }

    return 0;
}

char ifbuf[1024];

int main(int argc, char *argv[])
{
   ec_adaptert * adapter = NULL;
   printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

   if (argc > 1)
   {
      if ((argc > 2) && (strncmp(argv[2], "-sdo", sizeof("-sdo")) == 0)) printSDO = TRUE;
      if ((argc > 2) && (strncmp(argv[2], "-map", sizeof("-map")) == 0)) printMAP = TRUE;
      if ((argc > 2) && (strncmp(argv[2], "-fmmu", sizeof("-fmmu")) == 0)) printFMMU = TRUE;
      /* create thread to handle slave error handling in OP */
      osal_thread_create(&thread1, 128000, &ecatcheck, NULL);
      strcpy(ifbuf, argv[1]);
      /* start cyclic part */
      simpletest(ifbuf);
   }
   else
   {
      printf("Usage: simple_test ifname1\n");
   	/* Print the list */
      printf ("Available adapters\n");
      adapter = ec_find_adapters ();
      while (adapter != NULL)
      {
         printf ("Description : %s, Device to use for wpcap: %s\n", adapter->desc,adapter->name);
         adapter = adapter->next;
      }
   }

   printf("End program\n");
   return (0);
}
