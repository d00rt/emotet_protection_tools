/*
 *   This library is created to provide some functionality to Detectet and Protectet tools
 *   Both tools (Detectet and Protectet) are tools created to fight against Emotet malware
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

/* Protectet specific global variables */
LPCSTR INSTALLATION_FOLDER = "Protectet";
LPCSTR INSTALLATION_NAME = "protectect2020.exe";
LPCSTR SERVICE_NAME = "Protectet";
CHAR INSTALLATION_FULL_PATH[MAX_PATH + 1] = { 0 };

/* Protectet & Detectet common global variables */
DWORD VOLUME_SERIAL_NUMBER = 0;
CHAR EVENT_NAME_E[MAX_PATH + 1] = { 0 };
CHAR MUTEX_NAME_M[MAX_PATH + 1] = { 0 };
CHAR MUTEX_NAME_I[MAX_PATH + 1] = { 0 };

BOOL UtilsGetEventName( DWORD VolumeSerialNumber, LPSTR EventName )
{
    if ( EventName == NULL )
    {
        return FALSE;
    }

    snprintf( EventName, MAX_PATH, "Global\\E%08lX", VolumeSerialNumber );
    return TRUE;
}

BOOL UtilsGetMutexNameM( DWORD VolumeSerialNumber, LPSTR MutexName )
{
    if ( MutexName == NULL )
    {
        return FALSE;
    }

    snprintf( MutexName, MAX_PATH, "Global\\M%08lX", VolumeSerialNumber );
    return TRUE;
}

BOOL UtilsGetMutexNameI( DWORD VolumeSerialNumber, LPSTR MutexName )
{
    if ( MutexName == NULL )
    {
        return FALSE;
    }

    snprintf( MutexName, MAX_PATH, "Global\\I%08lX", VolumeSerialNumber );
    return TRUE;
}

BOOL UtilsGetInstallationPath( LPSTR InstallationFullPath )
{
    CHAR Path[MAX_PATH + 1] = { 0 };

    if ( !ExpandEnvironmentStringsA( "%PROGRAMFILES%", Path, MAX_PATH ) )
    {
        printf( "Error on ExpandEnvironmentStringsA error: %08lX", GetLastError() );
        return FALSE;
    }

    PathCombineA( InstallationFullPath, Path, INSTALLATION_FOLDER );
    PathCombineA( InstallationFullPath, InstallationFullPath, INSTALLATION_NAME );

    return TRUE;
}

BOOL UtilsServiceExists( LPSTR ServiceName )
{
    SC_HANDLE hServiceControl = NULL, hService = NULL;
    hServiceControl = OpenSCManagerA( NULL, NULL, SC_MANAGER_ALL_ACCESS );

    if ( ServiceName == NULL )
        return FALSE;

    if ( hServiceControl == NULL )
    {
        //printf( "Error on UtilsServiceExists.OpenSCManagerA %08lX\n", GetLastError() );
        return FALSE;
    }

    hService = OpenServiceA( hServiceControl, ServiceName, GENERIC_READ );

    if ( hService == NULL )
    {
        CloseServiceHandle( hServiceControl );
        return FALSE;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hServiceControl );
    return TRUE;
}

BOOL UtilsFileExists( LPSTR FileName )
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = NULL;

    if ( FileName == NULL )
        return FALSE;

    hFind = FindFirstFileA( FileName, &FindFileData );

    if ( hFind == INVALID_HANDLE_VALUE )
        return FALSE;

    FindClose( hFind );
    return TRUE;
}

BOOL UtilsLaunchedFromInstallationPath( LPSTR InstallationFullPath )
{
    CHAR ExecutableFilePath[MAX_PATH + 1] = { 0 };
    uint32_t InstallationPathSize = 0, ExecutableFilePathSize = 0, i = 0;

    if ( InstallationFullPath == NULL )
    {
        return FALSE;
    }

    GetModuleFileNameA( NULL, ExecutableFilePath, MAX_PATH );

    InstallationPathSize = strnlen( InstallationFullPath, MAX_PATH );
    ExecutableFilePathSize = strnlen( ExecutableFilePath, MAX_PATH );

    if ( InstallationPathSize > ExecutableFilePathSize )
        return FALSE;

    while ( i < InstallationPathSize )
    {
        if ( ( BYTE ) InstallationFullPath[InstallationPathSize - 1 - i] != ( BYTE ) ExecutableFilePath[ExecutableFilePathSize - 1 - i] )
            return FALSE;
        i++;
    }

    return TRUE;
}

