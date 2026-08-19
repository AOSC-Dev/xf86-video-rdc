/* 
 * Copyright (C) 2009 RDC Semiconductor Co.,Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * For technical support : 
 *     <rdc_xorg@rdc.com.tw>
 */

 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "vgaHW.h"
#include "mipointer.h"
#include "micmap.h"

#include "fb.h"
#include "regionstr.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include "vbe.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"


#include "xf86fbman.h"


#ifdef HAVE_XAA
#include "xaa.h"
#endif
#include "xaarop.h"


#include "xf86Cursor.h"


#include "rdc.h"
#include "rdc_mode.h"

#include <string.h>

RRateInfo RefreshRateMap[] = { {60.0f,  FALSE, 0},
                               {50.0f,  TRUE,  1},
                               {50.0f,  FALSE, 3},
                               {56.0f,  FALSE, 4},
                               {24.0f,  FALSE, 6},
                               {70.0f,  FALSE, 7},
                               {75.0f,  FALSE, 8},
                               {80.0f,  FALSE, 9},
                               {85.0f,  FALSE, 10},
                               {90.0f,  FALSE, 11},
                               {100.0f, FALSE, 12},
                               {120.0f, FALSE, 13},
                               {72.0f,  FALSE, 14},
                               {65.0f,  FALSE, 15}};



extern void vRDCOpenKey(ScrnInfoPtr pScrn);
extern Bool bRDCRegInit(ScrnInfoPtr pScrn);

extern Bool bInitHWC(ScrnInfoPtr pScrn, RDCRecPtr pRDC);


Bool RDCSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
USHORT usGetVbeModeNum(ScrnInfoPtr pScrn, DisplayModePtr mode);
float fDifference(float Value1, float Value2);
DisplayModePtr RDCBuildModePool(ScrnInfoPtr pScrn);
Bool BTranslateIndexToRefreshRate(UCHAR ucRRateIndex, float *fRefreshRate);
char* pcConvertResolutionToString(ULONG ulResolution);
DisplayModePtr SearchDisplayModeRecPtr(DisplayModePtr pModePoolHead, CBIOS_ARGUMENTS *pCBiosArguments);

Bool
RDCSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    RDCRecPtr pRDC;
    MODE_PRIVATE *pModePrivate;
    CBIOS_ARGUMENTS *pCBiosArguments;
    USHORT usVESAMode;
    
    pRDC = RDCPTR(pScrn);
    pModePrivate = MODE_PRIVATE_PTR(mode);
    
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InternalLevel, "==Enter RDCSetMode()== \n");

    vRDCOpenKey(pScrn);
    bRDCRegInit(pScrn);
    
    pCBiosArguments = pRDC->pCBIOSExtension->pCBiosArguments;
    
    
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, " Set Display1 Refresh Rate \n");

    
    memset(pCBiosArguments, 0, sizeof(CBIOS_ARGUMENTS));
    pCBiosArguments->AX = OEMFunction;
    pCBiosArguments->BX = SetDisplay1RefreshRate;
    pCBiosArguments->CL = pModePrivate->ucRRate_ID;
    
    CInt10(pRDC->pCBIOSExtension);

    usVESAMode = usGetVbeModeNum(pScrn, mode);
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, " RDCSetMode() Set VESA Mode 0x%x== \n",usVESAMode);
    
    memset(pCBiosArguments, 0, sizeof(CBIOS_ARGUMENTS));
    pCBiosArguments->AX = VBEFunction02;
    pCBiosArguments->BX = (0x4000 | usVESAMode);
    
    CInt10(pRDC->pCBIOSExtension);
    
    
    memset(pCBiosArguments, 0, sizeof(CBIOS_ARGUMENTS));
    pCBiosArguments->AX = VBEFunction06;
    
    pCBiosArguments->BL = 0x02;
    
    
    pCBiosArguments->CX = (USHORT)((ALIGN_TO_UB_32(pScrn->displayWidth*pScrn->bitsPerPixel)) >> 3);

    
    CInt10(pRDC->pCBIOSExtension);

    

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InternalLevel, "==Exit RDCSetMode(), return true== \n");    
    return (TRUE);    
}

