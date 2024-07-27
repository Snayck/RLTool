#include "Events.hpp"
#include "../Includes.hpp"


PreEvent::PreEvent() : PE_Caller(nullptr), PE_Function(nullptr), PE_Params(nullptr), PE_Detour(true) {}

PreEvent::PreEvent(class UObject* caller, class UFunction* function, void* params) : PE_Caller(caller), PE_Function(function), PE_Params(params), PE_Detour(true) {}

PreEvent::~PreEvent() {}

class UObject* PreEvent::Caller() const
{
	return PE_Caller;
}

template <typename T> T* PreEvent::GetCaller() const
{
	if (PE_Caller)
	{
		return static_cast<T*>(PE_Caller);
	}

	return nullptr;
}

class UFunction* PreEvent::Function() const
{
	return PE_Function;
}

void* PreEvent::Params() const
{
	return PE_Params;
}

template <typename T> T* PreEvent::GetParams() const
{
	if (PE_Params)
	{
		return reinterpret_cast<T*>(PE_Params);
	}

	return nullptr;
}

bool PreEvent::ShouldDetour() const
{
	if (PE_Function && EventsComponent::IsEventBlacklisted(PE_Function->ObjectInternalInteger))
	{
		return false;
	}

	return PE_Detour;
}

void PreEvent::SetDetour(bool bDetour)
{
	PE_Detour = bDetour;
}

PreEvent PreEvent::operator=(const PreEvent& other)
{
	PE_Caller = other.PE_Caller;
	PE_Function = other.PE_Function;
	PE_Params = other.PE_Params;
	PE_Detour = other.PE_Detour;
	return *this;
}

PostEvent::PostEvent() : PE_Result(nullptr) {}

PostEvent::PostEvent(class UObject* caller, class UFunction* function, void* params, void* result) : PreEvent(caller, function, params), PE_Result(result) {}

PostEvent::~PostEvent() {}

void* PostEvent::Result() const
{
	return PE_Result;
}

template <typename T> T* PostEvent::GetResult() const
{
	if (PE_Result)
	{
		return reinterpret_cast<T*>(PE_Result);
	}

	return nullptr;
}

PostEvent PostEvent::operator=(const PostEvent& other)
{
	PE_Caller = other.PE_Caller;
	PE_Function = other.PE_Function;
	PE_Params = other.PE_Params;
	PE_Detour = other.PE_Detour;
	PE_Result = other.PE_Result;
	return *this;
}

std::vector<AVehiclePickup_Boost_TA*> boosts;

namespace Hooks
{


	void ExecuteFunctions(PreEvent& event)
	{
		for (std::function<void()> Func : MainComponent::GameFunctions) {
			if (Func) {
				Func();
			}
		
		}
		MainComponent::GameFunctions.clear();
	}

	
	void PriReplicatedEvent(PreEvent& event)
	{

		APRI_TA_eventReplicatedEvent_Params* Params = event.GetParams<APRI_TA_eventReplicatedEvent_Params>();
		APRI_TA* Pri = event.GetCaller<APRI_TA>();

		if (Params && Pri)
		{
			if (Pri->IsLocalPlayerPRI())
			{
				
			}
		}
	}

	void UpdatePriData(PreEvent& event)
	{
		if (event.Caller())
		{
			if (UGFxData_PRI_TA* Pri = event.GetCaller<UGFxData_PRI_TA>())
			{
				
			}
		}
	}
	

	
	void HUDPostRender(PreEvent& event)
	{
		if (event.Caller())
		{
			AHUD* Hud = event.GetCaller<AHUD>();
			Instances.SetHUD(Hud);

			if (Instances.IAWorldInfo() == nullptr || Instances.IAWorldInfo()->PawnList == nullptr || Instances.IAPlayerController() == nullptr || Instances.IAPlayerController()->Pawn == nullptr || Instances.IAWorldInfo()->WorldPostProcessChain == nullptr)
				return;
			Hud->Canvas->SetDrawColor(255, 255, 255, 255);
			FFontRenderInfo font;
			font.bEnableShadow = 1;
			
			for (APawn* Pawn = Instances.IAWorldInfo()->PawnList; Pawn; Pawn = Pawn->NextPawn) {
				/*FVector pos = Drawing::CalculateScreenCoordinate(Pawn->Location);
				Hud->Canvas->SetPos(pos.X, pos.Y, pos.Z);
				Hud->Canvas->DrawTextW(Format::ToFString(Pawn->GetName()), 1, 1, 1, font);*/
			}
		}

		GRainbowColor::OnTick();
	}

