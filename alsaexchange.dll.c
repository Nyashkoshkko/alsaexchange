/*
 * File: MyWin.c
 *
 * Copyright (c) The copyright holder.
 *
 * Basic Wine wrapper for the Linux 3rd party library so that it can be
 * used by the application
 *
 * Currently this file makes no attempt to be a full wrapper for the 3rd
 * party library; it only exports enough for our own use.
 *
 * Note that this is a Unix file; please don't go converting it to DOS format
 * (e.g. converting line feeds to Carriage return/Line feed).
 *
 * This file should be built in a Wine environment as a Winelib library,
 * linked to the Linux 3rd party libraries (currently libxxxx.so and
 * libyyyy.so)
 */

#include "alsaexchange.h"
#include <windef.h> /* Part of the Wine header files */

/* This declaration is as defined in the spec file.  It is deliberately not
 * specified in terms of 3rd party types since we are messing about here
 * between two operating systems (making it look like a Windows thing when
 * actually it is a Linux thing).  In this way the compiler will point out any
 * inconsistencies.
 * For example the fourth argument needs care
 */
int WINAPI alsaexchange_putsamples_linuxcall (void * buf, int cnt)
{
    return alsaexchange_putsamples(buf, cnt);

    //return 0;
}

/* End of file */ 