USHORT usGetVbeModeNum(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    RDCRecPtr pRDC;
    MODE_PRIVATE *pModePrivate;
    USHORT usVESAModeNum;
    UCHAR  ucColorDepth = (UCHAR)(pScrn->bitsPerPixel);

    pRDC = RDCPTR(pScrn);
    pModePrivate = MODE_PRIVATE_PTR(mode);
    
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Enter usGetVbeModeNum()== \n");
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InfoLevel, "==Display Width=0x%x, Height=0x%x, Color Depth=0x%x==\n",
               mode->HDisplay,mode->VDisplay,pScrn->bitsPerPixel);

    
    if (pRDC->DeviceInfo.ucNewDeviceID == TVIndex && pRDC->bEnableTVPanning)
    {
        WORD wHSize = pRDC->TVEncoderInfo[0].TVOut_HSize;
        switch (ucColorDepth)
        {
        case 8:
            if(wHSize == 640)
                usVESAModeNum = 0x101;
            else if(wHSize == 800)
                usVESAModeNum = 0x103;
            else
                usVESAModeNum = 0x105;
            break;
        case 16:
            if(wHSize == 640)
                usVESAModeNum = 0x111;
            else if(wHSize == 800)
                usVESAModeNum = 0x114;
            else
                usVESAModeNum = 0x117;
            break;
        case 32:
            if(wHSize == 640)
                usVESAModeNum = 0x112;
            else if(wHSize == 800)
                usVESAModeNum = 0x115;
            else
                usVESAModeNum = 0x118;
            break;
        }

    }else
    {
        switch (ucColorDepth)
        {
            case 8:
                usVESAModeNum = pModePrivate->Mode_ID_8bpp;
                break;

            case 16:
                usVESAModeNum = pModePrivate->Mode_ID_16bpp;
                break;

            case 32:
                usVESAModeNum = pModePrivate->Mode_ID_32bpp;
                break;
        }
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Exit usGetVbeModeNum() return VESA Mode = 0x%x==\n", usVESAModeNum);
    return usVESAModeNum;

}

