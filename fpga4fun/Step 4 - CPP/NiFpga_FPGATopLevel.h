/*
 * Generated with the FPGA Interface C API Generator 1.2.0
 * for NI-RIO 3.5.0 or later.
 */

#ifndef __NiFpga_FPGATopLevel_h__
#define __NiFpga_FPGATopLevel_h__

#ifndef NiFpga_Version
   #define NiFpga_Version 120
#endif

#include "NiFpga.h"

/**
 * The filename of the FPGA bitfile.
 *
 * This is a #define to allow for string literal concatenation. For example:
 *
 *    static const char* const Bitfile = "C:\\" NiFpga_FPGATopLevel_Bitfile;
 */
#define NiFpga_FPGATopLevel_Bitfile "NiFpga_FPGATopLevel.lvbitx"

/**
 * The signature of the FPGA bitfile.
 */
static const char* const NiFpga_FPGATopLevel_Signature = "424F8C7E7E770824D0973A78938F7695";

typedef enum
{
   NiFpga_FPGATopLevel_IndicatorU32_U32PP = 0x8008,
   NiFpga_FPGATopLevel_IndicatorU32_U32TimesTwo = 0x8004,
} NiFpga_FPGATopLevel_IndicatorU32;

typedef enum
{
   NiFpga_FPGATopLevel_ControlU32_U32In = 0x8000,
} NiFpga_FPGATopLevel_ControlU32;

#endif
