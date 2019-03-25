/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 ************************************************************************
 * \file vlc.c
 *
 * \brief
 *    VLC support functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Lang�y               <inge.lille-langoy@telenor.com>
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Gabi Blaettermann               <blaetter@hhi.de>
 ************************************************************************
 */
#include "contributors.h"

#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "vlc.h"
#include "elements.h"
#include "header.h"


// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strncpy(sym->tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // do nothing
#endif

extern void tracebits(const char *trace_str,  int len,  int info,int value1);


int UsedBits;      // for internal statistics, is adjusted by se_v, ue_v, u_1

// Note that all NA values are filled with 0

//! for the linfo_levrun_inter routine
const byte NTAB1[4][8][2] =
{
  {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,1},{1,2},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{2,0},{1,3},{1,4},{1,5},{0,0},{0,0},{0,0},{0,0}},
  {{3,0},{2,1},{2,2},{1,6},{1,7},{1,8},{1,9},{4,0}},
};
const byte LEVRUN1[16]=
{
  4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0,
};


const byte NTAB2[4][8][2] =
{
  {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,1},{2,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,2},{3,0},{4,0},{5,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,3},{1,4},{2,1},{3,1},{6,0},{7,0},{8,0},{9,0}},
};

//! for the linfo_levrun__c2x2 routine
const byte LEVRUN3[4] =
{
  2,1,0,0
};
const byte NTAB3[2][2][2] =
{
  {{1,0},{0,0}},
  {{2,0},{1,1}},
};

/*! 
 *************************************************************************************
 * \brief
 *    ue_v, reads an ue(v) syntax element, the length in bits is stored in 
 *    the global UsedBits variable
 *
 * \param tracestring
 *    the string for the trace file
 *
 * \param bitstream
 *    the stream to be read from
 *
 * \return
 *    the value of the coded syntax element
 *
 *************************************************************************************
 */
int ue_v (char *tracestring, Bitstream *bitstream)
{
  SyntaxElement symbol, *sym=&symbol;

  assert (bitstream->streamBuffer != NULL);
  sym->type = SE_HEADER;
  sym->mapping = linfo_ue;   // Mapping rule
  SYMTRACESTRING(tracestring);
  readSyntaxElement_VLC (sym, bitstream);
  UsedBits+=sym->len;
  return sym->value1;
}


/*! 
 *************************************************************************************
 * \brief
 *    ue_v, reads an se(v) syntax element, the length in bits is stored in 
 *    the global UsedBits variable
 *
 * \param tracestring
 *    the string for the trace file
 *
 * \param bitstream
 *    the stream to be read from
 *
 * \return
 *    the value of the coded syntax element
 *
 *************************************************************************************
 */
int se_v (char *tracestring, Bitstream *bitstream)
{
  SyntaxElement symbol, *sym=&symbol;

  assert (bitstream->streamBuffer != NULL);
  sym->type = SE_HEADER;
  sym->mapping = linfo_se;   // Mapping rule: signed integer
  SYMTRACESTRING(tracestring);
  readSyntaxElement_VLC (sym, bitstream);
  UsedBits+=sym->len;
  return sym->value1;
}


/*! 
 *************************************************************************************
 * \brief
 *    ue_v, reads an u(v) syntax element, the length in bits is stored in 
 *    the global UsedBits variable
 *
 * \param tracestring
 *    the string for the trace file
 *
 * \param bitstream
 *    the stream to be read from
 *
 * \return
 *    the value of the coded syntax element
 *
 *************************************************************************************
 */
int u_v (int LenInBits, char*tracestring, Bitstream *bitstream)
{
  SyntaxElement symbol, *sym=&symbol;

  assert (bitstream->streamBuffer != NULL);
  sym->type = SE_HEADER;
  sym->mapping = linfo_ue;   // Mapping rule
  sym->len = LenInBits;
  SYMTRACESTRING(tracestring);
  readSyntaxElement_FLC (sym, bitstream);
  UsedBits+=sym->len;
  return sym->inf;
};

                
/*! 
 *************************************************************************************
 * \brief
 *    ue_v, reads an u(1) syntax element, the length in bits is stored in 
 *    the global UsedBits variable
 *
 * \param tracestring
 *    the string for the trace file
 *
 * \param bitstream
 *    the stream to be read from
 *
 * \return
 *    the value of the coded syntax element
 *
 *************************************************************************************
 */