DisplayModePtr RDCBuildModePool(ScrnInfoPtr pScrn)
{
    DisplayModePtr pMode = NULL, pModePoolHead = NULL, pModePoolTail = NULL;
    
    CBIOS_ARGUMENTS *pCBiosArguments;
    RDCRecPtr pRDC = RDCPTR(pScrn);
    MODE_PRIVATE *pModePrivate;
    USHORT usSerialNum = 0;
    USHORT wLCDHorSize, wLCDVerSize;
    USHORT wVESAModeHorSize, wVESAModeVerSize;
    BYTE bColorDepth; 
    BYTE bEnoughMem;
    ULONG   ulModeMemSize;
    
    pRDC->ulMaxPitch = pRDC->ulMaxHeight = 0;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Enter RDCBuildModePool()== \n");

    
    pCBiosArguments = pRDC->pCBIOSExtension->pCBiosArguments;

    do {
        xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InfoLevel, "Mode serial Num 0x%x\n",usSerialNum);

        
        memset(pCBiosArguments, 0, sizeof(CBIOS_ARGUMENTS));
        pCBiosArguments->AX = OEMFunction;
        pCBiosArguments->BX = QuerySupportedMode;
        pCBiosArguments->CX = usSerialNum++;
        
        CInt10(pRDC->pCBIOSExtension);

        wVESAModeHorSize = (USHORT)(pCBiosArguments->Edx & 0x0000FFFF);
        wVESAModeVerSize = (USHORT)(pCBiosArguments->Edx >> 16);

        
        bEnoughMem = FALSE;
        
        
        if (pScrn->bitsPerPixel == pCBiosArguments->CL)
        {
            
            
            if (wVESAModeHorSize <= 1920 && wVESAModeHorSize > pRDC->ulMaxPitch)
                pRDC->ulMaxPitch = wVESAModeHorSize;
                
            if (wVESAModeVerSize <= 1200 && wVESAModeVerSize > pRDC->ulMaxHeight)
                pRDC->ulMaxHeight = wVESAModeVerSize;

            ulModeMemSize = ALIGN_TO_UB_32(pRDC->ulMaxPitch * pCBiosArguments->CL >> 3);
            ulModeMemSize = ulModeMemSize * pRDC->ulMaxHeight;
            
            if (pRDC->AvailableFBsize > ulModeMemSize)
                bEnoughMem = TRUE;
        }

        
        if (pCBiosArguments->AX == VBEFunctionCallSuccessful)
        {
            pMode = SearchDisplayModeRecPtr(pModePoolHead, pCBiosArguments);

            if (pMode == NULL)
            {
                if (pModePoolHead != NULL)
                {
                    pModePoolTail->next = xnfcalloc(1, sizeof(DisplayModeRec));
                    pModePoolTail->next->prev = pModePoolTail;
                    pModePoolTail = pModePoolTail->next;
                }
                else
                {
                    pModePoolHead = xnfcalloc(1, sizeof(DisplayModeRec));
                    pModePoolHead->prev = NULL;
                    pModePoolTail = pModePoolHead;
                }
                
                pModePoolTail->next = NULL;

                pModePoolTail->name = pcConvertResolutionToString(pCBiosArguments->Edx);

                pModePoolTail->status = MODE_OK;
                pModePoolTail->type = M_T_BUILTIN;
                pModePoolTail->Flags = 0;

                
                pModePoolTail->PrivSize = sizeof(MODE_PRIVATE);
                pModePoolTail->Private  = xnfcalloc(1, pModePoolTail->PrivSize);
                pModePrivate = MODE_PRIVATE_PTR(pModePoolTail);

                
                pModePoolTail->Clock = pModePoolTail->SynthClock = pCBiosArguments->Edi;
                pModePoolTail->HDisplay = pModePoolTail->CrtcHDisplay = wVESAModeHorSize;
                pModePoolTail->VDisplay = pModePoolTail->CrtcVDisplay = wVESAModeVerSize;
                pModePoolTail->HTotal = (int)(pCBiosArguments->Esi & 0x0000FFFF);
                pModePoolTail->VTotal = (int)(pCBiosArguments->Esi >> 16);
 
                BTranslateIndexToRefreshRate(pCBiosArguments->CH, &(pModePoolTail->VRefresh));
                
                pModePoolTail->PrivFlags = (int)pCBiosArguments->SI;
                pModePrivate->ucRRate_ID = pCBiosArguments->CH;
            }
            else
            {
                pModePrivate = MODE_PRIVATE_PTR(pMode);
            }
            
            switch (pCBiosArguments->CL)
            {
                case 8:
                    pModePrivate->Mode_ID_8bpp = pCBiosArguments->BX;
                    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InfoLevel, "pModePrivate->Mode_ID_8bpp = 0x%x\n",pModePrivate->Mode_ID_8bpp);
                    break;
                case 16:
                    pModePrivate->Mode_ID_16bpp = pCBiosArguments->BX;
                    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InfoLevel, "pModePrivate->Mode_ID_16bpp = 0x%x\n",pModePrivate->Mode_ID_16bpp);
                    break;
                case 32:
                    pModePrivate->Mode_ID_32bpp = pCBiosArguments->BX;
                    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, InfoLevel, "pModePrivate->Mode_ID_32bpp = 0x%x\n",pModePrivate->Mode_ID_32bpp);
                    break;
            }
        }
    } while (pCBiosArguments->AX == VBEFunctionCallSuccessful);

    
        for (bColorDepth = 0; bColorDepth < 3; bColorDepth++)
        {
            
            memset(pCBiosArguments, 0, sizeof(CBIOS_ARGUMENTS));
            pCBiosArguments->AX = OEMFunction;
            pCBiosArguments->BX = QueryLCDPanelSizeMode;
            pCBiosArguments->CX = bColorDepth;
            
            CInt10(pRDC->pCBIOSExtension);
            if(pCBiosArguments->AX == VBEFunctionCallSuccessful)
            {
                pMode = SearchDisplayModeRecPtr(pModePoolHead, pCBiosArguments);

                if (pMode == NULL)
                {
                    if (pModePoolHead != NULL)
                    {
                        pModePoolTail->next = xnfcalloc(1, sizeof(DisplayModeRec));
                        pModePoolTail->next->prev = pModePoolTail;
                        pModePoolTail = pModePoolTail->next;
                    }
                    else
                    {
                        pModePoolHead = xnfcalloc(1, sizeof(DisplayModeRec));
                        pModePoolHead->prev = NULL;
                        pModePoolTail = pModePoolHead;
                    }
                    
                    pModePoolTail->next = NULL;

                    pModePoolTail->name = pcConvertResolutionToString(pCBiosArguments->Edx);

                    pModePoolTail->status = MODE_OK;
                    pModePoolTail->type = M_T_BUILTIN;
                    pModePoolTail->Flags = 0;

                    
                    pModePoolTail->PrivSize = sizeof(MODE_PRIVATE);
                    pModePoolTail->Private  = xnfcalloc(1, pModePoolTail->PrivSize);
                    pModePrivate = MODE_PRIVATE_PTR(pModePoolTail);

                    
                    pModePoolTail->Clock = pModePoolTail->SynthClock = pCBiosArguments->Edi;
                    pModePoolTail->HDisplay = pModePoolTail->CrtcHDisplay = (pCBiosArguments->Edx & 0x0000FFFF);
                    pModePoolTail->VDisplay = pModePoolTail->CrtcVDisplay = (pCBiosArguments->Edx >> 16);
                    BTranslateIndexToRefreshRate(pCBiosArguments->CH, &(pModePoolTail->VRefresh));
                    
                    pModePoolTail->PrivFlags = (int)pCBiosArguments->SI | LCD_TIMING;
                    pModePrivate->ucRRate_ID = pCBiosArguments->CH;
                }
                else
                {
                    pModePrivate = MODE_PRIVATE_PTR(pMode);
                }

                if (pModePoolTail->PrivFlags & LCD_TIMING)
                {
                    switch (pCBiosArguments->CL)
                    {
                        case 8:
                            pModePrivate->Mode_ID_8bpp  = pCBiosArguments->BX;
                            break;
                        case 16:
                            pModePrivate->Mode_ID_16bpp = pCBiosArguments->BX;
                            break;
                        case 32:
                            pModePrivate->Mode_ID_32bpp = pCBiosArguments->BX;
                            break;
                    }
                }
                
            }
        }

    
    
    
    
    
       
    /* M2012/M2015 documentation: maximum resolution is 1920x1200 and only
     * refresh rates up to 60Hz are supported. Also adapt the available
     * resolutions to the actual framebuffer memory size (the currently
     * allocated video memory minus the reserved buffers). */
    {
        int fbpp = (pScrn->bitsPerPixel + 1) / 8;
        DisplayModePtr p = pModePoolHead, pnext;
        while (p)
        {
            pnext = p->next;
            if (p->VRefresh > 60.5f ||
                p->HDisplay > 1920 || p->VDisplay > 1200 ||
                (ULONG)p->HDisplay * p->VDisplay * fbpp > pRDC->AvailableFBsize)
            {
                if (p->prev)
                    p->prev->next = p->next;
                else
                    pModePoolHead = p->next;
                if (p->next)
                    p->next->prev = p->prev;
                if (p->Private)
                    xfree(p->Private);
                xfree((void *)p->name);
                xfree(p);
            }
            p = pnext;
        }
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Exit RDCBuildModePool()== pModePoolHead = 0x%x\n", pModePoolHead);         
    return pModePoolHead;
}