BOOL UtilsMoveFileToInstallationPath( LPSTR InstallationFullPath )
{
    //CreateDirectory();
    CHAR InstallationFullPathDir[MAX_PATH + 1] = { 0 };
    CHAR ExecutableFilePath[MAX_PATH + 1] = { 0 };

    if ( InstallationFullPath == NULL )
    {
        return FALSE;
    }

    size_t i = 0;
    for (; InstallationFullPath[ strnlen( InstallationFullPath, MAX_PATH ) - 1 - i ] != '\\'; i++);

    strncpy( InstallationFullPathDir, InstallationFullPath,  strnlen( InstallationFullPath, MAX_PATH ) - ++i );

    if ( !CreateDirectoryA( InstallationFullPathDir, NULL ) )
    {
        if ( GetLastError() != ERROR_ALREADY_EXISTS )
        {
            CHAR ErrorMsg[255] = { 0 };
            sprintf( "ERROR: Can't create installation folder %s", InstallationFullPathDir );
            printf( ErrorMsg );
            return FALSE;
        }
    }
    //MoveFile();
    GetModuleFileNameA( NULL, ExecutableFilePath, MAX_PATH );

    if ( !MoveFileExA( ExecutableFilePath, InstallationFullPath, MOVEFILE_REPLACE_EXISTING ) )
    {
        return FALSE;
    }
    return TRUE;
}

BOOL UtilsBasicInit( DWORD *VolumeSerialNumber, LPSTR EventName, LPSTR MutantNameI, LPSTR MutantNameM, LPSTR InstallationFullPath )
{
    /*
     * WARNING: GetInstallatinPath uses SHGetSpecialFolderPathA function
     * to retrieve the current user Desktop folder, when this function is called
     * from the service since the service is executed as System, it fails.
     * In order to avoid this, move installation path to programfiles for example
     */
    if ( InstallationFullPath != NULL )
    {
        if ( !UtilsGetInstallationPath( InstallationFullPath ) )
        {
            printf( "ERROR: Can't get current user desktop path" );
            return FALSE;
        }
    }

    if ( !GetVolumeInformationA( "C:\\", 0, 0, VolumeSerialNumber, 0, 0, 0, 0 ) )
    {
        printf( "Can't Obtain Volume Serial Number\n" );
        return FALSE;
    }

    if ( !UtilsGetEventName( *VolumeSerialNumber, EventName ) )
    {
        printf( "Can't Obtain Event Name\n" );
        return FALSE;
    }

    if ( !UtilsGetMutexNameI( *VolumeSerialNumber, MutantNameI ) )
    {
        printf( "Can't Obtain Mutex Name I\n" );
        return FALSE;
    }

    if ( !UtilsGetMutexNameM( *VolumeSerialNumber, MutantNameM ) )
    {
        printf( "Can't Obtain Mutex Name M\n" );
        return FALSE;
    }

    return TRUE;
}

VOID UtilsPrintEmotetGeneratedObjects( DWORD VolumeSerialNumber )
{
    CHAR  EventName[MAX_PATH + 1] = { 0 }, MutantName1[MAX_PATH + 1] = { 0 }, MutantName2[MAX_PATH + 1] = { 0 };
    UtilsGetEventName( VolumeSerialNumber, EventName );
    UtilsGetMutexNameI( VolumeSerialNumber, MutantName1 );
    UtilsGetMutexNameM( VolumeSerialNumber, MutantName2 );

    printf( "[+] These Objects have been created by Emotet on your system\n" );
    printf( "    - Event: %s\n", EventName );
    printf( "    - Mutant: %s\n", MutantName1 );
    printf( "    - Mutant: %s\n", MutantName2 );
    printf( "[+] The process containing 2 or more of these Objects is the Emotet binary.\n" );
    printf( "[+] You should remove it, and any registry key, scheduled tasks that points to that binary.\n" );
}

BYTE UtilsIsInfected( DWORD VolumeSerialNumber )
{
    HANDLE hEvent = NULL, hMutant1 = NULL, hMutant2 = NULL;
    BYTE EventMatch = 0, /* Mutant1Match = 0,*/ Mutant2Match = 0;
    CHAR  EventName[MAX_PATH + 1] = { 0 }, MutantName1[MAX_PATH + 1] = { 0 }, MutantName2[MAX_PATH + 1] = { 0 };

    UtilsGetEventName( VolumeSerialNumber, EventName );
    UtilsGetMutexNameI( VolumeSerialNumber, MutantName1 );
    UtilsGetMutexNameM( VolumeSerialNumber, MutantName2 );

    hEvent = CreateEventA( 0, 0, 0, EventName );

    if ( GetLastError() == ERROR_ACCESS_DENIED ||
         GetLastError() == ERROR_ALREADY_EXISTS )
    {
        EventMatch = 1;
    }
    SetEvent(hEvent);

    hMutant1 = CreateMutexA( 0, 0, MutantName1 );

    /*
    if ( GetLastError() == ERROR_ACCESS_DENIED ||
         GetLastError() == ERROR_ALREADY_EXISTS )
    {
        Mutant1Match = 1;
    }
    */

    hMutant2 = CreateMutexA( 0, 0, MutantName2 );

    if ( GetLastError() == ERROR_ACCESS_DENIED ||
         GetLastError() == ERROR_ALREADY_EXISTS )
    {
        Mutant2Match = 1;
    }

    if ( hEvent != NULL ) CloseHandle( hEvent );
    if ( hMutant1 != NULL ) CloseHandle( hMutant1 );
    if ( hMutant2 != NULL ) CloseHandle( hMutant2 );
    return ( BYTE )( EventMatch & /* Mutant1Match &*/ Mutant2Match );
}

