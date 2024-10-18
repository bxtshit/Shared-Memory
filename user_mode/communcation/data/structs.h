#pragma once
#include <string>
#include <Windows.h>

typedef struct _read_invoke {
	char command[16];
	INT32 process_id;
	ULONGLONG address;
	ULONGLONG buffer;
	ULONGLONG size;
} read_invoke, * pread_invoke;

typedef struct _base_invoke {
	char command[32];
	INT32 process_id;
	uintptr_t address;
} base_invoke, * pbase_invoke;

typedef struct _gr_invoke {
	uintptr_t address;
} ggr_invoke, * pggr_invoke;

typedef struct _dtb_invoke
{
	INT32 process_id;
	uintptr_t dtb;
} dtb_invoke, * pdtb_invoke;