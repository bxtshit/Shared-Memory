#include <ntifs.h>
#include <windef.h>

#include <core/data/receiver.h>
#include <core/data/shared_memory/functions.h>

NTSTATUS driver_entry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registry_path)
{
    UNREFERENCED_PARAMETER(registry_path);

    if (!sm_pointer.create()) { DbgPrint("failed to create shared memory"); }

    sm_pointer.open_events();

    HANDLE hThread;
    PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, (PKSTART_ROUTINE)data::receive, NULL);

    return STATUS_SUCCESS;
}