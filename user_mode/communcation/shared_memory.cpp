#include <communcation/shared_memory.h>
#include <AccCtrl.h>
#include <AclAPI.h>
#include <TlHelp32.h>
#include <iostream>

void shared_memory::create_shared_events()
{
	DWORD dw_res;
	SECURITY_ATTRIBUTES sec_attr;
	PSECURITY_DESCRIPTOR p_sec_desc = NULL;
	SID_IDENTIFIER_AUTHORITY sid_auth_world = SECURITY_WORLD_SID_AUTHORITY;
	PACL p_acl = NULL;
	PSID p_everyone_sid = NULL;
	EXPLICIT_ACCESS explicit_access[1];

	AllocateAndInitializeSid(
		&sid_auth_world,
		1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&p_everyone_sid);

	ZeroMemory(&explicit_access, sizeof(EXPLICIT_ACCESS));
	explicit_access[0].grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
	explicit_access[0].grfAccessMode = SET_ACCESS;
	explicit_access[0].grfInheritance = NO_INHERITANCE;
	explicit_access[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	explicit_access[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	explicit_access[0].Trustee.ptstrName = (LPTSTR)p_everyone_sid;

	dw_res = SetEntriesInAcl(1, explicit_access, NULL, &p_acl);

	p_sec_desc = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

	InitializeSecurityDescriptor(p_sec_desc, SECURITY_DESCRIPTOR_REVISION);

	SetSecurityDescriptorDacl(p_sec_desc, TRUE, p_acl, FALSE);

	sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	sec_attr.lpSecurityDescriptor = p_sec_desc;
	sec_attr.bInheritHandle = FALSE;

	shared_event_data_arrived = CreateEventA(&sec_attr, TRUE, FALSE, "Global\\DataArrived");
	shared_event_trigger = CreateEventA(&sec_attr, TRUE, FALSE, "Global\\trigger");
	shared_event_ready_to_read = CreateEventA(&sec_attr, TRUE, FALSE, "Global\\ReadyRead");
}

bool shared_memory::open()
{
	map_file_write = OpenFileMappingA(FILE_MAP_WRITE, FALSE, "Global\\SharedMem");
	if (!map_file_write) { return false; }

	map_file_read = OpenFileMappingA(FILE_MAP_READ, FALSE, "Global\\SharedMem");
	if (!map_file_read) { return false; }

	return true;
}

INT32 shared_memory::get_process_id(LPCTSTR process_name)
{
	PROCESSENTRY32 pt;
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pt.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hsnap, &pt))
	{
		do
		{
			if (!lstrcmpi(pt.szExeFile, process_name))
			{
				CloseHandle(hsnap);
				process_id = pt.th32ProcessID;
				return pt.th32ProcessID;
			}
		} while (Process32Next(hsnap, &pt));
	}
	CloseHandle(hsnap);

	return { NULL };
}

bool shared_memory::write_to_shared_memory(const void* data, SIZE_T data_size)
{
	LPVOID pBuf = MapViewOfFile(map_file_write, FILE_MAP_WRITE, 0, 0, 0);
	if (pBuf == NULL)
	{
		std::cout << "\nfailed to map view of file: \n" << GetLastError() << std::endl;
		return false;
	}

	memcpy(pBuf, data, data_size);

	UnmapViewOfFile(pBuf);

	return true;
}

uintptr_t shared_memory::get_base(INT32 pid)
{
	base_invoke request_base;
	strcpy_s(request_base.command, "get_base");
	uintptr_t image_base = NULL;
	request_base.process_id = pid;
	request_base.address = image_base;

	if (!write_to_shared_memory(&request_base, sizeof(base_invoke)))
	{
		std::cout << "\nfailed to write request to shared memory\n" << std::endl;
		return 0;
	}

	SetEvent(shared_event_trigger);
	WaitForSingleObject(shared_event_ready_to_read, INFINITE);

	LPVOID pBuf = MapViewOfFile(map_file_read, FILE_MAP_READ, 0, 0, sizeof(base_invoke));
	if (pBuf == NULL)
	{
		std::cout << "\nfailed to map view of file: \n" << GetLastError() << std::endl;
		return 0;
	}

	base_invoke* response_base = reinterpret_cast<base_invoke*>(pBuf);

	image_base = response_base->address;

	UnmapViewOfFile(pBuf);

	return image_base;
}

bool shared_memory::read_memory(uintptr_t address, PVOID buffer, DWORD size)
{
	read_invoke request_read;
	strcpy_s(request_read.command, "read");
	request_read.address = address;
	request_read.size = size;

	if (!write_to_shared_memory(&request_read, sizeof(read_invoke)))
	{
		std::cerr << "Failed to write request to shared memory" << std::endl;
		return false;
	}

	SetEvent(shared_event_trigger);
	WaitForSingleObject(shared_event_ready_to_read, INFINITE);

	LPVOID pBuf = MapViewOfFile(map_file_read, FILE_MAP_READ, 0, 0, sizeof(base_invoke));
	if (pBuf == NULL)
	{
		std::cerr << "Failed to map view of file: " << GetLastError() << std::endl;
		return false;
	}

	read_invoke* response = reinterpret_cast<read_invoke*>(pBuf);

	buffer = (PVOID)response->buffer;

	UnmapViewOfFile(pBuf);

	return true;
}