Bool BTranslateIndexToRefreshRate(UCHAR ucRRateIndex, float *fRefreshRate)
{
    int i;

    for (i = 0; i < sizeof(RefreshRateMap)/sizeof(RRateInfo); i++)
    {
        if (RefreshRateMap[i].ucRRateIndex == ucRRateIndex)
        {
            *fRefreshRate = RefreshRateMap[i].fRefreshRate;
            return (TRUE);
        }
    }
    return (FALSE);
}

char* pcConvertResolutionToString(ULONG ulResolution)
{
    USHORT usHorResolution = (USHORT)(ulResolution & 0x0000FFFF);
    USHORT usVerResolution = (USHORT)(ulResolution >> 16);
    USHORT usTemp;
    int iIndex, iStringSize, i;
    char *pcResolution;
    
    pcResolution = xnfcalloc(1, 10);
    
    
    iIndex = 0;

    iStringSize = 1;
    usTemp = usHorResolution;
    while ((usTemp/10) > 0)
    {
        iStringSize++;
        usTemp /= 10;
    }

    usTemp = usHorResolution;
    for ( i = 1 ; i <= iStringSize; i++)
    {
        pcResolution[iIndex+ iStringSize - i] = (usTemp%10) + 0x30;
        usTemp /= 10;
    }
    iIndex += iStringSize;

    pcResolution[iIndex] = 'x';
    iIndex++;
    
    iStringSize = 1;
    usTemp = usVerResolution;
    while ((usTemp/10) > 0)
    {
        iStringSize++;
        usTemp /= 10;
    }

    usTemp = usVerResolution;
    for ( i = 1 ; i <= iStringSize; i++)
    {
        pcResolution[iIndex+ iStringSize - i] = (usTemp%10) + 0x30;
        usTemp /= 10;
    }
    iIndex += iStringSize;
    
    pcResolution[iIndex] = '\0';

    return pcResolution;
}

