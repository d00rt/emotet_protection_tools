/*
 *   Detectet is a program created to detect if a Windows systems is infected by Emotet malware.
 *   Combining Detectet and Protectet tools you can protect your system against Emotet
 *
 *   Copyright (C) 2020  Markel Picado Ortiz aka d00rt <d00rt.fake@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include "utils.h"


int main( int argc, char *argv[] )
{
    printf( "Detectet Copyright (C) 2020 d00rt <d00rt.fake@gmail.com>\n" );

    if ( !UtilsBasicInit( &VOLUME_SERIAL_NUMBER, EVENT_NAME_E, MUTEX_NAME_I, MUTEX_NAME_M, NULL ) )
    {
        printf( "ERROR: Basic initialization error...\n" );
        return 0;
    }

    if ( !UtilsIsInfected( VOLUME_SERIAL_NUMBER ) ) {
        printf( "[+] This system apparently is not infected by Emotet :)\n" );
        return 0;
    }

    printf( "[!!!] This system apparently was infected by Emotet :( \n" );
    UtilsPrintEmotetGeneratedObjects( VOLUME_SERIAL_NUMBER );
    return 0;
}
