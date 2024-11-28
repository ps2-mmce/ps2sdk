/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

#include "libspu2_internal.h"

void SpuGetCommonCDMix(int *cd_mix)
{
	*cd_mix = SPU_OFF;
	if ( (_spu_RXX[512 * _spu_core + 205] & 1) != 0 )
		*cd_mix = SPU_ON;
}