#include "Components/Includes.hpp"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        Core.InitializeThread();
    }
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        GUI.Unload();
        Events.DetachAllDetours();
        Core.DestroyThread();
    }

    return TRUE;
}