#pragma once
#include <ntifs.h>
#include <windef.h>

#include <dependencies/structs.h>

static class base
{
public:
	static NTSTATUS get(pbase_invoke x);
};