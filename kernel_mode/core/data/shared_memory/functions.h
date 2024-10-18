#pragma once
#include <ntdef.h>
#include <ntifs.h>

inline const WCHAR shared_section_name[] = L"\\BaseNamedObjects\\SharedMem";

inline SECURITY_DESCRIPTOR sec_descriptor;
inline HANDLE section_handle;
inline PVOID shared_section = NULL;
inline PVOID shared_output_var = NULL;
inline ULONG dacl_length;
inline PACL dacl;

inline HANDLE  shared_event_handle_trigger = NULL;
inline PKEVENT shared_event_trigger = NULL;
inline UNICODE_STRING event_name_trigger;

inline HANDLE  shared_event_handle_ready_read = NULL;
inline PKEVENT shared_event_ready_read = NULL;
inline UNICODE_STRING event_name_ready_read;

inline HANDLE  shared_event_handle_dt = NULL;
inline PKEVENT shared_event_dt = NULL;
inline UNICODE_STRING event_name_dt;

class shared_memory
{
public:
	NTSTATUS create();
	VOID open_events();
	VOID read();
};

inline shared_memory sm_pointer;