int u_1 (char *tracestring, Bitstream *bitstream)
{
  return u_v (1, tracestring, bitstream);
}



/*!
 ************************************************************************
 * \brief
 *    mapping rule for ue(v) syntax elements
 * \par Input:
 *    lenght and info
 * \par Output:
 *    number in the code table
 ************************************************************************
 */
void linfo_ue(int len, int info, int *value1, int *dummy)
{
  *value1 = (int)pow(2,(len/2))+info-1; // *value1 = (int)(2<<(len>>1))+info-1;
}

/*!
 ************************************************************************
 * \brief
 *    mapping rule for se(v) syntax elements
 * \par Input:
 *    lenght and info
 * \par Output:
 *    signed mvd
 ************************************************************************
 */
void linfo_se(int len,  int info, int *value1, int *dummy)
{
  int n;
  n = (int)pow(2,(len/2))+info-1;
  *value1 = (n+1)/2;
  if((n & 0x01)==0)                           // lsb is signed bit
    *value1 = -*value1;
}


/*!
 ************************************************************************
 * \par Input:
 *    lenght and info
 * \par Output:
 *    cbp (intra)
 ************************************************************************
 */
void linfo_cbp_intra(int len,int info,int *cbp, int *dummy)
{
  extern const byte NCBP[48][2];
    int cbp_idx;
  linfo_ue(len,info,&cbp_idx,dummy);
    *cbp=NCBP[cbp_idx][0];
}

/*!
 ************************************************************************
 * \par Input:
 *    lenght and info
 * \par Output:
 *    cbp (inter)
 ************************************************************************
 */
void linfo_cbp_inter(int len,int info,int *cbp, int *dummy)
{
  extern const byte NCBP[48][2];
  int cbp_idx;
  linfo_ue(len,info,&cbp_idx,dummy);
    *cbp=NCBP[cbp_idx][1];
}

/*!
 ************************************************************************
 * \par Input:
 *    lenght and info
 * \par Output:
 *    level, run
 ************************************************************************
 */
void linfo_levrun_inter(int len, int info, int *level, int *irun)
{
  int l2;
  int inf;
  if (len<=9)
  {
    l2=max(0,len/2-1);
    inf=info/2;
    *level=NTAB1[l2][inf][0];
    *irun=NTAB1[l2][inf][1];
    if ((info&0x01)==1)
      *level=-*level;                   // make sign
  }
  else                                  // if len > 9, skip using the array
  {
    *irun=(info&0x1e)>>1;
    *level = LEVRUN1[*irun] + info/32 + (int)pow(2,len/2 - 5);
    if ((info&0x01)==1)
      *level=-*level;
  }
    if (len == 1) // EOB
        *level = 0;
}


/*!
 ************************************************************************
 * \par Input:
 *    lenght and info
 * \par Output:
 *    level, run
 ************************************************************************
 */
void linfo_levrun_c2x2(int len, int info, int *level, int *irun)
{
  int l2;
  int inf;

  if (len<=5)
  {
    l2=max(0,len/2-1);
    inf=info/2;
    *level=NTAB3[l2][inf][0];
    *irun=NTAB3[l2][inf][1];
    if ((info&0x01)==1)
      *level=-*level;                 // make sign
  }
  else                                  // if len > 5, skip using the array
  {
    *irun=(info&0x06)>>1;
    *level = LEVRUN3[*irun] + info/8 + (int)pow(2,len/2 - 3);
    if ((info&0x01)==1)
      *level=-*level;
  }
  if (len == 1) // EOB
    *level = 0;
}

/*!
 ************************************************************************
 * \brief
 *    read next UVLC codeword from UVLC-partition and
 *    map it to the corresponding syntax element
 ************************************************************************
 */
int readSyntaxElement_VLC(SyntaxElement *sym, Bitstream *currStream)
{
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;

  sym->len =  GetVLCSymbol (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);
  if (sym->len == -1)
    return -1;
  currStream->frame_bitoffset += sym->len;
  sym->mapping(sym->len,sym->inf,&(sym->value1),&(sym->value2));

#if TRACE
  tracebits(sym->tracestring, sym->len, sym->inf, sym->value1);
#endif

  return 1;
}


