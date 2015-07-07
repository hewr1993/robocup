/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"
#include "motor_control.h"

int main (int argc, char **argv)
{
    motor_init();
    kbctrl(argc, argv);
    return 0;
}