DisplayModePtr SearchDisplayModeRecPtr(DisplayModePtr pModePoolHead, CBIOS_ARGUMENTS *pCBiosArguments)
{
    DisplayModePtr pMode = pModePoolHead;
    MODE_PRIVATE *pModePrivate;
    
    xf86DrvMsgVerb(0, X_INFO, InternalLevel, "==Enter SearchDisplayModeRecPtr(CH = 0x%02X, EDX = 0x%08X, SI = 0x%X, EDI = %d)== \n",
        pCBiosArguments->CH, pCBiosArguments->Edx, pCBiosArguments->SI, pCBiosArguments->Edi);
    
    while(pMode != NULL)
    {
        pModePrivate = MODE_PRIVATE_PTR(pMode);
        
        if ((pModePrivate->ucRRate_ID == pCBiosArguments->CH) &&
            (pMode->HDisplay == (int)(pCBiosArguments->Edx & 0x0000FFFF)) &&
            (pMode->VDisplay == (int)(pCBiosArguments->Edx >>16)) &&
            ((pMode->PrivFlags & 0xFFFF) == (int)pCBiosArguments->SI) &&
            (pMode->Clock == pCBiosArguments->Edi))
        {
            xf86DrvMsgVerb(0, X_INFO, InternalLevel, "==Exit1 SearchDisplayModeRecPtr()== \n");
            return pMode;
        }

        pMode = pMode->next;
    }
    xf86DrvMsgVerb(0, X_INFO, InternalLevel, "==Exit2 SearchDisplayModeRecPtr()== \n");
    return NULL;
}

/* EDID header: 00 FF FF FF FF FF FF 00 */
static const BYTE RDCEDIDHeader[8] =
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

/* Established timing bits (EDID bytes 35-37, MSB first) and their sizes. */
static const USHORT RDCEstablishedH[] = {
    720, 720, 640, 640, 640, 640, 800, 800,
    800, 800, 832, 1024, 1024, 1024, 1024, 1280
};
static const USHORT RDCEstablishedV[] = {
    400, 400, 480, 480, 480, 480, 600, 600,
    600, 600, 624, 768, 768, 768, 768, 1024
};