	void GameViewPortPostRender(PreEvent& event)
	{
		if (event.Caller())
		{
			Instances.SetGameViewportClient(event.GetCaller<UGameViewportClient>());
		}
	}

	void PlayerControllerTick(PreEvent& event)
	{
		if (event.Caller())
		{
			Instances.SetPlayerController(event.GetCaller<APlayerController>());
		}
	}

	void GameViewPortKeyPress(const PostEvent& event)
	{
		if (event.Params())
		{
			UGameViewportClient_TA_execHandleKeyPress_Params* Params = event.GetParams<UGameViewportClient_TA_execHandleKeyPress_Params>();
			if (Params->EventType == static_cast<uint8_t>(EInputEvent::IE_Released))
			{
				std::string KeyName = Params->Key.ToString();

				if (KeyName == "F5")
				{
					GUI.IsOpen = !GUI.IsOpen;
				}
			}
		}
	}

	void GFxDataMainMenuAdded(PreEvent& event)
	{
		GameState.MainMenuAdded();
	}
}

EventsComponent::EventsComponent() : Component("Events", "Manages function hooks and process event.") { OnCreate(); }

EventsComponent::~EventsComponent() { OnDestroy(); }

void EventsComponent::OnCreate()
{
	ProcessEventDetoured = false;
	ProcessInternalDetoured = false;
	CallFunctionDetoured = false;

	ProcessEvent = nullptr;
	ProcessInternal = nullptr;
	CallFunction = nullptr;

	ProcessInternalToBeHooked = true;

	LastPreFunction = INT_MAX;
	LastPostFunction = INT_MAX;
}

void EventsComponent::OnDestroy()
{
	DetachAllDetours();
}

bool EventsComponent::IsProcessEventDetoured()
{
	return (ProcessEventDetoured && ProcessEvent);
}

bool EventsComponent::IsProcessInternalDetoured()
{
	return (ProcessInternalDetoured && ProcessInternal);
}

bool EventsComponent::IsCallFunctionDetoured()
{
	return (CallFunctionDetoured && CallFunction);
}

void EventsComponent::HookProcessInternal(class UFunction* Function)
{
	ProcessInternalToBeHooked = false;
	AttachProcessInternalDetour(reinterpret_cast<ProcessInternalType>(Function->Func.Dummy));
}

void EventsComponent::AttachProcessEventDetour(const ProcessEventType& detourFunction)
{
	if (!IsProcessEventDetoured())
	{
		ProcessEvent = detourFunction;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&reinterpret_cast<PVOID&>(ProcessEvent), reinterpret_cast<PVOID>(ProcessEventDetour));
		DetourTransactionCommit();
		ProcessEventDetoured = true;

#if defined _DEBUG
		Console.Notify("[Events Component] ProcessEvent Attached");
#endif
	}
	else
	{
#if defined _DEBUG
		Console.Error("[Events Component] ProcessEvent has already attached");
#endif
	}
}

void EventsComponent::AttachProcessInternalDetour(const ProcessInternalType& detourFunction)
{
	if (!IsProcessInternalDetoured())
	{
		ProcessInternal = detourFunction;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&reinterpret_cast<PVOID&>(ProcessInternal), reinterpret_cast<PVOID>(ProcessInternalDetour));
		DetourTransactionCommit();
		ProcessInternalDetoured = true;

#if defined _DEBUG
		Console.Notify("[Events Component] ProcessInternal Attached");
#endif
	}
	else
	{
#if defined _DEBUG
		Console.Error("[Events Component] ProcessInternal has already attached");
#endif
	}
}

void EventsComponent::AttachCallFunctionDetour(const CallFunctionType& detourFunction)
{
	if (!IsCallFunctionDetoured())
	{
		CallFunction = detourFunction;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&reinterpret_cast<PVOID&>(CallFunction), reinterpret_cast<PVOID>(CallFunctionDetour));
		DetourTransactionCommit();
		CallFunctionDetoured = true;

#if defined _DEBUG
		Console.Notify("[Events Component] CallFunction Attached");
#endif
	}
	else
	{
#if defined _DEBUG
		Console.Error("[Events Component] CallFunction has already attached");
#endif
	}
}

