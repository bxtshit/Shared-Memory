#include <core/data/shared_memory/functions.h>
#include <dependencies/spoofing/callstack.h>

NTSTATUS shared_memory::create()
{
    hide;

    auto status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR sec_descriptor = ExAllocatePoolWithTag(PagedPool, sizeof(SECURITY_DESCRIPTOR), 'SDsc');
    if (sec_descriptor == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateSecurityDescriptor(sec_descriptor, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(sec_descriptor);
        return status;
    }

    dacl_length = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) * 3 +
        RtlLengthSid(SeExports->SeLocalSystemSid) +
        RtlLengthSid(SeExports->SeAliasAdminsSid) +
        RtlLengthSid(SeExports->SeWorldSid);

    dacl = (PACL)ExAllocatePoolWithTag(PagedPool, dacl_length, 'lcaD');
    if (dacl == NULL) {
        ExFreePool(sec_descriptor);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl(dacl, dacl_length, ACL_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(dacl);
        ExFreePool(sec_descriptor);
        return status;
    }

    status = RtlAddAccessAllowedAce(dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeWorldSid);
    status = NT_SUCCESS(status) ? RtlAddAccessAllowedAce(dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeAliasAdminsSid) : status;
    status = NT_SUCCESS(status) ? RtlAddAccessAllowedAce(dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeLocalSystemSid) : status;

    if (!NT_SUCCESS(status)) {
        ExFreePool(dacl);
        ExFreePool(sec_descriptor);
        return status;
    }

    status = RtlSetDaclSecurityDescriptor(sec_descriptor, TRUE, dacl, FALSE);
    if (!NT_SUCCESS(status)) {
        ExFreePool(dacl);
        ExFreePool(sec_descriptor);
        return status;
    }

    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING sectionName;
    RtlInitUnicodeString(&sectionName, shared_section_name);
    InitializeObjectAttributes(&objAttr, &sectionName, OBJ_CASE_INSENSITIVE, NULL, sec_descriptor);

    LARGE_INTEGER lMaxSize = { 0 };
    lMaxSize.LowPart = 10240;
    status = ZwCreateSection(&section_handle, SECTION_ALL_ACCESS, &objAttr, &lMaxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        ExFreePool(dacl);
        ExFreePool(sec_descriptor);
        return status;
    }

    SIZE_T ulViewSize = 10240;
    status = ZwMapViewOfSection(section_handle, NtCurrentProcess(), &shared_section, 0, ulViewSize, NULL, &ulViewSize, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);
    if (!NT_SUCCESS(status)) {
        ZwClose(section_handle);
        ExFreePool(dacl);
        ExFreePool(sec_descriptor);
        return status;
    }

    ExFreePool(dacl);
    ExFreePool(sec_descriptor);
    return status;
}

void shared_memory::read()
{
    if (shared_section) {
        auto unmapping_status = ZwUnmapViewOfSection(NtCurrentProcess(), shared_section);
        if (unmapping_status != STATUS_SUCCESS) {
            DbgPrint("Failed to unmap view of section: %08x", unmapping_status);
        }
        shared_section = NULL;
    }

    SIZE_T ul_view_size = 1024 * 10;
    auto nt_status = ZwMapViewOfSection(section_handle, NtCurrentProcess(), &shared_section, 0, ul_view_size, NULL, &ul_view_size, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);

    if (nt_status != STATUS_SUCCESS) {
        DbgPrint("Failed to map view of section: %08x", nt_status);
        ZwClose(section_handle);
        section_handle = NULL;
    }
}

void shared_memory::open_events()
{
    hide;

    RtlInitUnicodeString(&event_name_dt, L"\\BaseNamedObjects\\DataArrived");
    RtlInitUnicodeString(&event_name_trigger, L"\\BaseNamedObjects\\trigger");
    RtlInitUnicodeString(&event_name_ready_read, L"\\BaseNamedObjects\\ReadyRead");

    shared_event_dt = IoCreateNotificationEvent(&event_name_dt, &shared_event_handle_dt);
    if (!shared_event_dt) {
        DbgPrint("shared_event_dt failed");
    }

    shared_event_trigger = IoCreateNotificationEvent(&event_name_trigger, &shared_event_handle_trigger);
    if (!shared_event_trigger) {
        DbgPrint("shared_event_trigger failed");
    }

    shared_event_ready_read = IoCreateNotificationEvent(&event_name_ready_read, &shared_event_handle_ready_read);
    if (!shared_event_ready_read)
    {
        DbgPrint("shared_event_ready_read failed");
    }
}