static Bool RDCParseEDID(BYTE *ucEDID, USHORT *pusNativeH, USHORT *pusNativeV,
                         USHORT *pusMaxH, USHORT *pusMaxV)
{
    USHORT usH, usV, usPClock;
    int i;
    BYTE *dtd;

    *pusNativeH = *pusNativeV = *pusMaxH = *pusMaxV = 0;

    /* Detailed timing descriptors at offset 54, 18 bytes each.  The first
     * usable one is the monitor's preferred (native) mode. */
    for (i = 0; i < 4; i++)
    {
        dtd = ucEDID + 54 + i * 18;
        usPClock = dtd[0] | (dtd[1] << 8);
        if (usPClock == 0)
            continue;   /* monitor descriptor, not a timing */
        usH = (dtd[2] | ((dtd[4] & 0xF0) << 4)) + 1;
        usV = (dtd[5] | ((dtd[7] & 0xF0) << 4)) + 1;
        if (usH < 320 || usV < 240 || usH > 4096 || usV > 4096)
            continue;
        if (*pusNativeH == 0)
        {
            *pusNativeH = usH;
            *pusNativeV = usV;
        }
        if (usH > *pusMaxH) *pusMaxH = usH;
        if (usV > *pusMaxV) *pusMaxV = usV;
    }

    /* Standard timings at offset 38, 8 entries of 2 bytes. */
    for (i = 0; i < 8; i++)
    {
        BYTE b0 = ucEDID[38 + i * 2], b1 = ucEDID[39 + i * 2];

        if ((b0 == 0x01 && b1 == 0x01) || b0 == 0 || b1 == 0)
            continue;   /* unused entry */
        usH = (b0 + 31) * 8;
        switch (b1 >> 6)
        {
        case 0:  usV = usH * 10 / 16; break;   /* 16:10 */
        case 1:  usV = usH * 3 / 4;  break;    /* 4:3 */
        case 2:  usV = usH * 4 / 5;  break;    /* 5:4 */
        default: usV = usH * 9 / 16; break;    /* 16:9 */
        }
        if (usH < 320 || usV < 240)
            continue;
        if (usH > *pusMaxH) *pusMaxH = usH;
        if (usV > *pusMaxV) *pusMaxV = usV;
    }

    /* Established timings (VGA-safe modes) from bytes 35-37. */
    for (i = 0; i < 16; i++)
    {
        if (ucEDID[35 + i / 8] & (0x80 >> (i % 8)))
        {
            if (RDCEstablishedH[i] > *pusMaxH) *pusMaxH = RDCEstablishedH[i];
            if (RDCEstablishedV[i] > *pusMaxV) *pusMaxV = RDCEstablishedV[i];
        }
    }

    if (*pusMaxH == 0 || *pusMaxV == 0)
        return FALSE;

    if (*pusNativeH == 0)
    {
        *pusNativeH = *pusMaxH;
        *pusNativeV = *pusMaxV;
    }
    return TRUE;
}