void EventsComponent::DetachProcessEventDetour()
{
	if (IsProcessEventDetoured())
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&reinterpret_cast<PVOID&>(ProcessEvent), reinterpret_cast<PVOID>(ProcessEventDetour));
		DetourTransactionCommit();
		ProcessEventDetoured = false;
		ProcessEvent = nullptr;

#if defined _DEBUG
		Console.Notify("[Events Component] ProcessEvent Detached");
#endif
	}
	else
	{
#if defined _DEBUG
		Console.Error("[Events Component] ProcessEvent has already been detached");
#endif
	}
}

void EventsComponent::DetachProcessInternalDetour()
{
	if (IsProcessInternalDetoured())
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&reinterpret_cast<PVOID&>(ProcessInternal), reinterpret_cast<PVOID>(ProcessInternalDetour));
		DetourTransactionCommit();
		ProcessInternalDetoured = false;
		ProcessInternal = nullptr;

#if defined _DEBUG
		Console.Notify("[Events Component] ProcessInternal Detached");
#endif
	}
	else
	{
#if defined _DEBUG
		Console.Error("[Events Component] ProcessInternal has already been detached");
#endif
	}
}

void EventsComponent::DetachCallFunctionDetour()
{
	if (IsCallFunctionDetoured())
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&reinterpret_cast<PVOID&>(CallFunction), reinterpret_cast<PVOID>(CallFunctionDetour));
		DetourTransactionCommit();
		CallFunctionDetoured = false;
		CallFunction = nullptr;

#if defined _DEBUG
		Console.Notify("[Events Component] CallFunction Detached");
#endif
	}
	else
	{
#if defined _DEBUG
		Console.Error("[Events Component] CallFunction has already been detached");
#endif
	}
}

void EventsComponent::AttachAllDetours(uint8_t ProcessEventIndex, uint8_t CallFunctionIndex)
{
	void** UnrealVTable = reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Dummy);

	EventsComponent::AttachProcessEventDetour(reinterpret_cast<ProcessEventType>(UnrealVTable[ProcessEventIndex]));
	EventsComponent::AttachCallFunctionDetour(reinterpret_cast<CallFunctionType>(UnrealVTable[CallFunctionIndex]));
	EventsComponent::HookProcessInternal();
}

void EventsComponent::DetachAllDetours()
{
	DetachProcessEventDetour();
	DetachProcessInternalDetour();
	DetachCallFunctionDetour();
}




std::string removeProperty(const std::string& input) {
	std::string output = input;
	size_t pos = 0;
	while ((pos = output.find(_("Property"), pos)) != std::string::npos) {
		output.erase(pos, 8); // Length of "Property" is 8
	}
	return output;
}



