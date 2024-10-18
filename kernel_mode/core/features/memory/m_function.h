#pragma once
#include <ntifs.h>
#include <windef.h>
#include <dependencies/structs.h>

static class memory
{
public:
	static NTSTATUS read_memory(pread_invoke x);
	//static NTSTATUS read(PVOID target_address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read);
	auto get_dtb_offset();
	uintptr_t get_proc_dirbase(PEPROCESS process, uintptr_t base);

	static UINT_PTR find_guarded_region(pggr_invoke x);
};

#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19569
#define WINDOWS_21H1 20180