Bool RDCReadEDID(ScrnInfoPtr pScrn)
{
    RDCRecPtr pRDC = RDCPTR(pScrn);
    CBIOS_ARGUMENTS *pCBiosArguments = pRDC->pCBIOSExtension->pCBiosArguments;
    BYTE ucEDID[128];
    BYTE ucI2CPort = 0, ucI2CAddr = 0;
    BYTE ucDeviceID;
    EDID_DETAILED_TIMING EDIDDetailedTimingList;
    ULONG ulChecksum = 0;
    int i, j;
    Bool bValid = FALSE;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Enter RDCReadEDID()== \n");

    pRDC->bEDIDValid = FALSE;

    /* Unlock the extended CRTC registers and put the I2C buses into their
     * default state before bit-banging the DDC lines. */
    vRDCOpenKey(pScrn);
    CBIOSInitialI2CReg();

    /* The currently active display device determines the DDC port to use. */
    memset(pCBiosArguments, 0, sizeof(CBIOS_ARGUMENTS));
    pCBiosArguments->AX = OEMFunction;
    pCBiosArguments->BX = QueryDisplayPathInfo;
    CInt10(pRDC->pCBIOSExtension);
    ucDeviceID = (pCBiosArguments->Ebx & 0x000F0000) >> 16;

    /* Try the active device's DDC port first, then the CRT and HDMI/DVI
     * ports, so EDID works even when the vbe module is not available. */
    for (i = 0; i < 3 && !bValid; i++)
    {
        switch (i)
        {
        case 0:
            if (ucDeviceID != CRTIndex && ucDeviceID != HDMIIndex &&
                ucDeviceID != DVIIndex && ucDeviceID != HDTVIndex)
                continue;   /* LCD/TV path has no DDC */
            CBIOSGetDeviceI2CInformation(ucDeviceID, &ucI2CPort, &ucI2CAddr);
            break;
        case 1:
            CBIOSGetDeviceI2CInformation(CRTIndex, &ucI2CPort, &ucI2CAddr);
            break;
        default:
            CBIOSGetDeviceI2CInformation(HDMIIndex, &ucI2CPort, &ucI2CAddr);
            break;
        }
        if (!ucI2CPort)
            continue;

        memset(ucEDID, 0, sizeof(ucEDID));
        for (j = 0; j < 128; j++)
        {
            if (CBIOSReadI2C(ucI2CPort, MonitorEDID, (BYTE)j, &ucEDID[j]) != CBIOSI2C_OK)
                break;
        }
        if (j < 128)
            continue;

        ulChecksum = 0;
        for (j = 0; j < 128; j++)
            ulChecksum += ucEDID[j];
        if ((ulChecksum & 0xFF) != 0)
            continue;

        if (memcmp(ucEDID, RDCEDIDHeader, sizeof(RDCEDIDHeader)) != 0)
            continue;

        bValid = TRUE;
    }

    if (!bValid)
    {
        xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel,
                       "==Exit1 RDCReadEDID()== no EDID found== \n");
        return FALSE;
    }

    if (!RDCParseEDID(ucEDID, &pRDC->usEDIDNativeH, &pRDC->usEDIDNativeV,
                      &pRDC->usEDIDMaxH, &pRDC->usEDIDMaxV))
    {
        xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel,
                       "==Exit2 RDCReadEDID()== EDID has no usable timings== \n");
        return FALSE;
    }

    /* Feed the preferred timing into the emulated VBIOS EDID table, exactly
     * like the vbe-based DDC path in RDCDoDDC() does.  When bEDIDValid is
     * set, mode switching programs the exact native timing from the DTD, and
     * the CRT path uses wCRTDefaultH/V as its scaling target. */
    memset(&EDIDDetailedTimingList, 0, sizeof(EDIDDetailedTimingList));
    CreateEDIDDetailedTimingList(ucEDID, sizeof(ucEDID), &EDIDDetailedTimingList);
    if (EDIDDetailedTimingList.bValid)
    {
        CBIOS_SetEDIDToModeTable(pScrn, &EDIDDetailedTimingList);
        pRDC->pCBIOSExtension->wCRTDefaultH = EDIDDetailedTimingList.usHorDispEnd;
        pRDC->pCBIOSExtension->wCRTDefaultV = EDIDDetailedTimingList.usVerDispEnd;
    }

    pRDC->pCBIOSExtension->bEDIDValid = TRUE;
    pRDC->bEDIDValid = TRUE;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, ErrorLevel,
        "RDCReadEDID: native resolution %dx%d, max %dx%d (device 0x%x, DDC port 0x%x)\n",
        pRDC->usEDIDNativeH, pRDC->usEDIDNativeV,
        pRDC->usEDIDMaxH, pRDC->usEDIDMaxV, ucDeviceID, ucI2CPort);
    return TRUE;
}

static DisplayModePtr RDCFindMode(ScrnInfoPtr pScrn, int H, int V)
{
    DisplayModePtr p = pScrn->modes;

    if (!p)
        return NULL;
    do {
        if (p->HDisplay == H && p->VDisplay == V)
            return p;
        p = p->next;
    } while (p && p != pScrn->modes);
    return NULL;
}