std::string processValue(UProperty* prop, void* params) {
	if (prop->PropertyFlags & 0x400 || prop->PropertyFlags & 0x100) {}
	else {
		if (prop->IsA<UStructProperty>()) {

			std::string w = "{ ";
			for (UField* field = ((UStructProperty*)prop)->Struct->Children; field; field = field->Next) {
				std::string item = (processValue(reinterpret_cast<UProperty*>(field), params) + ", ").c_str();
				for (auto c : item)
					w.push_back(c);
			}
			w.pop_back(); w.pop_back();
			w.push_back(' '); w.push_back('}');

			return w;
		}
		else if (prop->IsA<UStrProperty>()) {
			FString* valueAddress = (FString*)((uintptr_t)params + prop->Offset);
			if (params)
				return "'" + (*valueAddress).ToString() + "'";
		}
		else if (prop->IsA<UQWordProperty>()) {
			uint64_t* valueAddress = (uint64_t*)((uintptr_t)params + prop->Offset);
			if (params)
				return std::to_string(*valueAddress);
		}
		else if (prop->IsA<UObjectProperty>()) {
			int* ptr = reinterpret_cast<int*>( ((UObjectProperty*)prop)->PropertyClass );
			std::stringstream ss; ss << ptr;
			return "'0x" + ss.str() + "'";
		}
		else if (prop->IsA<UComponentProperty>()) {

		}
		else if (prop->IsA<UClassProperty>()) {

		}
		else if (prop->IsA<UNameProperty>()) {
			FName* valueAddress = (FName*)((uintptr_t)params + prop->Offset);
			if (params)
				return "'" + (*valueAddress).ToString() + "'";
		}
		else if (prop->IsA<UMapProperty>()) {

		}
		else if (prop->IsA<UIntProperty>()) {
			int* valueAddress = (int*)((uintptr_t)params + prop->Offset);
			if (params)
				return std::to_string(*valueAddress);
		}
		else if (prop->IsA<UInterfaceProperty>()) {

		}
		else if (prop->IsA<UFloatProperty>()) {
			float* valueAddress = (float*)((uintptr_t)params + prop->Offset);
			if (params)
				return std::to_string(*valueAddress);
		}
		else if (prop->IsA<UDelegateProperty>()) {

		}
		else if (prop->IsA<UByteProperty>()) {

		}
		else if (prop->IsA<UBoolProperty>()) {
			BOOL* valueAddress = (BOOL*)((uintptr_t)params + prop->Offset);
			if (params) {
				BYTE Mask = ((UBoolProperty*)prop)->BitMask;
				int w = (*valueAddress & Mask);
				return std::to_string(w);
			}
		}
		else if (prop->IsA<UArrayProperty>()) {
			int* ptr = reinterpret_cast<int*>(((UArrayProperty*)prop)->Inner);
			std::stringstream ss; ss << ptr;
			return "'0x" + ss.str() + "'";
		}

		return _("NOT HANDLED");
	}
	return _("NOT PROCESSED");
}

std::string processType(UProperty* prop) {

	if (prop->IsA<UStructProperty>()) {
		return _("Struct ") + (((UStructProperty*)prop)->Struct)->GetName();
	}
	else if (prop->IsA<UStrProperty>()) {

	}
	else if (prop->IsA<UQWordProperty>()) {

	}
	else if (prop->IsA<UObjectProperty>()) {
		return _("Object ") + (((UObjectProperty*)prop)->PropertyClass)->GetName();
	}
	else if (prop->IsA<UComponentProperty>()) {

	}
	else if (prop->IsA<UClassProperty>()) {

	}
	else if (prop->IsA<UNameProperty>()) {

	}
	else if (prop->IsA<UMapProperty>()) {

	}
	else if (prop->IsA<UIntProperty>()) {

	}
	else if (prop->IsA<UInterfaceProperty>()) {

	}
	else if (prop->IsA<UFloatProperty>()) {

	}
	else if (prop->IsA<UDelegateProperty>()) {

	}
	else if (prop->IsA<UByteProperty>()) {

	}
	else if (prop->IsA<UBoolProperty>()) {

	}
	else if (prop->IsA<UArrayProperty>()) {
		std::string type = (processType(((UArrayProperty*)prop)->Inner));
		return _("Array<") + type + _(">");
	}

	return removeProperty(prop->Class->GetName());
}






std::vector<EventsComponent::func> EventsComponent::functions = {};
std::vector<std::string> blacklist = {};
std::vector<std::string> whitelist = {};

std::string toLowercase(const std::string& str) {
	std::string result;
	result.reserve(str.length());

	for (char c : str) {
		result.push_back(std::tolower(c));
	}

	return result;
}

bool containsSubstring(const std::string& str, const std::string& substr) {
	return str.find(substr) != std::string::npos;
}
std::vector<std::string> splitString(const std::string& input, char delimiter = ',') {
	std::vector<std::string> tokens;
	std::istringstream stream(input);
	std::string token;

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}
void addFunc(UObject* caller, UFunction* Function, void* paramsPointer) {
	EventsComponent::func funct;
	funct.functionData.function = *Function;
	funct.functionData.callerObject = *caller;

	std::vector<UProperty*> prop;

	for (UField* field = Function->Children; field; field = field->Next) {
		prop.push_back(reinterpret_cast<UProperty*>(field));
	}

	std::sort(prop.begin(), prop.end(), [](UProperty* PropertyA, UProperty* PropertyB) {
		if ((PropertyA->Offset == PropertyB->Offset) && PropertyA->IsA<UBoolProperty>() && PropertyB->IsA<UBoolProperty>())
		{
			return (static_cast<UBoolProperty*>(PropertyA)->BitMask < static_cast<UBoolProperty*>(PropertyB)->BitMask);
		}

		return (PropertyA->Offset < PropertyB->Offset);
		});

	for (UProperty* a : prop) {
		EventsComponent::prop p;
		p.Property = *a;
		p.value = processValue(a, paramsPointer);
		p.type = processType(a);
		funct.properties.push_back(p);
	}

	EventsComponent::functions.push_back(funct);
}