/*!
 ************************************************************************
 * \brief
 *    read next UVLC codeword from UVLC-partition and
 *    map it to the corresponding syntax element
 ************************************************************************
 */
int readSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP)
{
  Bitstream   *currStream = dP->bitstream;

  return (readSyntaxElement_VLC(sym, currStream));
}

/*!
 ************************************************************************
 * \brief
 *    read next VLC codeword for 4x4 Intra Prediction Mode and
 *    map it to the corresponding Intra Prediction Direction
 ************************************************************************
 */
int readSyntaxElement_Intra4x4PredictionMode(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP)
{
  Bitstream   *currStream            = dP->bitstream;
  int         frame_bitoffset        = currStream->frame_bitoffset;
  byte        *buf                   = currStream->streamBuffer;
  int         BitstreamLengthInBytes = currStream->bitstream_length;

  sym->len = GetVLCSymbol_IntraMode (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);

  if (sym->len == -1)
    return -1;

  currStream->frame_bitoffset += sym->len;
  sym->value1                  = sym->len == 1 ? -1 : sym->inf;

#if TRACE
  tracebits2(sym->tracestring, sym->len, sym->value1);
#endif

  return 1;
}

int GetVLCSymbol_IntraMode (byte buffer[],int totbitoffset,int *info, int bytecount)
{

  register int inf;
  long byteoffset;      // byte from start of buffer
  int bitoffset;      // bit from start of byte
  int ctr_bit=0;      // control bit for current bit posision
  int bitcounter=1;
  int len;
  int info_bit;

  byteoffset = totbitoffset/8;
  bitoffset  = 7-(totbitoffset%8);
  ctr_bit    = (buffer[byteoffset] & (0x01<<bitoffset));   // set up control bit
  len        = 1;

  //First bit
  if (ctr_bit)
  {
    *info = 0;
    return bitcounter;
  }
  else
    len=4;

  // make infoword
  inf=0;                          // shortest possible code is 1, then info is always 0
  for(info_bit=0;(info_bit<(len-1)); info_bit++)
  {
    bitcounter++;
    bitoffset-=1;
    if (bitoffset<0)
    {                 // finished with current byte ?
      bitoffset=bitoffset+8;
      byteoffset++;
    }
    if (byteoffset > bytecount)
    {
      return -1;
    }
    inf=(inf<<1);
    if(buffer[byteoffset] & (0x01<<(bitoffset)))
      inf |=1;
  }

  *info = inf;
  return bitcounter;           // return absolute offset in bit from start of frame
}


/*!
 ************************************************************************
 * \brief
 *    test if bit buffer contains only stop bit
 *
 * \param byte buffer[]
 *    containing VLC-coded data bits
 * \param int totbitoffset
 *    bit offset from start of partition
 * \param int bytecont
 *    buffer length
 ************************************************************************
 */
int more_rbsp_data (byte buffer[],int totbitoffset,int bytecount)
{

  long byteoffset;      // byte from start of buffer
  int bitoffset;      // bit from start of byte
  int ctr_bit=0;      // control bit for current bit posision

  int cnt=0;

  
  byteoffset= totbitoffset/8;
  bitoffset= 7-(totbitoffset%8);

  assert (byteoffset<bytecount);

  // there is more until we're in the last byte
  if (byteoffset<(bytecount-1)) return TRUE;

  // read one bit
  ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));
  
  // a stop bit has to be one
  if (ctr_bit==0) return TRUE;

  bitoffset--;

  while (bitoffset>=0)
  {
    ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));   // set up control bit
    if (ctr_bit>0) cnt++;
    bitoffset--;
  }

  return (0!=cnt);

}


/*!
 ************************************************************************
 * \brief
 *    Check if there are symbols for the next MB
 ************************************************************************
 */
