#include "Core.hpp"
#include "../Includes.hpp"

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

uintptr_t CoreComponent::GetGObjects() {
	HANDLE CurrentBaseHandle = GetCurrentProcess();

	BYTE Bytes[4];
	uintptr_t Pattern;

	Pattern = Memory::FindPattern(GetModuleHandleW(NULL), (const unsigned char*)"\xE8\x00\x00\x00\x00\x8B\x5D\xAF", "x????xxx") + 1;
	ReadProcessMemory(CurrentBaseHandle, (LPCVOID)Pattern, Bytes, sizeof(Bytes), NULL);
	Pattern += Memory::BytesToInt32(Bytes, 0) + 0x69;
	ReadProcessMemory(CurrentBaseHandle, (LPCVOID)(Pattern + 3), Bytes, sizeof(Bytes), NULL);
	Pattern += Memory::BytesToInt32(Bytes, 0) + 7;

	return Pattern;
}

uintptr_t CoreComponent::GetGNames() {
	HANDLE CurrentBaseHandle = GetCurrentProcess();

	BYTE Bytes[4];
	uintptr_t Pattern;

	Pattern = Memory::FindPattern(GetModuleHandleW(NULL), (const unsigned char*)"\xE8\x00\x00\x00\x00\x48\xC7\xC7", "x????xxx") + 1;
	ReadProcessMemory(CurrentBaseHandle, (LPCVOID)Pattern, Bytes, sizeof(Bytes), NULL);
	Pattern += Memory::BytesToInt32(Bytes, 0) + 0x33;
	ReadProcessMemory(CurrentBaseHandle, (LPCVOID)(Pattern + 3), Bytes, sizeof(Bytes), NULL);
	Pattern += Memory::BytesToInt32(Bytes, 0) + 7;

	return Pattern;
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