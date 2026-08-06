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



#define VIDEOMEM_SIZE_04M       0x00400000
#define VIDEOMEM_SIZE_08M       0x00800000
#define VIDEOMEM_SIZE_16M       0x01000000
#define VIDEOMEM_SIZE_32M       0x02000000
#define VIDEOMEM_SIZE_64M       0x04000000
#define VIDEOMEM_SIZE_128M      0x08000000
#define VIDEOMEM_SIZE_256M      0x10000000
#define VIDEOMEM_SIZE_512M      0x20000000

BYTE GetReg(WORD base);
void SetReg(WORD base, BYTE val);
void vSetRDCIOBase(void *base);
void GetIndexReg(WORD base, BYTE index, BYTE* val);
void SetIndexReg(WORD base, BYTE index, BYTE val);
void GetIndexRegMask(WORD base, BYTE index, BYTE mask, BYTE* val);
void SetIndexRegMask(WORD base, BYTE index, BYTE mask, BYTE val);
void vRDCOpenKey(ScrnInfoPtr pScrn);
void vSetStartAddressCRT1(RDCRecPtr pRDC, ULONG base);
ULONG RDCGetMemBandWidth(ScrnInfoPtr pScrn);
void VGA_LOAD_PALETTE_INDEX(BYTE index, BYTE red, BYTE green, BYTE blue);
Bool bRDCRegInit(ScrnInfoPtr pScrn);
ULONG GetVRAMInfo(ScrnInfoPtr pScrn);
Bool RDCFilterModeByBandWidth(ScrnInfoPtr pScrn, DisplayModePtr mode);
void vRDCLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors, VisualPtr pVisual);
void RDCDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags);
void vSetDispalyStartAddress(xf86CrtcPtr crtc, int x, int y);
CBStatus CBIOS_SetEDIDToModeTable(ScrnInfoPtr pScrn, EDID_DETAILED_TIMING *pEDIDDetailedTiming);
void CreateEDIDDetailedTimingList(UCHAR *ucEdidBuffer, ULONG ulEdidBufferSize, EDID_DETAILED_TIMING *pEDIDDetailedTiming);