int uvlc_startcode_follows(struct img_par *img, struct inp_par *inp, int dummy)
{
  int dp_Nr = assignSE2partition[img->currentSlice->dp_mode][SE_MBTYPE];
  DataPartition *dP = &(img->currentSlice->partArr[dp_Nr]);
  Bitstream   *currStream = dP->bitstream;
  byte *buf = currStream->streamBuffer;

  //KS: new function test for End of Buffer
  return (!(more_rbsp_data(buf, currStream->frame_bitoffset,currStream->bitstream_length)));
}



/*!
 ************************************************************************
 * \brief
 *  Moves the read pointer of the partition forward by one symbol
 *
 * \param byte buffer[]
 *    containing VLC-coded data bits
 * \param int totbitoffset
 *    bit offset from start of partition
 * \param int type
 *    expected data type (Partiotion ID)
 * \return  int info, len
 *    Length and Value of the next symbol
 *
 * \note
 *    As in both nal_bits.c and nal_part.c all data of one partition, slice,
 *    picture was already read into a buffer, there is no need to read any data
 *    here again.
 * \par
 *    GetVLCInfo was extracted because there should be only one place in the
 *    source code that has knowledge about symbol extraction, regardless of
 *    the number of different NALs.
 * \par
 *    This function could (and should) be optimized considerably
 * \par
 *    If it is ever decided to have different VLC tables for different symbol
 *    types, then this would be the place for the implementation
 * \par
 *    An alternate VLC table is implemented based on exponential Golomb codes.
 *    The encoder must have a matching define selected.
 *  
 ************************************************************************
 */
int GetVLCSymbol (byte buffer[],int totbitoffset,int *info, int bytecount)
{

  register int inf;
  long byteoffset;      // byte from start of buffer
  int bitoffset;      // bit from start of byte
  int ctr_bit=0;      // control bit for current bit posision
  int bitcounter=1;
  int len;
  int info_bit;

  byteoffset= totbitoffset/8;
  bitoffset= 7-(totbitoffset%8);
  ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));   // set up control bit

  len=1;
  while (ctr_bit==0)
  {                 // find leading 1 bit
    len++;
    bitoffset-=1;           
    bitcounter++;
    if (bitoffset<0)
    {                 // finish with current byte ?
      bitoffset=bitoffset+8;
      byteoffset++;
    }
    ctr_bit=buffer[byteoffset] & (0x01<<(bitoffset));
  }
    // make infoword
  inf=0;                          // shortest possible code is 1, then info is always 0
  for(info_bit=0;(info_bit<(len-1)); info_bit++)
  {
    bitcounter++;
    bitoffset-=1;
    if (bitoffset<0)
    {                 // finished with current byte ?
      bitoffset=bitoffset+8;
      byteoffset++;
    }
    if (byteoffset > bytecount)
    {
      return -1;
    }
    inf=(inf<<1);
    if(buffer[byteoffset] & (0x01<<(bitoffset)))
      inf |=1;
  }

  *info = inf;
  return bitcounter;           // return absolute offset in bit from start of frame
}

extern void tracebits2(const char *trace_str,  int len,  int info) ;

/*!
 ************************************************************************
 * \brief
 *    code from bitstream (2d tables)
 ************************************************************************
 */

int code_from_bitstream_2d(SyntaxElement *sym,  
                           DataPartition *dP,
                           int *lentab,
                           int *codtab,
                           int tabwidth,
                           int tabheight,
                           int *code)
{
  Bitstream   *currStream = dP->bitstream;
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;

  int i,j;
  int len, cod;

  // this VLC decoding method is not optimized for speed
  for (j = 0; j < tabheight; j++) {
    for (i = 0; i < tabwidth; i++)
    {
      len = lentab[i];
      if (!len)
        continue;
      cod = codtab[i];

      if ((ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, len) == cod))
      {
        sym->value1 = i;
        sym->value2 = j;
        currStream->frame_bitoffset += len; // move bitstream pointer
        sym->len = len;
        goto found_code;
      }
    }
    lentab += tabwidth;
    codtab += tabwidth;
  }
  
  return -1;  // failed to find code

found_code:

  *code = cod;

  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    read FLC codeword from UVLC-partition 
 ************************************************************************
 */
int readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream)
{
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;

  if ((GetBits(buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes, sym->len)) < 0)
    return -1;

  currStream->frame_bitoffset += sym->len; // move bitstream pointer
  sym->value1 = sym->inf;

#if TRACE
  tracebits2(sym->tracestring, sym->len, sym->inf);
#endif

  return 1;
}



/*!
 ************************************************************************
 * \brief
 *    read NumCoeff/TrailingOnes codeword from UVLC-partition 
 ************************************************************************
 */

int readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym,  DataPartition *dP,
                                           char *type)
{
  Bitstream   *currStream = dP->bitstream;
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;

  int vlcnum, retval;
  int code, *ct, *lt;

  int lentab[3][4][17] = 
  {
    {   // 0702
      { 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16},
      { 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16},
      { 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16},
      { 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16},
    },                                                 
    {                                                  
      { 2, 6, 6, 7, 8, 8, 9,11,11,12,12,12,13,13,13,14,14},
      { 0, 2, 5, 6, 6, 7, 8, 9,11,11,12,12,13,13,14,14,14},
      { 0, 0, 3, 6, 6, 7, 8, 9,11,11,12,12,13,13,13,14,14},
      { 0, 0, 0, 4, 4, 5, 6, 6, 7, 9,11,11,12,13,13,13,14},
    },                                                 
    {                                                  
      { 4, 6, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 9,10,10,10,10},
      { 0, 4, 5, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 9,10,10,10},
      { 0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,10},
      { 0, 0, 0, 4, 4, 4, 4, 4, 5, 6, 7, 8, 8, 9,10,10,10},
    },

  };

  int codtab[3][4][17] = 
  {
    {
      { 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7,4}, 
      { 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10,6}, 
      { 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9,5}, 
      { 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12,8},
    },
    {
      { 3,11, 7, 7, 7, 4, 7,15,11,15,11, 8,15,11, 7, 9,7}, 
      { 0, 2, 7,10, 6, 6, 6, 6,14,10,14,10,14,10,11, 8,6}, 
      { 0, 0, 3, 9, 5, 5, 5, 5,13, 9,13, 9,13, 9, 6,10,5}, 
      { 0, 0, 0, 5, 4, 6, 8, 4, 4, 4,12, 8,12,12, 8, 1,4},
    },
    {
      {15,15,11, 8,15,11, 9, 8,15,11,15,11, 8,13, 9, 5,1}, 
      { 0,14,15,12,10, 8,14,10,14,14,10,14,10, 7,12, 8,4},
      { 0, 0,13,14,11, 9,13, 9,13,10,13, 9,13, 9,11, 7,3},
      { 0, 0, 0,12,11,10, 9, 8,13,12,12,12, 8,12,10, 6,2},
    },
  };

  vlcnum = sym->value1;

  if (vlcnum == 3)
  {
    // read 6 bit FLC
    code = ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, 6);
    currStream->frame_bitoffset += 6;
    sym->value2 = code & 3;
    sym->value1 = (code >> 2);

    if (!sym->value1 && sym->value2 == 3)
    {
      // #c = 0, #t1 = 3 =>  #c = 0
      sym->value2 = 0;
    }
    else
      sym->value1++;

    sym->len = 6;

    retval = 0;
  }
  else

  {
    lt = &lentab[vlcnum][0][0];
    ct = &codtab[vlcnum][0][0];
    retval = code_from_bitstream_2d(sym, dP, lt, ct, 17, 4, &code);
  }

  if (retval)
  {
    printf("ERROR: failed to find NumCoeff/TrailingOnes\n");
    exit(-1);
  }

#if TRACE
  snprintf(sym->tracestring, 
    TRACESTRING_SIZE, "%s # c & tr.1s vlc=%d #c=%d #t1=%d",
           type, vlcnum, sym->value1, sym->value2);
  tracebits2(sym->tracestring, sym->len, code);

#endif

  return retval;
}


/*!
 ************************************************************************
 * \brief
 *    read NumCoeff/TrailingOnes codeword from UVLC-partition ChromaDC
 ************************************************************************
 */
