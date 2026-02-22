/**
 * @file sim_k64.h
 * @brief System Integration Module (SIM) for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef SIM_K64_H
#define SIM_K64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// SIM Base Address
//=============================================================================

#define SIM_BASE                      0x40047000
#define SIM                           ((SIM_Type*)SIM_BASE)

//=============================================================================
// SIM Register Structure
//=============================================================================

typedef struct {
    volatile uint32_t SOPT1;          // System Options Register 1
    volatile uint32_t SOPT1CFG;       // SOPT1 Configuration Register
    uint32_t RESERVED0[2];
    volatile uint32_t SOPT2;          // System Options Register 2
    uint32_t RESERVED1;
    volatile uint32_t SOPT4;          // System Options Register 4
    volatile uint32_t SOPT5;          // System Options Register 5
    uint32_t RESERVED2[2];
    volatile uint32_t SOPT7;          // System Options Register 7
    uint32_t RESERVED3[2];
    volatile uint32_t SDID;           // System Device ID Register
    volatile uint32_t SDIDH;          // System Device ID Register High
    uint32_t RESERVED4[6];
    volatile uint32_t SCGC4;          // System Clock Gating Control Register 4
    volatile uint32_t SCGC5;          // System Clock Gating Control Register 5
    volatile uint32_t SCGC6;          // System Clock Gating Control Register 6
    volatile uint32_t SCGC7;          // System Clock Gating Control Register 7
    volatile uint32_t CLKDIV1;        // System Clock Divider Register 1
    volatile uint32_t CLKDIV2;        // System Clock Divider Register 2
    volatile uint32_t SCGC3;          // System Clock Gating Control Register 3
    uint32_t RESERVED5[3];
    volatile uint32_t FCFG1;          // Flash Configuration Register 1
    volatile uint32_t FCFG2;          // Flash Configuration Register 2
    uint32_t RESERVED6[2];
    volatile uint32_t UIDH;           // Unique Identification Register High
    volatile uint32_t UIDMH;          // Unique Identification Register Mid-High
    volatile uint32_t UIDML;          // Unique Identification Register Mid Low
    volatile uint32_t UIDL;           // Unique Identification Register Low
    uint32_t RESERVED7[4];
    volatile uint32_t CLKDIV3;        // System Clock Divider Register 3
    volatile uint32_t CLKDIV4;        // System Clock Divider Register 4
    uint32_t RESERVED8[5];
    volatile uint32_t MCR;            // Misc Control Register
    uint32_t RESERVED9[7];
    volatile uint32_t COPC;           // COP Control Register
    volatile uint32_t SRVCOP;         // Service COP Register
    uint32_t RESERVED10[10];
    volatile uint32_t CLKDIV5;        // System Clock Divider Register 5
    volatile uint32_t CLKDIV6;        // System Clock Divider Register 6
    volatile uint32_t CLKDIV7;        // System Clock Divider Register 7
    volatile uint32_t CLKDIV8;        // System Clock Divider Register 8
    uint32_t RESERVED11[4];
    volatile uint32_t FCFG3;          // Flash Configuration Register 3
    volatile uint32_t FCFG4;          // Flash Configuration Register 4
    uint32_t RESERVED12[2];
    volatile uint32_t MISCCTRL;       // Misc Control Register
    uint32_t RESERVED13[3];
    volatile uint32_t COPC1;          // COP Control Register 1
    volatile uint32_t SRVCOP1;         // Service COP Register 1
    uint32_t RESERVED14[4];
    volatile uint32_t CLKSOUR1;       // Clock Source 1 Register
    volatile uint32_t CLKSOUR2;       // Clock Source 2 Register
    volatile uint32_t CLKSOUR3;       // Clock Source 3 Register
    volatile uint32_t CLKSOUR4;       // Clock Source 4 Register
    uint32_t RESERVED15[4];
    volatile uint32_t CLKOUTDIV1;     // Clock Out Divider 1 Register
    volatile uint32_t CLKOUTDIV2;     // Clock Out Divider 2 Register
    volatile uint32_t CLKOUTDIV3;     // Clock Out Divider 3 Register
    volatile uint32_t CLKOUTDIV4;     // Clock Out Divider 4 Register
    uint32_t RESERVED16[4];
    volatile uint32_t FLASHOPT1;      // Flash Option 1 Register
    volatile uint32_t FLASHOPT2;      // Flash Option 2 Register
    volatile uint32_t FLASHOPT3;      // Flash Option 3 Register
    volatile uint32_t FLASHOPT4;      // Flash Option 4 Register
    volatile uint32_t FLASHOPT5;      // Flash Option 5 Register
    volatile uint32_t FLASHOPT6;      // Flash Option 6 Register
    volatile uint32_t FLASHOPT7;      // Flash Option 7 Register
    volatile uint32_t FLASHOPT8;      // Flash Option 8 Register
    volatile uint32_t FLASHOPT9;      // Flash Option 9 Register
    volatile uint32_t FLASHOPT10;     // Flash Option 10 Register
    uint32_t RESERVED17[6];
    volatile uint32_t SDID128;        // System Device ID Register 128
    volatile uint32_t SDID127;        // System Device ID Register 127
    volatile uint32_t SDID126;        // System Device ID Register 126
    volatile uint32_t SDID125;        // System Device ID Register 125
    volatile uint32_t SDID124;        // System Device ID Register 124
    volatile uint32_t SDID123;        // System Device ID Register 123
    volatile uint32_t SDID122;        // System Device ID Register 122
    volatile uint32_t SDID121;        // System Device ID Register 121
    volatile uint32_t SDID120;        // System Device ID Register 120
    volatile uint32_t SDID119;        // System Device ID Register 119
    volatile uint32_t SDID118;        // System Device ID Register 118
    volatile uint32_t SDID117;        // System Device ID Register 117
    volatile uint32_t SDID116;        // System Device ID Register 116
    volatile uint32_t SDID115;        // System Device ID Register 115
    volatile uint32_t SDID114;        // System Device ID Register 114
    volatile uint32_t SDID113;        // System Device ID Register 113
    volatile uint32_t SDID112;        // System Device ID Register 112
    volatile uint32_t SDID111;        // System Device ID Register 111
    volatile uint32_t SDID110;        // System Device ID Register 110
    volatile uint32_t SDID109;        // System Device ID Register 109
    volatile uint32_t SDID108;        // System Device ID Register 108
    volatile uint32_t SDID107;        // System Device ID Register 107
    volatile uint32_t SDID106;        // System Device ID Register 106
    volatile uint32_t SDID105;        // System Device ID Register 105
    volatile uint32_t SDID104;        // System Device ID Register 104
    volatile uint32_t SDID103;        // System Device ID Register 103
    volatile uint32_t SDID102;        // System Device ID Register 102
    volatile uint32_t SDID101;        // System Device ID Register 101
    volatile uint32_t SDID100;        // System Device ID Register 100
    volatile uint32_t SDID99;         // System Device ID Register 99
    volatile uint32_t SDID98;         // System Device ID Register 98
    volatile uint32_t SDID97;         // System Device ID Register 97
    volatile uint32_t SDID96;         // System Device ID Register 96
    volatile uint32_t SDID95;         // System Device ID Register 95
    volatile uint32_t SDID94;         // System Device ID Register 94
    volatile uint32_t SDID93;         // System Device ID Register 93
    volatile uint32_t SDID92;         // System Device ID Register 92
    volatile uint32_t SDID91;         // System Device ID Register 91
    volatile uint32_t SDID90;         // System Device ID Register 90
    volatile uint32_t SDID89;         // System Device ID Register 89
    volatile uint32_t SDID88;         // System Device ID Register 88
    volatile uint32_t SDID87;         // System Device ID Register 87
    volatile uint32_t SDID86;         // System Device ID Register 86
    volatile uint32_t SDID85;         // System Device ID Register 85
    volatile uint32_t SDID84;         // System Device ID Register 84
    volatile uint32_t SDID83;         // System Device ID Register 83
    volatile uint32_t SDID82;         // System Device ID Register 82
    volatile uint32_t SDID81;         // System Device ID Register 81
    volatile uint32_t SDID80;         // System Device ID Register 80
    volatile uint32_t SDID79;         // System Device ID Register 79
    volatile uint32_t SDID78;         // System Device ID Register 78
    volatile uint32_t SDID77;         // System Device ID Register 77
    volatile uint32_t SDID76;         // System Device ID Register 76
    volatile uint32_t SDID75;         // System Device ID Register 75
    volatile uint32_t SDID74;         // System Device ID Register 74
    volatile uint32_t SDID73;         // System Device ID Register 73
    volatile uint32_t SDID72;         // System Device ID Register 72
    volatile uint32_t SDID71;         // System Device ID Register 71
    volatile uint32_t SDID70;         // System Device ID Register 70
    volatile uint32_t SDID69;         // System Device ID Register 69
    volatile uint32_t SDID68;         // System Device ID Register 68
    volatile uint32_t SDID67;         // System Device ID Register 67
    volatile uint32_t SDID66;         // System Device ID Register 66
    volatile uint32_t SDID65;         // System Device ID Register 65
    volatile uint32_t SDID64;         // System Device ID Register 64
    volatile uint32_t SDID63;         // System Device ID Register 63
    volatile uint32_t SDID62;         // System Device ID Register 62
    volatile uint32_t SDID61;         // System Device ID Register 61
    volatile uint32_t SDID60;         // System Device ID Register 60
    volatile uint32_t SDID59;         // System Device ID Register 59
    volatile uint32_t SDID58;         // System Device ID Register 58
    volatile uint32_t SDID57;         // System Device ID Register 57
    volatile uint32_t SDID56;         // System Device ID Register 56
    volatile uint32_t SDID55;         // System Device ID Register 55
    volatile uint32_t SDID54;         // System Device ID Register 54
    volatile uint32_t SDID53;         // System Device ID Register 53
    volatile uint32_t SDID52;         // System Device ID Register 52
    volatile uint32_t SDID51;         // System Device ID Register 51
    volatile uint32_t SDID50;         // System Device ID Register 50
    volatile uint32_t SDID49;         // System Device ID Register 49
    volatile uint32_t SDID48;         // System Device ID Register 48
    volatile uint32_t SDID47;         // System Device ID Register 47
    volatile uint32_t SDID46;         // System Device ID Register 46
    volatile uint32_t SDID45;         // System Device ID Register 45
    volatile uint32_t SDID44;         // System Device ID Register 44
    volatile uint32_t SDID43;         // System Device ID Register 43
    volatile uint32_t SDID42;         // System Device ID Register 42
    volatile uint32_t SDID41;         // System Device ID Register 41
    volatile uint32_t SDID40;         // System Device ID Register 40
    volatile uint32_t SDID39;         // System Device ID Register 39
    volatile uint32_t SDID38;         // System Device ID Register 38
    volatile uint32_t SDID37;         // System Device ID Register 37
    volatile uint32_t SDID36;         // System Device ID Register 36
    volatile uint32_t SDID35;         // System Device ID Register 35
    volatile uint32_t SDID34;         // System Device ID Register 34
    volatile uint32_t SDID33;         // System Device ID Register 33
    volatile uint32_t SDID32;         // System Device ID Register 32
    volatile uint32_t SDID31;         // System Device ID Register 31
    volatile uint32_t SDID30;         // System Device ID Register 30
    volatile uint32_t SDID29;         // System Device ID Register 29
    volatile uint32_t SDID28;         // System Device ID Register 28
    volatile uint32_t SDID27;         // System Device ID Register 27
    volatile uint32_t SDID26;         // System Device ID Register 26
    volatile uint32_t SDID25;         // System Device ID Register 25
    volatile uint32_t SDID24;         // System Device ID Register 24
    volatile uint32_t SDID23;         // System Device ID Register 23
    volatile uint32_t SDID22;         // System Device ID Register 22
    volatile uint32_t SDID21;         // System Device ID Register 21
    volatile uint32_t SDID20;         // System Device ID Register 20
    volatile uint32_t SDID19;         // System Device ID Register 19
    volatile uint32_t SDID18;         // System Device ID Register 18
    volatile uint32_t SDID17;         // System Device ID Register 17
    volatile uint32_t SDID16;         // System Device ID Register 16
    volatile uint32_t SDID15;         // System Device ID Register 15
    volatile uint32_t SDID14;         // System Device ID Register 14
    volatile uint32_t SDID13;         // System Device ID Register 13
    volatile uint32_t SDID12;         // System Device ID Register 12
    volatile uint32_t SDID11;         // System Device ID Register 11
    volatile uint32_t SDID10;         // System Device ID Register 10
    volatile uint32_t SDID9;          // System Device ID Register 9
    volatile uint32_t SDID8;          // System Device ID Register 8
    volatile uint32_t SDID7;          // System Device ID Register 7
    volatile uint32_t SDID6;          // System Device ID Register 6
    volatile uint32_t SDID5;          // System Device ID Register 5
    volatile uint32_t SDID4;          // System Device ID Register 4
    volatile uint32_t SDID3;          // System Device ID Register 3
    volatile uint32_t SDID2;          // System Device ID Register 2
    volatile uint32_t SDID1;          // System Device ID Register 1
} SIM_Type;

//=============================================================================
// SIM SCGC6 (System Clock Gating Control Register 6) Bits
//=============================================================================

#define SIM_SCGC6_SPI0_SHIFT          12
#define SIM_SCGC6_SPI0_MASK           (1 << SIM_SCGC6_SPI0_SHIFT)
#define SIM_SCGC6_SPI0(x)             ((x) << SIM_SCGC6_SPI0_SHIFT)

#define SIM_SCGC6_SPI1_SHIFT          13
#define SIM_SCGC6_SPI1_MASK           (1 << SIM_SCGC6_SPI1_SHIFT)
#define SIM_SCGC6_SPI1(x)             ((x) << SIM_SCGC6_SPI1_SHIFT)

//=============================================================================
// SIM SCGC3 (System Clock Gating Control Register 3) Bits
//=============================================================================

#define SIM_SCGC3_SPI2_SHIFT          0
#define SIM_SCGC3_SPI2_MASK           (1 << SIM_SCGC3_SPI2_SHIFT)
#define SIM_SCGC3_SPI2(x)             ((x) << SIM_SCGC3_SPI2_SHIFT)

//=============================================================================
// Convenience Macros
//=============================================================================

#define SIM_SCGC6_SPI0                SIM_SCGC6_SPI0(1)
#define SIM_SCGC6_SPI1                SIM_SCGC6_SPI1(1)
#define SIM_SCGC3_SPI2                SIM_SCGC3_SPI2(1)

#ifdef __cplusplus
}
#endif

#endif // SIM_K64_H
