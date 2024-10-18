#include <core/data/receiver.h>
#include <core/data/shared_memory/functions.h>
#include <core/features/memory/m_function.h>
#include <dependencies/spoofing/callstack.h>
#include <core/features/base/b_function.h>

extern "C" PVOID __fastcall PsDereferenceKernelStack();

void hide_thread(PKTHREAD Thread)
{
	ULONG_PTR initialStackOffset = 0x28;
	ULONG_PTR stackReferenceOffset = 0;

	if (!stackReferenceOffset) {
		stackReferenceOffset = *(ULONG*)((ULONG_PTR)PsDereferenceKernelStack + 0x10);
	}

	PVOID initialStack = *(PVOID*)((ULONG_PTR)Thread + initialStackOffset);
	ULONG kernelStackReference = *(ULONG*)((ULONG_PTR)Thread + stackReferenceOffset);

	*(PVOID*)((ULONG_PTR)Thread + initialStackOffset) = 0;
	*(ULONG*)((ULONG_PTR)Thread + stackReferenceOffset) = 0;

	*(PVOID*)((ULONG_PTR)Thread + initialStackOffset) = initialStack;
	*(ULONG*)((ULONG_PTR)Thread + stackReferenceOffset) = kernelStackReference;
}

void process_read_command()
{
    read_invoke* req = (read_invoke*)shared_section;
    memory::read_memory(req);
    if (0 == memcpy(shared_section, req, sizeof(read_invoke))) {
        DbgPrint("failed to copy memory data (read)");
    }
}

void process_dtb_command()
{

}

void process_get_base_command()
{
    dtb_invoke* req = (dtb_invoke*)shared_section;
}

void data::receive()
{
    while (true)
    {
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = RELATIVE(SECONDS(1));

        DbgPrint("waiting for command...");
        NTSTATUS status = KeWaitForSingleObject(shared_event_trigger, Executive, KernelMode, FALSE, &Timeout);
        if (status == STATUS_SUCCESS)
        {
            DbgPrint("event triggered, reading shared memory");
            sm_pointer.read();

            const char* command = (PCHAR)shared_section;
            DbgPrint("received command: %s", command);

            if (command != nullptr)
            {
                if (strcmp(command, "get_base") == 0)
                {
                    DbgPrint("processing get_base command");
                    process_get_base_command();
                }
                else if (strcmp(command, "read") == 0)
                {
                    process_read_command();
                }
                else
                {
                    DbgPrint("unknown command: %s", command);
                }
            }
            else
            {
                DbgPrint("command is null");
            }

            KeSetEvent(shared_event_ready_read, 0, FALSE);
        }
    }
}