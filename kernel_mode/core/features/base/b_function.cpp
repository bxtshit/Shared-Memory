#include <core/features/base/b_function.h>

NTSTATUS base::get(pbase_invoke x)
{
    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)x->process_id, &process);
    if (!NT_SUCCESS(status) || !process)
    {
        DbgPrint("failed to find process_id: %X", status);
        return 0;
    }

    uintptr_t image_base = (uintptr_t)PsGetProcessSectionBaseAddress(process);
    if (!image_base)
    {
        DbgPrint("failed to get process's base address");
        ObDereferenceObject(process);
        return 0;
    }

    x->address = image_base;

    DbgPrint("process_id: %d", x->process_id);
    DbgPrint("base_address: %p", (void*)x->address);

    ObDereferenceObject(process);

    return STATUS_SUCCESS;
}