int readSyntaxElement_NumCoeffTrailingOnesChromaDC(SyntaxElement *sym,  DataPartition *dP)
{
  int retval;
  int code, *ct, *lt;

  int lentab[4][5] = 
  {
    { 2, 6, 6, 6, 6,},          
    { 0, 1, 6, 7, 8,}, 
    { 0, 0, 3, 7, 8,}, 
    { 0, 0, 0, 6, 7,},
  };

  int codtab[4][5] = 
  {
    {1,7,4,3,2},
    {0,1,6,3,3},
    {0,0,1,2,2},
    {0,0,0,5,0},
  };



  lt = &lentab[0][0];
  ct = &codtab[0][0];

  retval = code_from_bitstream_2d(sym, dP, lt, ct, 5, 4, &code);

  if (retval)
  {
    printf("ERROR: failed to find NumCoeff/TrailingOnes ChromaDC\n");
    exit(-1);
  }


#if TRACE
    snprintf(sym->tracestring, 
      TRACESTRING_SIZE, "ChrDC # c & tr.1s  #c=%d #t1=%d",
              sym->value1, sym->value2);
    tracebits2(sym->tracestring, sym->len, code);

#endif

  return retval;
}




/*!
 ************************************************************************
 * \brief
 *    read Level VLC0 codeword from UVLC-partition 
 ************************************************************************
 */
int readSyntaxElement_Level_VLC0(SyntaxElement *sym, struct datapartition *dP)
{
  Bitstream   *currStream = dP->bitstream;
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;
  int len, sign, level, code;

  len = 0;
  while (!ShowBits(buf, frame_bitoffset+len, BitstreamLengthInBytes, 1))
    len++;

  len++;
  code = 1;
  frame_bitoffset += len;

  if (len < 15)
  {
    sign = (len - 1) & 1;
    level = (len-1) / 2 + 1;
  }
  else if (len == 15)
  {
    // escape code
    code = (code << 4) | ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, 4);
    len += 4;
    frame_bitoffset += 4;
    sign = (code & 1);
    level = ((code >> 1) & 0x7) + 8;
  }
  else if (len == 16)
  {
    // escape code
    code = (code << 12) | ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, 12);
    len += 12;
    frame_bitoffset += 12;
    sign =  (code & 1);
    level = ((code >> 1) & 0x7ff) + 16;
  }
  else
  {
    printf("ERROR reading Level code\n");
    exit(-1);
  }

  if (sign)
    level = -level;

  sym->inf = level;
  sym->len = len;

#if TRACE
  tracebits2(sym->tracestring, sym->len, code);
#endif
  currStream->frame_bitoffset = frame_bitoffset;
  return 0;

}

/*!
 ************************************************************************
 * \brief
 *    read Level VLC codeword from UVLC-partition 
 ************************************************************************
 */
int readSyntaxElement_Level_VLCN(SyntaxElement *sym, int vlc, struct datapartition *dP)  
{
  
  Bitstream   *currStream = dP->bitstream;
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;
  
  int levabs, sign;
  int len = 0;
  int code, sb;
  
  int numPrefix;
  int shift = vlc-1;
  int escape = (15<<shift)+1;
  
  // read pre zeros
  numPrefix = 0;
  while (!ShowBits(buf, frame_bitoffset+numPrefix, BitstreamLengthInBytes, 1))
    numPrefix++;
  
  
  len = numPrefix+1;
  code = 1;
  
  if (numPrefix < 15)
  {
    levabs = (numPrefix<<shift) + 1;
    
    // read (vlc-1) bits -> suffix
    if (vlc-1)
    {
      sb =  ShowBits(buf, frame_bitoffset+len, BitstreamLengthInBytes, vlc-1);
      code = (code << (vlc-1) )| sb;
      levabs += sb;
      len += (vlc-1);
    }
    
    // read 1 bit -> sign
    sign = ShowBits(buf, frame_bitoffset+len, BitstreamLengthInBytes, 1);
    code = (code << 1)| sign;
    len ++;
  }
  else  // escape
  {
    // read 11 bits -> levabs
    // levabs += escape
    sb = ShowBits(buf, frame_bitoffset+len, BitstreamLengthInBytes, 11);
    code = (code << 11 )| sb;
    
    levabs =  sb + escape;
    len+=11;
    
    // read 1 bit -> sign
    sign = ShowBits(buf, frame_bitoffset+len, BitstreamLengthInBytes, 1);
    code = (code << 1)| sign;
    len++;
  }
  
  sym->inf = (sign)?-levabs:levabs;
  sym->len = len;
  
  currStream->frame_bitoffset = frame_bitoffset+len;
  
#if TRACE
  tracebits2(sym->tracestring, sym->len, code);
#endif
  
  return 0;
}