void handleFunc(UObject* caller, UFunction* Function, void* paramsPointer) {
	if (EventsComponent::functions.size() < 1000) {

		if (GUIComponent::whitelist != "") {
			std::vector<std::string> tokens = splitString(GUIComponent::whitelist);

			auto it = std::find_if(tokens.begin(), tokens.end(), [&](const std::string& str) {
				return containsSubstring(toLowercase(Function->Name.ToString()), toLowercase(str));
				});

			if (it != tokens.end()) {

			}
			else {
				return;
			}
		}

		if (GUIComponent::blacklist != "") {
			std::vector<std::string> tokens = splitString(GUIComponent::blacklist);

			auto it = std::find_if(tokens.begin(), tokens.end(), [&](const std::string& str) {
				return containsSubstring(toLowercase(Function->Name.ToString()), toLowercase(str));
				});

			if (it != tokens.end()) { 
				return; 
			}
			else {

			}
		}
		
		addFunc(caller, Function, paramsPointer);
		
	}
	else {
		
	}
}

void EventsComponent::ProcessEventDetour(class UObject* Caller, class UFunction* Function, void* Params, void* Result)
{
	if (ProcessEvent && Function)
	{
		if (GUIComponent::isLogging && GUIComponent::hookEvent)
			handleFunc(Caller, Function, Params);

		auto preIt = PreHookedEvents.find(Function->ObjectInternalInteger);
		PreEvent event(Caller, Function, Params);

		if (preIt != PreHookedEvents.end())
		{
			for (const auto& preEvent : preIt->second)
			{
				if (Function->ObjectInternalInteger != LastPreFunction)
				{
					LastPreFunction = Function->ObjectInternalInteger;
					preEvent(event);
					if (event.ShouldDetour())
					{
						ProcessEvent(Caller, Function, Params, Result);
					}
				}
				else
				{
					LastPreFunction = INT_MAX;
					if (event.ShouldDetour())
					{
						ProcessEvent(Caller, Function, Params, Result);
					}
				}
			}
		}
		else if (event.ShouldDetour())
		{
			ProcessEvent(Caller, Function, Params, Result);
		}

		auto postIt = PostHookedEvents.find(Function->ObjectInternalInteger);

		if (postIt != PostHookedEvents.end())
		{
			for (const auto& postEvent : postIt->second)
			{
				if (Function->ObjectInternalInteger != LastPostFunction)
				{
					LastPostFunction = Function->ObjectInternalInteger;
					postEvent(PostEvent(Caller, Function, Params, Result));
				}
				else
				{
					LastPostFunction = INT_MAX;
				}
			}
		}
	}
}

void EventsComponent::ProcessInternalDetour(class UObject* Caller, struct FFrame& Stack, RESULT_DECL)
{

	UFunction* Function = ((UFunction*)Stack.Node);
	uint8_t* Params = Stack.Locals;

	if (ProcessInternal && Caller && Function)
	{
		if (GUIComponent::isLogging && GUIComponent::hookInternal)
			handleFunc(Caller, Function, Params);

		auto preIt = PreHookedEvents.find(Function->ObjectInternalInteger);
		PreEvent event(Caller, Function, Params);

		if (preIt != PreHookedEvents.end())	
		{
			for (const auto& preEvent : preIt->second)
			{	
				if (Function->ObjectInternalInteger != LastPreFunction)
				{
					LastPreFunction = Function->ObjectInternalInteger;
					preEvent(event);
					if (event.ShouldDetour())
					{
						ProcessInternal(Caller, Stack, Result);
					}
				}
				else
				{
					LastPreFunction = INT_MAX;
					if (event.ShouldDetour())
					{
						ProcessInternal(Caller, Stack, Result);
					}
				}
			}
		}
		else if (event.ShouldDetour())
		{
			ProcessInternal(Caller, Stack, Result);
		}

		auto postIt = PostHookedEvents.find(Function->ObjectInternalInteger);

		if (postIt != PostHookedEvents.end())
		{
			for (const auto& postEvent : postIt->second)
			{
				if (Function->ObjectInternalInteger != LastPostFunction)
				{
					LastPostFunction = Function->ObjectInternalInteger;
					postEvent(PostEvent(Caller, Function, Params, Result));
				}
				else
				{
					LastPostFunction = INT_MAX;
				}
			}
		}
	}
}

