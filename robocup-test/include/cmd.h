#ifndef CMD_H
#define CMD_H

#define VERSION 15

// data: -99 ~ 99
#define DATA_MAX 99

// short cmd start here
//
// stop motor now
#define STOP 100
// cmd head, len = 2
#define HEAD2 101
// cmd head, len = 3
#define HEAD3 102
// read ir
#define IR 103
// read v
#define RV 124
// read d1, d2
#define RD 125
//
// read version (stable)
#define VER 126
// clear sn, len_cmd, t_cur, t_end (stable)
#define CLR 127

// long cmd start here
//
// acc with angle set, len = 3
#define AA_ANGLE -100
// acc with distance set, len = 3
#define AA_DISTANCE -101
// acc without late echo, len = 2
#define AA -102
// acc only, len = 2
#define AA_ONLY -103

// distinguish echo
#define M_RD 0x80

#endif