static void RDCSetPreferredMode(ScrnInfoPtr pScrn, DisplayModePtr m)
{
    DisplayModePtr head;

    if (!m)
        return;
    m->type |= M_T_PREFERRED;
    if (m == pScrn->modes)
        return;

    head = pScrn->modes;
    /* unlink m from the circular list */
    m->prev->next = m->next;
    m->next->prev = m->prev;
    /* splice m in front of the head */
    m->prev = head->prev;
    m->next = head;
    head->prev->next = m;
    head->prev = m;
    pScrn->modes = m;
}

static void RDCPruneModes(ScrnInfoPtr pScrn, int maxH, int maxV)
{
    DisplayModePtr p, pnext;
    int n = 0, orig, i;

    for (p = pScrn->modes; p; p = p->next)
    {
        n++;
        if (p->next == pScrn->modes)
            break;
    }
    if (n == 0)
        return;
    orig = n;

    p = pScrn->modes;
    for (i = 0; i < orig && p; i++)
    {
        pnext = p->next;
        if ((p->HDisplay > maxH || p->VDisplay > maxV) && n > 1)
        {
            xf86DrvMsgVerb(pScrn->scrnIndex, X_PROBED, InfoLevel,
                           "Removing mode \"%s\" (larger than monitor's %dx%d)\n",
                           p->name ? p->name : "", maxH, maxV);
            if (pScrn->modes == p)
                pScrn->modes = pnext;
            p->prev->next = p->next;
            p->next->prev = p->prev;
            if (p->Private)
                xfree(p->Private);
            xfree((void *)p->name);
            xfree(p);
            n--;
        }
        p = pnext;
    }
}

void RDCSelectInitialMode(ScrnInfoPtr pScrn)
{
    RDCRecPtr pRDC = RDCPTR(pScrn);
    DisplayModePtr m = NULL, p;
    char *s = NULL;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Enter RDCSelectInitialMode()== \n");

    if (!pScrn->modes)
        return;

    /* 1. Option "DefaultMode" pins the initial/preferred mode explicitly. */
    s = (char *)xf86GetOptValString(pRDC->Options, OPTION_DEFAULT_MODE);
    if (s)
    {
        for (p = pScrn->modes; p; p = p->next)
        {
            if (p->name && !strcmp(p->name, s))
            {
                m = p;
                break;
            }
            if (p->next == pScrn->modes)
                break;
        }
        if (m)
        {
            xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel,
                "RDCSelectInitialMode: using configured default mode \"%s\"\n", s);
            RDCSetPreferredMode(pScrn, m);
            goto exit;
        }
        xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel,
            "RDCSelectInitialMode: configured default mode \"%s\" not available, ignoring\n", s);
    }

    if (pRDC->bEDIDValid)
    {
        /* 2. EDID is available: start at the monitor's native resolution and
         *    drop modes it cannot physically display. */
        m = RDCFindMode(pScrn, pRDC->usEDIDNativeH, pRDC->usEDIDNativeV);
        if (!m)
            m = RDCFindMode(pScrn, pRDC->usEDIDMaxH, pRDC->usEDIDMaxV);
        if (m)
        {
            xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel,
                "RDCSelectInitialMode: using EDID native mode \"%s\"\n", m->name);
            RDCSetPreferredMode(pScrn, m);
        }
        RDCPruneModes(pScrn, pRDC->usEDIDMaxH, pRDC->usEDIDMaxV);
    }
    else
    {
        /* 3. No EDID: pick a safe resolution instead of the maximum, so the
         *    display is not driven out of range on every server start. */
        for (p = pScrn->modes; p; p = p->next)
        {
            if (p->HDisplay <= 1024 && p->VDisplay <= 768)
            {
                if (!m || (p->HDisplay * p->VDisplay > m->HDisplay * m->VDisplay))
                    m = p;
            }
            if (p->next == pScrn->modes)
                break;
        }
        if (m)
        {
            xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel,
                "RDCSelectInitialMode: no EDID, using safe default mode \"%s\"\n", m->name);
            RDCSetPreferredMode(pScrn, m);
        }
    }

exit:
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DefaultLevel, "==Exit RDCSelectInitialMode()== \n");
}