void EventsComponent::CallFunctionDetour(class UObject* Caller, struct FFrame& Stack, RESULT_DECL, class UFunction* Function)
{
	if (ProcessInternalToBeHooked) { Events.HookProcessInternal(Function); }
	
	uint8_t* Params = Stack.Locals;

	if (CallFunction && Function && Result)
	{
		if (GUIComponent::isLogging && GUIComponent::hookCallFunc)
			handleFunc(Caller, Function, Params);

		auto preIt = PreHookedEvents.find(Function->ObjectInternalInteger);
		PreEvent event(Caller, Function, Params);

		if (preIt != PreHookedEvents.end())
		{
			for (const auto& preEvent : preIt->second)
			{
				if (Function->ObjectInternalInteger != LastPreFunction)
				{
					LastPreFunction = Function->ObjectInternalInteger;
					preEvent(event);
					if (event.ShouldDetour())
					{
						CallFunction(Caller, Stack, Result, Function);
					}
				}
				else
				{
					LastPreFunction = INT_MAX;
					if (event.ShouldDetour())
					{
						CallFunction(Caller, Stack, Result, Function);
					}
				}
			}
		}
		else if (event.ShouldDetour())
		{
			CallFunction(Caller, Stack, Result, Function);
		}

		auto postIt = PostHookedEvents.find(Function->ObjectInternalInteger);

		if (postIt != PostHookedEvents.end())
		{
			for (const auto& postEvent : postIt->second)
			{
				if (Function->ObjectInternalInteger != LastPostFunction)
				{
					LastPostFunction = Function->ObjectInternalInteger;
					postEvent(PostEvent(Caller, Function, Params, Result));
				}
				else
				{
					LastPostFunction = INT_MAX;
				}
			}
		}
	}
}

bool EventsComponent::IsEventBlacklisted(uint32_t functionInteger)
{
	return (std::find(BlacklistedEvents.begin(), BlacklistedEvents.end(), functionInteger) != BlacklistedEvents.end());
}

void EventsComponent::BlacklistEvent(const std::string& function)
{
	UFunction* foundFunction = Instances.FindStaticFunction(function);

	if (foundFunction)
	{
		if (!IsEventBlacklisted(foundFunction->ObjectInternalInteger))
		{
			BlacklistedEvents.emplace_back(foundFunction->ObjectInternalInteger);
			std::sort(BlacklistedEvents.begin(), BlacklistedEvents.end());
		}
	}
	else
	{
		Console.Warning(GetNameFormatted() + "Warning: Failed to blacklist function \"" + function + "\"!");
	}
}

void EventsComponent::WhitelistEvent(const std::string& functionName)
{
	UFunction* foundFunction = Instances.FindStaticFunction(functionName);

	if (foundFunction)
	{
		std::vector<uint32_t>::iterator blackIt = std::find(BlacklistedEvents.begin(), BlacklistedEvents.end(), foundFunction->ObjectInternalInteger);

		if (blackIt != BlacklistedEvents.end())
		{
			BlacklistedEvents.erase(blackIt);
			std::sort(BlacklistedEvents.begin(), BlacklistedEvents.end());
		}
	}
	else
	{
		Console.Warning(GetNameFormatted() + "Warning: Failed to whitelist function \"" + functionName + "\"!");
	}
}

void EventsComponent::HookEventPre(const std::string& functionName, std::function<void(PreEvent&)> preHook)
{
	UFunction* foundFunction = Instances.FindStaticFunction(functionName);

	if (foundFunction)
	{
		HookEventPre(foundFunction->ObjectInternalInteger, preHook);
	}
	else
	{
		Console.Warning(GetNameFormatted() + "Warning: Failed to hook function \"" + functionName + "\"!");
	}
}

