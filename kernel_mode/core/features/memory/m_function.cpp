#include <core/features/memory/m_function.h>
#include <dependencies/spoofing/callstack.h>
#include <core/features/dtb/dtb.h>

NTSTATUS resolve_dtb(dtb_invoke* request)
{
    dtb_invoke data = { 0 };

    PEPROCESS process = 0;
    if (PsLookupProcessByProcessId((HANDLE)data.process_id, &process) != STATUS_SUCCESS)
    {
        DbgPrint("invalid process.\n");
        return STATUS_UNSUCCESSFUL;
    }

    physical::m_stored_dtb = pml4::dirbase_from_base_address((void*)PsGetProcessSectionBaseAddress(process));

    DbgPrint("cr3: %llx\n", physical::m_stored_dtb);

    ObfDereferenceObject(process);

    return STATUS_SUCCESS;
}

NTSTATUS memory::read_memory(pread_invoke x)
{
    hide;

    PEPROCESS process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)x->process_id, &process);

    if (!NT_SUCCESS(status) || !process)
        return STATUS_UNSUCCESSFUL;

    __try
    {
        SIZE_T this_offset = 0;
        SIZE_T total_size = x->size;

        if (total_size == 0)
        {
            ObDereferenceObject(process);
            return STATUS_INVALID_PARAMETER;
        }

        auto physical_address = physical::translate_linear(physical::m_stored_dtb, x->address + this_offset);

        if (!physical_address)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ULONG64 final_size = physical::find_min(PAGE_SIZE - (physical_address & 0xFFF), x->size);
        SIZE_T bytes_read = 0;

        NTSTATUS read_status = physical::read_physical(physical_address, (PVOID)(x->buffer + this_offset), final_size, &bytes_read);
        if (!NT_SUCCESS(read_status))
        {
            ObDereferenceObject(process);
            return read_status;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ObDereferenceObject(process);
        return GetExceptionCode();
    }

    ObDereferenceObject(process);
    return STATUS_SUCCESS;
}

UINT_PTR memory::find_guarded_region(pggr_invoke x)
{
    hide;

    PSYSTEM_BIGPOOL_INFORMATION pool_information = 0;

    ULONG information_length = 0;
    NTSTATUS status = ZwQuerySystemInformation(System_bigpool_informationn, &information_length, 0, &information_length);

    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        if (pool_information)
            ExFreePool(pool_information);

        pool_information = (PSYSTEM_BIGPOOL_INFORMATION)ExAllocatePool(NonPagedPool, information_length);
        status = ZwQuerySystemInformation(System_bigpool_informationn, pool_information, information_length, &information_length);
    }
    x->address = 0;

    if (pool_information)
    {
        for (ULONG i = 0; i < pool_information->Count; i++)
        {
            SYSTEM_BIGPOOL_ENTRY* allocation_entry = &pool_information->AllocatedInfo[i];

            UINT_PTR virtual_address = (UINT_PTR)allocation_entry->VirtualAddress & ~1ull;

            if (allocation_entry->NonPaged && allocation_entry->SizeInBytes == 0x200000)
            {
                if (x->address == 0 && allocation_entry->TagUlong == 'TnoC') {
                    x->address = virtual_address;
                }

            }
        }

        ExFreePool(pool_information);
    }

    return x->address;
}


auto memory::get_dtb_offset() {

    RTL_OSVERSIONINFOW ver = { 0 };
    RtlGetVersion(&ver);

    switch (ver.dwBuildNumber)
    {
    case WINDOWS_1803:
        return 0x0278;
        break;
    case WINDOWS_1809:
        return 0x0278;
        break;
    case WINDOWS_1903:
        return 0x0280;
        break;
    case WINDOWS_1909:
        return 0x0280;
        break;
    case WINDOWS_2004:
        return 0x0388;
        break;
    case WINDOWS_20H2:
        return 0x0388;
        break;
    case WINDOWS_21H1:
        return 0x0388;
        break;
    default:
        return 0x0388;
    }
}

uintptr_t memory::get_proc_dirbase(PEPROCESS process, uintptr_t base)
{

    if (!process) return 0;

    uintptr_t process_dirbase = (uintptr_t)((PUCHAR)process + 0x28);
    if (process_dirbase == 0)
    {
        ULONG user_diroffset = get_dtb_offset();
        process_dirbase = (uintptr_t)((PUCHAR)process + user_diroffset);
    }
    if ((process_dirbase >> 0x38) == 0x40)
    {
        process_dirbase = pml4::dirbase_from_base_address((void*)base);
    }

    return process_dirbase;
}