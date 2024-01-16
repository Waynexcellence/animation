#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#define ERR_EXIT(a)	do { perror(a); exit(-1); } while(0)

#define HEADER_SIZE 14
#define DataSize(bmp) (bmp->width*bmp->height*3)

#define RED     "\x1b[;31;1m"	// for develop
#define GRN     "\x1b[;32;1m"	// for finished
#define YEL     "\x1b[;33;1m"	// for processd
#define BLU     "\x1b[;34;1m"	// for create
#define PUR     "\x1b[;35;1m"	// for
#define CYA     "\x1b[;36;1m"	// for join
#define RESET   "\x1b[0;m"
#define CLEAR   "\033[2J"       // Clear all the terminal
#define CORNER  "\033[H"        
#define DELETE  "\r\033[K"
#define HIDE    "\033[?25l"     // 隱藏光標
#define SHOW    "\033[?25h"     // 顯示光標

typedef long UINT32;
typedef unsigned short int UINT16;
typedef unsigned char BYTE;

#define U16(x)    ((unsigned short) (x))
#define U32(x)    ((int) (x))
#define B2U16(bytes,offset)  (U16(bytes[offset]) | U16(bytes[offset+1]) << 8)
#define B2U32(bytes,offset)  (U32(bytes[offset]) | U32(bytes[offset+1]) << 8 | U32(bytes[offset+2]) << 16 | U32(bytes[offset+3]) << 24)


typedef struct BMP {
    BYTE header[HEADER_SIZE];
    BYTE info[256];
    // Header
    UINT16 signature;
    UINT32 fileSize;
    UINT32 hreserved;
    UINT32 dataOffset;
    // InfoHeader
    UINT32 infosize;
    UINT32 width;
    UINT32 height;
    UINT16 planes;
    UINT16 bitsPerPixel;
    UINT32 compression;
    UINT32 imageSize;
    UINT32 xPixelsPerM;
    UINT32 yPixelsPerM;
    UINT32 colorsUsed;
    UINT32 colorsImportant;
    // Data
    BYTE **data;
} BMP;

typedef struct Pixel {
    BYTE R;
    BYTE G;
    BYTE B;
} Pixel;

typedef struct Info {
    int frame_rate_a;
    int frame_rate_b;
    int period;
    int width;
    int height;
} Info;

typedef struct Cut_Argument {
    char input[256];
    int start;
    int end;
    int which;
    int result;
} Cut_Argument;

typedef struct Load_Argument {
    int source;
    int target;
    int result;
} Load_Argument;