void EventsComponent::HookEventPre(uint32_t functionIndex, std::function<void(PreEvent&)> preHook)
{
	UObject* foundFunction = UObject::GObjObjects()->At(functionIndex);

	if (foundFunction && foundFunction->IsA(UFunction::StaticClass()))
	{
		if (PreHookedEvents.find(functionIndex) != PreHookedEvents.end())
		{
			PreHookedEvents[functionIndex].push_back(preHook);
		}
		else
		{
			PreHookedEvents[functionIndex] = std::vector<std::function<void(PreEvent&)>>{ preHook };
		}
	}
	else
	{
		Console.Warning(GetNameFormatted() + "Warning: Failed to hook function at index \"" + std::to_string(functionIndex) + "\"!");
	}
}

void EventsComponent::HookEventPost(const std::string& functionName, std::function<void(const PostEvent&)> postHook)
{
	UFunction* foundFunction = Instances.FindStaticFunction(functionName);

	if (foundFunction)
	{
		HookEventPost(foundFunction->ObjectInternalInteger, postHook);
	}
	else
	{
		Console.Warning(GetNameFormatted() + "Warning: Failed to hook function \"" + functionName + "\"!");
	}
}

void EventsComponent::HookEventPost(uint32_t functionIndex, std::function<void(const PostEvent&)> postHook)
{
	UObject* foundFunction = UObject::GObjObjects()->At(functionIndex);
	if (foundFunction && foundFunction->IsA(UFunction::StaticClass()))
	{
		if (PostHookedEvents.find(functionIndex) != PostHookedEvents.end())
		{
			PostHookedEvents[functionIndex].push_back(postHook);
		}
		else
		{
			PostHookedEvents[functionIndex] = std::vector<std::function<void(const PostEvent&)>>{ postHook };
		}
	}
	else
	{
		Console.Warning(GetNameFormatted() + "Warning: Failed to hook function at index \"" + std::to_string(functionIndex) + "\"!");
	}
}

void EventsComponent::Initialize()
{
	HookEventPre("Function Engine.Interaction.Tick", &Hooks::ExecuteFunctions);


	HookEventPre("Function TAGame.PRI_TA.ReplicatedEvent", &Hooks::PriReplicatedEvent);
	HookEventPre("Function Engine.HUD.PostRender", &Hooks::HUDPostRender);
	HookEventPre("Function Engine.PlayerController.PlayerTick", &Hooks::PlayerControllerTick);
	HookEventPre("Function TAGame.GFxData_PRI_TA.UpdatePRIData", &Hooks::UpdatePriData);
	HookEventPost("Function TAGame.GameViewportClient_TA.HandleKeyPress", &Hooks::GameViewPortKeyPress);

	HookEventPre("Function TAGame.GFxData_MainMenu_TA.MainMenuAdded", &Hooks::GFxDataMainMenuAdded);


	HookEventPre("Function TAGame.PlayerController_TA.ReplicateCameraRotation", [&](PreEvent event) {
		if (event.GetCaller<APlayerController>()->PlayerCamera) {
			Main.FOV = event.GetCaller<APlayerController>()->PlayerCamera->GetFOVAngle();
			Main.cameraLocation = event.GetCaller<APlayerController>()->PlayerCamera->Location;
			Main.cameraRotator = event.GetCaller<APlayerController>()->PlayerCamera->Rotation;
		}
		});

	HookEventPre("Function TAGame.GFxData_Community_TA.HandleBlogChanged", [&](PreEvent event){
		UGFxData_Community_TA* Community = event.GetCaller<UGFxData_Community_TA>();
		
		if (Community && Community->Config) {	
			Community->Config->MotD = Format::ToFString(_("@tvyt's function logger<br>Snayck on UnknownCheats"));
		}
	});

	Console.Write(GetNameFormatted() + std::to_string(PreHookedEvents.size()) + " Pre-Hook(s) Initialized!");
	Console.Write(GetNameFormatted() + std::to_string(PostHookedEvents.size()) + " Post-Hook(s) Initialized!");
}

class EventsComponent Events {};