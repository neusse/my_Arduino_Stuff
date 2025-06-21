#define VERSION "1"
#define BAUD_RATE 115200
#define MAX_INPUT_SIZE 100
#define TERMINAL_COLS 80
#define DEBUG

/* Pin definitions */
#define SBC_CLOCK 2
#define SBC_RW 3
#define SBC_IRQ 5
#define SBC_NMI 6
#define SBC_BE 4
#define SBC_READY 7
#define SBC_SYNC 8
#define SBC_RESET 23

//  https://www.zilog.com/docs/z80/um0080.pdf
// sbc label goes to arduino pin on backplae board
//
#define Z80_MREQ 8 // board label SYNC 8
#define Z80_RD 3   // board label RW 3
#define Z80_WR 7   // board label READY 7       SBC_BE 4
#define Z80_M1 23   // board label RESET 23
#define Z80_WAIT 999

#define RC2014_PAGE 9   // board pin 35 
#define Z80_BUSREQ 10   // board pin 36
#define Z80_BUSACK 11   // board pin 37
#define Z80_IORQ 12     // board pin 38
#define Z80_INT 13      // board pin 39


// #define SBC_PIN35 9
// #define SBC_PIN36 10
// #define SBC_PIN37 11
// #define SBC_PIN38 12
// #define SBC_PIN39 13

#define SBC_A15 22
#define SBC_A14 24
#define SBC_A13 26
#define SBC_A12 28
#define SBC_A11 30
#define SBC_A10 32
#define SBC_A9 34
#define SBC_A8 36
#define SBC_A7 38
#define SBC_A6 40
#define SBC_A5 42
#define SBC_A4 44
#define SBC_A3 46
#define SBC_A2 48
#define SBC_A1 50
#define SBC_A0 52
const char SBC_ADDR[] = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52};

#define SBC_D7 39
#define SBC_D6 41
#define SBC_D5 43
#define SBC_D4 45
#define SBC_D3 47
#define SBC_D2 49
#define SBC_D1 51
#define SBC_D0 53
const char SBC_DATA[] = {39, 41, 43, 45, 47, 49, 51, 53};



#define USER_LED 65
#define USER_SW1 62
#define USER_SW2 63
#define USER_SW3 64

/* Arduino clock modes */
#define CLK_MODE_NONE 0
#define CLK_MODE_MANUAL 1
#define CLK_MODE_AUTO 2
/*                            1Hz     2Hz     4Hz    16Hz   32Hz   128   256 */
const long CLK_PERIOD[] = {1000000, 500000, 250000, 62500, 31250, 7812, 3906};
#define CLK_SPEED_1 0
#define CLK_SPEED_2 1
#define CLK_SPEED_4 2
#define CLK_SPEED_16 3
#define CLK_SPEED_32 4
#define CLK_SPEED_128 5
#define CLK_SPEED_256 6
#define CLK_MAX_SETTING CLK_SPEED_256
#define CLK_MAX_MONITOR_SPEED CLK_SPEED_32

/* SBC Address Segments */
#define ADR_UNSPECIFIED 0
#define ADR_RAM 1
#define ADR_ZERO_PAGE 2
#define ADR_STACK 3
#define ADR_VIA 4
#define ADR_CUSTOM 5
#define ADR_ROM 6
#define ADR_VECTORS 7

#define COMMAND_SET_MAIN 0
#define COMMAND_SET_CONTROL 1
