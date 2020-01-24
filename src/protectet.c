/*
 *   Protectet is a program created to protect Windows systems against Emotet malware.
 *   Protectet also stops an Emotet infection if the system is infected
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
#include <windows.h>
#include <shlwapi.h>
#include "utils.h"

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
HANDLE stopServiceEvent = 0;

DWORD GlobalI = 1;

int Protection();

void WINAPI ServiceControlHandler( DWORD controlCode )
{
    switch ( controlCode )
    {
        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus( serviceStatusHandle, &serviceStatus );

            SetEvent( stopServiceEvent );
            return;

        case SERVICE_CONTROL_PAUSE:
            break;

        case SERVICE_CONTROL_CONTINUE:
            break;

        default:
            if ( controlCode >= 128 && controlCode <= 255 )
                // user defined control code
                break;
            else
                // unrecognised control code
                break;
    }

    SetServiceStatus( serviceStatusHandle, &serviceStatus );
}

VOID WINAPI ServiceMain( DWORD argc, TCHAR *argv[] )
{
    // initialise service status
    serviceStatus.dwServiceType = SERVICE_WIN32;
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted = 0;
    serviceStatus.dwWin32ExitCode = NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;

    serviceStatusHandle = RegisterServiceCtrlHandler( SERVICE_NAME, ServiceControlHandler );

    if ( serviceStatusHandle == NULL )
    {
        return;
    }

    // service is starting
    serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus( serviceStatusHandle, &serviceStatus );

    // do initialisation here
    stopServiceEvent = CreateEvent( 0, FALSE, FALSE, 0 );

    // running
    serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus( serviceStatusHandle, &serviceStatus );

    //Main

    // Create Protection Mutant
    HANDLE hMutant = CreateMutexA( 0, 0, MUTEX_NAME_M );

    do
    {
        Protection();
    } while ( WaitForSingleObject( stopServiceEvent, 5000 ) == WAIT_TIMEOUT );

    serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    SetServiceStatus( serviceStatusHandle, &serviceStatus );

    CloseHandle( stopServiceEvent );
    CloseHandle( hMutant );

    // service is now stopped
    serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus( serviceStatusHandle, &serviceStatus );
}

VOID RunService()
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { (LPSTR) SERVICE_NAME, ServiceMain },
        { 0, 0 }
    };

    StartServiceCtrlDispatcher( ServiceTable );
}

BOOL CreateProtectetService()
{
    CHAR CommandLine[MAX_PATH + 1] = { 0 };
    SC_HANDLE hServiceControl = NULL, hService = NULL;
    hServiceControl = OpenSCManagerA( NULL, NULL, SC_MANAGER_ALL_ACCESS );

    if ( hServiceControl == NULL )
    {
        printf( "Error on OpenSCManagerA %08lX\n", GetLastError() );
        return FALSE;
    }

    sprintf( CommandLine, "%s @D00RT_RM d00rt.fake@gmail.com", INSTALLATION_FULL_PATH );

    hService = CreateServiceA( hServiceControl, SERVICE_NAME, SERVICE_NAME, 0x12,
                    SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
                    CommandLine,
                    NULL, NULL, NULL, NULL, NULL );

    if ( hService == NULL )
    {
        printf( "Error on CreateServiceA %08lX\n", GetLastError() );
        hService = OpenServiceA( hServiceControl, SERVICE_NAME, SERVICE_ALL_ACCESS );
        if ( hService == NULL )
        {
            printf( "Error on OpenServiceA %08lX\n", GetLastError() );
            return FALSE;
        }
    }

    if ( !StartServiceA( hService, 0, NULL ) )
    {
        printf( "Error on StartServiceA %08lX\n", GetLastError() );
        return FALSE;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hServiceControl );

    return TRUE;
}

BOOL AmiInstalled()
{
    //if ( UtilsLaunchedFromInstallationPath() )
    if ( UtilsServiceExists( ( LPSTR ) SERVICE_NAME ) & UtilsFileExists( INSTALLATION_FULL_PATH ) )
        return TRUE;
    return FALSE;
}

BOOL Install()
{
    if ( !UtilsMoveFileToInstallationPath( INSTALLATION_FULL_PATH ) || !CreateProtectetService() )
        return FALSE;

    return TRUE;
}

int Protection()
{
    printf( "Trying to stop the current running Emotet instance...\n" );
    /*
        Doing this with OpenEvent(), it doesn't work since it returns NULL and
        GetLastError says 0x5 which means ERROR_ACCESS_DENIED. But if you do it
        with CreateEvent, you can get the HANDLE to the Event...
        This worked only if the Event owner if the same user
    */
    HANDLE hEvent = CreateEventA( NULL, FALSE, FALSE, EVENT_NAME_E );

    if ( hEvent == NULL ) {
        printf( "Invalid access rights, you should execute this program as SYSTEM.\n" );
        return 1;
    }

    SetEvent(hEvent);
    CloseHandle(hEvent);

    printf( "The Emotet running instance was successfully stopped!\n" );
    return 0;
}

int main( int argc, char *argv[] )
{
    printf( "Protectet Copyright (C) 2020 d00rt <d00rt.fake@gmail.com>\n" );

    if ( !UtilsBasicInit( &VOLUME_SERIAL_NUMBER, EVENT_NAME_E, MUTEX_NAME_I, MUTEX_NAME_M, INSTALLATION_FULL_PATH ) )
    {
        MessageBoxA( NULL, "ERROR: Basic initialization error...", SERVICE_NAME, MB_OK );
        return 0;
    }

    if ( !AmiInstalled() )
    {
        if ( Install() )
        {
            MessageBoxA( NULL, "Emotet Active Protection Enabled!\n\nProtectet Copyright (C) 2020 d00rt <d00rt.fake@gmail.com>", SERVICE_NAME, MB_OK );
            return 0;
        }
        printf( "ERROR: Can't Install Emotet Active Protection!" );
        return 0;
    }

    if ( argc == 3 && UtilsLaunchedFromInstallationPath( INSTALLATION_FULL_PATH ) )
    {
        RunService();
    }
    else
    {
        MessageBoxA( NULL, "Emotet Active Protection is already installed!", SERVICE_NAME, MB_OK );
        UtilsMoveFileToInstallationPath( INSTALLATION_FULL_PATH );
    }

    return 0;
}
