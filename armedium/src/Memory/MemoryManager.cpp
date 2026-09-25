#include "MemoryManager.h"
#include <vector>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

int32_t MemoryManager::getProcessId(const std::string& processName) {
	uint32_t processId = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (snapshot == INVALID_HANDLE_VALUE) {
		return processId;
	}

	PROCESSENTRY32 processEntry{};
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &processEntry)) {
		do {
			if (!_stricmp(processName.c_str(), processEntry.szExeFile)) {
				processId = processEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &processEntry));
	}

	CloseHandle(snapshot);
	return processId;
}

uintptr_t MemoryManager::getModuleAddress(const std::string& moduleName) {
	uintptr_t moduleAddress = 0;

	if (!processHandle) {
		return moduleAddress;
	}

	DWORD processId = GetProcessId(processHandle);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);

	if (snapshot == INVALID_HANDLE_VALUE) {
		return moduleAddress;
	}

	MODULEENTRY32 moduleEntry{};
	moduleEntry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &moduleEntry)) {
		do {
			if (!_stricmp(moduleName.c_str(), moduleEntry.szModule)) {
				moduleAddress = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
				break;
			}
		} while (Module32Next(snapshot, &moduleEntry));
	}

	CloseHandle(snapshot);
	return moduleAddress;
}

bool MemoryManager::attachToProcess(const std::string& processName)
{
	auto pid = getProcessId(processName);
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

	if (process == INVALID_HANDLE_VALUE || !process) {
		return false;
	}

	processHandle = process;
	processId = pid;

	baseAddress = getModuleAddress(processName);

	return true;
}


void MemoryManager::readRaw(uintptr_t address, void* buffer, uintptr_t size) {
	Luck_ReadVirtualMemory(processHandle, reinterpret_cast<void*>(address), &buffer, size, nullptr);
}

std::string MemoryManager::readString(uintptr_t address) {
	std::string result;
	char character;
	int offset = 0;

	if (address == 0) {
		return result;
	}

	int32_t StrLength = read<int32_t>(address + 0x18);

	if (StrLength >= 16) {
		address = read<uintptr_t>(address);
	}

	// Cap the null-walk: a stale/freed object after a server transition can
	// point at a mapped region with no null terminator in sight, letting this
	// loop grow the string unboundedly until bad_alloc -> std::terminate.
	while (offset < 1024 && (character = read<char>(address + offset)) != 0)
	{
		result.push_back(character);
		offset += sizeof(character);
	}

	return result;
}

int32_t MemoryManager::getProcessId() {
	return processId;
}

void MemoryManager::setProcessId(int32_t newProcessId) {
	processId = newProcessId;
}

uintptr_t MemoryManager::getBaseAddress() {
	return baseAddress;
}

std::string MemoryManager::getProcessImagePath() {
	char path[MAX_PATH] = {};
	if (!processHandle)
		return {};
	if (!GetModuleFileNameExA(processHandle, NULL, path, MAX_PATH))
		return {};
	return std::string(path);
}

// Extracts "version-xxxxxxxxxxxxxxxx" from a client install path such as
// ...\Versions\version-2366ba214ec740ca\RobloxPlayerBeta.exe.
// Works for stock installs and bootstrappers (Bloxstrap/Froststrap keep the
// same version-folder layout). Returns "" when not found.
std::string ParseRobloxClientVersion(const std::string& imagePath) {
	const std::string tag = "version-";
	size_t pos = imagePath.find(tag);
	while (pos != std::string::npos) {
		if (pos + tag.size() + 16 <= imagePath.size()) {
			bool hex = true;
			for (size_t i = 0; i < 16; i++) {
				char c = imagePath[pos + tag.size() + i];
				if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
				{ hex = false; break; }
			}
			if (hex)
				return imagePath.substr(pos, tag.size() + 16);
		}
		pos = imagePath.find(tag, pos + 1);
	}
	return {};
}

void MemoryManager::setBaseAddress(uintptr_t newBaseAddress) {
	baseAddress = newBaseAddress;
}