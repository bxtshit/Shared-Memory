#include <iostream>
#include <communcation/shared_memory.h>

int main()
{
	sm_ptr.create_shared_events();

	printf("\ncreated shared events\n");

	Sleep(1000);

	if (!sm_ptr.open())
	{
		printf("\nfailed to open shared memory for requests\n");
	}
	else
	{
		printf("\nopened shared memory for requests\n");
	}

	sm_ptr.process_id = sm_ptr.get_process_id(L"FortniteClient-Win64-Shipping.exe"); //notepad
	
	printf("\nprocess_id: %d\n", sm_ptr.process_id);

	sm_ptr.module_base = sm_ptr.get_base(sm_ptr.process_id);

	printf("\nbase_address: %p\n", sm_ptr.module_base);

	auto offset_test = sm_ptr.read<uintptr_t>(sm_ptr.module_base + 0x10);

	printf("\noffset_test: %p\n", offset_test);

	std::cin.get();

}