/*!
 ************************************************************************
 * \brief
 *    read Total Zeros codeword from UVLC-partition 
 ************************************************************************
 */
int readSyntaxElement_TotalZeros(SyntaxElement *sym,  DataPartition *dP)
{
  int vlcnum, retval;
  int code, *ct, *lt;

  int lentab[TOTRUN_NUM][16] = 
  {
    
    { 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},  
    { 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},  
    { 4,3,3,3,4,4,3,3,4,5,5,6,5,6},  
    { 5,3,4,4,3,3,3,4,3,4,5,5,5},  
    { 4,4,4,3,3,3,3,3,4,5,4,5},  
    { 6,5,3,3,3,3,3,3,4,3,6},  
    { 6,5,3,3,3,2,3,4,3,6},  
    { 6,4,5,3,2,2,3,3,6},  
    { 6,6,4,2,2,3,2,5},  
    { 5,5,3,2,2,2,4},  
    { 4,4,3,3,1,3},  
    { 4,4,2,1,3},  
    { 3,3,1,2},  
    { 2,2,1},  
    { 1,1},  
  };

  int codtab[TOTRUN_NUM][16] = 
  {
    {1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
    {7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
    {5,7,6,5,4,3,4,3,2,3,2,1,1,0},
    {3,7,5,4,6,5,4,3,3,2,2,1,0},
    {5,4,3,7,6,5,4,3,2,1,1,0},
    {1,1,7,6,5,4,3,2,1,1,0},
    {1,1,5,4,3,3,2,1,1,0},
    {1,1,1,3,3,2,2,1,0},
    {1,0,1,3,2,1,1,1,},
    {1,0,1,3,2,1,1,},
    {0,1,1,2,1,3},
    {0,1,1,1,1},
    {0,1,1,1},
    {0,1,1},
    {0,1},  
  };
  vlcnum = sym->value1;

  lt = &lentab[vlcnum][0];
  ct = &codtab[vlcnum][0];

  retval = code_from_bitstream_2d(sym, dP, lt, ct, 16, 1, &code);

  if (retval)
  {
    printf("ERROR: failed to find Total Zeros\n");
    exit(-1);
  }


#if TRACE
    tracebits2(sym->tracestring, sym->len, code);

#endif

  return retval;
}    

/*!
 ************************************************************************
 * \brief
 *    read Total Zeros Chroma DC codeword from UVLC-partition 
 ************************************************************************
 */
int readSyntaxElement_TotalZerosChromaDC(SyntaxElement *sym,  DataPartition *dP)
{
  int vlcnum, retval;
  int code, *ct, *lt;

  int lentab[3][4] = 
  {
    { 1, 2, 3, 3,},
    { 1, 2, 2, 0,},
    { 1, 1, 0, 0,}, 
  };

  int codtab[3][4] = 
  {
    { 1, 1, 1, 0,},
    { 1, 1, 0, 0,},
    { 1, 0, 0, 0,},
  };

  vlcnum = sym->value1;

  lt = &lentab[vlcnum][0];
  ct = &codtab[vlcnum][0];

  retval = code_from_bitstream_2d(sym, dP, lt, ct, 4, 1, &code);

  if (retval)
  {
    printf("ERROR: failed to find Total Zeros\n");
    exit(-1);
  }


#if TRACE
    tracebits2(sym->tracestring, sym->len, code);

#endif

  return retval;
}    


/*!
 ************************************************************************
 * \brief
 *    read  Run codeword from UVLC-partition 
 ************************************************************************
 */
int readSyntaxElement_Run(SyntaxElement *sym,  DataPartition *dP)
{
  int vlcnum, retval;
  int code, *ct, *lt;

  int lentab[TOTRUN_NUM][16] = 
  {
    {1,1},
    {1,2,2},
    {2,2,2,2},
    {2,2,2,3,3},
    {2,2,3,3,3,3},
    {2,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,4,5,6,7,8,9,10,11},
  };

  int codtab[TOTRUN_NUM][16] = 
  {
    {1,0},
    {1,1,0},
    {3,2,1,0},
    {3,2,1,1,0},
    {3,2,3,2,1,0},
    {3,0,1,3,2,5,4},
    {7,6,5,4,3,2,1,1,1,1,1,1,1,1,1},
  };

  vlcnum = sym->value1;

  lt = &lentab[vlcnum][0];
  ct = &codtab[vlcnum][0];

  retval = code_from_bitstream_2d(sym, dP, lt, ct, 16, 1, &code);

  if (retval)
  {
    printf("ERROR: failed to find Run\n");
    exit(-1);
  }


#if TRACE
    tracebits2(sym->tracestring, sym->len, code);
#endif

  return retval;
}    


/*!
 ************************************************************************
 * \brief
 *  Reads bits from the bitstream buffer
 *
 * \param byte buffer[]
 *    containing VLC-coded data bits
 * \param int totbitoffset
 *    bit offset from start of partition
 * \param int bytecount
 *    total bytes in bitstream
 * \param int numbits
 *    number of bits to read
 *
 ************************************************************************
 */


int GetBits (byte buffer[],int totbitoffset,int *info, int bytecount, 
             int numbits)
{

  register int inf;
  long byteoffset;      // byte from start of buffer
  int bitoffset;      // bit from start of byte

  int bitcounter=numbits;

  byteoffset= totbitoffset/8;
  bitoffset= 7-(totbitoffset%8);

  inf=0;
  while (numbits)
  {
    inf <<=1;
    inf |= (buffer[byteoffset] & (0x01<<bitoffset))>>bitoffset;
    numbits--;
    bitoffset--;
    if (bitoffset < 0)
    {
      byteoffset++;
      bitoffset += 8;
      if (byteoffset > bytecount)
      {
        return -1;
      }
    }
  }

  *info = inf;
  return bitcounter;           // return absolute offset in bit from start of frame
}     

/*!
 ************************************************************************
 * \brief
 *  Reads bits from the bitstream buffer
 *
 * \param byte buffer[]
 *    containing VLC-coded data bits
 * \param int totbitoffset
 *    bit offset from start of partition
 * \param int bytecount
 *    total bytes in bitstream
 * \param int numbits
 *    number of bits to read
 *
 ************************************************************************
 */

int ShowBits (byte buffer[],int totbitoffset,int bytecount, int numbits)
{

  register int inf;
  long byteoffset;      // byte from start of buffer
  int bitoffset;      // bit from start of byte

  byteoffset= totbitoffset/8;
  bitoffset= 7-(totbitoffset%8);

  inf=0;
  while (numbits)
  {
    inf <<=1;
    inf |= (buffer[byteoffset] & (0x01<<bitoffset))>>bitoffset;
    numbits--;
    bitoffset--;
    if (bitoffset < 0)
    {
      byteoffset++;
      bitoffset += 8;
      if (byteoffset > bytecount)
      {
        return -1;
      }
    }
  }

  return inf;           // return absolute offset in bit from start of frame
}     


/*!
 ************************************************************************
 * \brief
 *    peek at the next 2 UVLC codeword from UVLC-partition to determine
 *    if a skipped MB is field/frame
 ************************************************************************
 */
int peekSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP)
{
  Bitstream   *currStream = dP->bitstream;
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;


/*
  sym->len =  GetVLCSymbol (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);
  if (sym->len == -1)
    return -1;
  frame_bitoffset += sym->len;
  sym->mapping(sym->len,sym->inf,&(sym->value1),&(sym->value2));

#if TRACE
  tracebits(sym->tracestring, sym->len, sym->inf, sym->value1, sym->value2); //GB
#endif
*/

  sym->len =  GetVLCSymbol (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);
  if (sym->len == -1)
    return -1;
  frame_bitoffset += sym->len;
  sym->mapping(sym->len,sym->inf,&(sym->value1),&(sym->value2));


#if TRACE
  tracebits(sym->tracestring, sym->len, sym->inf, sym->value1);
#endif

  return 1;
}

