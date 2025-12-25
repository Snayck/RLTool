#include "Core.hpp"
#include "../Includes.hpp"
#include <vector>
#include <cstring>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

CoreComponent::CoreComponent() : Component("Core", "Initializes globals, components, and modules.") { OnCreate(); }

CoreComponent::~CoreComponent() { OnDestroy(); }

void CoreComponent::OnCreate()
{
	MainThread = nullptr;
}

void CoreComponent::OnDestroy() {
	DestroyThread();
}

void CoreComponent::InitializeThread()
{
	MainThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(InitializeGlobals), nullptr, 0, nullptr);
}

void CoreComponent::DestroyThread()
{
	CloseHandle(MainThread);
}

bool CoreComponent::ValidateTArray(HANDLE hProcess, uintptr_t address) {
	if (!address) return false;

	struct TArrayHeader {
		void* Data;
		int32_t Num;
		int32_t Max;
	};

	TArrayHeader header;
	SIZE_T bytesRead = 0;

	if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &header, sizeof(TArrayHeader), &bytesRead)) {
		return false;
	}

	return (header.Data != nullptr && header.Num > 0 && header.Max > header.Num);
}

std::vector<uintptr_t> CoreComponent::FindPattern(HANDLE hProcess, uintptr_t baseAddress, SIZE_T moduleSize) {
	std::vector<uintptr_t> results;
	std::vector<unsigned char> buffer(moduleSize);
	SIZE_T bytesRead = 0;

	if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(baseAddress), buffer.data(), moduleSize, &bytesRead)) {
		return results;
	}

	unsigned char pattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
	const char* mask = "??????xx??xxxxxx";
	size_t maskLength = strlen(mask);

	for (size_t i = 0; i < bytesRead - maskLength; i++) {
		bool found = true;
		for (size_t j = 0; j < maskLength; j++) {
			if (mask[j] != '?' && buffer[i + j] != pattern[j]) {
				found = false;
				break;
			}
		}
		if (found) {
			results.push_back(baseAddress + i);
		}
	}

	return results;
}

uintptr_t CoreComponent::GetGNames() {
	HANDLE hProcess = GetCurrentProcess();
	HMODULE hModule = GetModuleHandleW(NULL);

	// Get module size
	MODULEINFO moduleInfo;
	if (!GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(moduleInfo))) {
		return 0;
	}

	uintptr_t baseAddress = reinterpret_cast<uintptr_t>(hModule);
	SIZE_T moduleSize = moduleInfo.SizeOfImage;

	std::vector<uintptr_t> matches = FindPattern(hProcess, baseAddress, moduleSize);

	// The pattern finds GNames, validate both GNames and GObjects (GNames + 0x48)
	for (uintptr_t candidate : matches) {
		if (ValidateTArray(hProcess, candidate) && ValidateTArray(hProcess, candidate + 0x48)) {
			return candidate; // Return GNames address
		}
	}

	// Fallback to first match if validation fails
	return matches.empty() ? 0 : matches[0];
}

uintptr_t CoreComponent::GetGObjects() {
	uintptr_t gnames = GetGNames();
	return gnames ? gnames + 0x48 : 0;
}

void CoreComponent::InitializeGlobals(HMODULE hModule)
{
	Console.Initialize(std::filesystem::current_path(), "FunctionLogger.log");

	GObjects = reinterpret_cast<TArray<UObject*>*>(GetGObjects());
	GNames = reinterpret_cast<TArray<FNameEntry*>*>(GetGObjects() - 0x48);

	if (AreGlobalsValid())
	{
		Console.Notify("[Core Module] Entry Point " + Format::ToHex(reinterpret_cast<void*>(GetModuleHandle(NULL))));
		Console.Notify("[Core Module] Global Objects: " + Format::ToHex(GObjects));
		Console.Notify("[Core Module] Global Names: " + Format::ToHex(GNames));
		Console.Write("[Core Module] Initialized!");

		void** UnrealVTable = reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Dummy);
		EventsComponent::AttachProcessEventDetour(reinterpret_cast<ProcessEventType>(UnrealVTable[67]));
		EventsComponent::AttachCallFunctionDetour(reinterpret_cast<CallFunctionType>(UnrealVTable[76]));

		Instances.Initialize();
		Main.Initialize();
		Events.Initialize();
		GUI.Initialize();
	}
	else
	{
		Console.Error("[Core Module] GObject and GNames are not valid, wrong address detected!");
	}
}

bool CoreComponent::AreGlobalsValid()
{
	return (AreGObjectsValid() && AreGNamesValid());
}

bool CoreComponent::AreGObjectsValid()
{
	if (GObjects
		&& UObject::GObjObjects()->Num() > 0
		&& UObject::GObjObjects()->Max() > UObject::GObjObjects()->Num())
	{
		return true;
	}

	return false;
}

bool CoreComponent::AreGNamesValid()
{
	if (GNames
		&& FName::Names()->Num() > 0
		&& FName::Names()->Max() > FName::Names()->Num())
	{
		return true;
	}

	return false;
}

class CoreComponent Core{};