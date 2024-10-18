#pragma once
#include <communcation/data/structs.h>

inline HANDLE map_file_write;
inline HANDLE map_file_read;
inline HANDLE g_mutex;

inline HANDLE shared_event_data_arrived;
inline HANDLE shared_event_trigger;
inline HANDLE shared_event_ready_to_read;

class shared_memory
{
public:

	INT32 process_id;

	uintptr_t module_base;

	INT32 get_process_id(LPCTSTR process_name);

	bool write_to_shared_memory(const void* data, SIZE_T data_size);

	uintptr_t get_base(INT32 pid);

	bool read_memory(uintptr_t address, PVOID buffer, DWORD size);

	template <typename T>
	T read(uint64_t address)
	{
		T buffer{ };
		read_memory(address, &buffer, sizeof(T));
		return buffer;
	}

	void create_shared_events();

	bool open();
};
inline shared_memory sm_ptr;