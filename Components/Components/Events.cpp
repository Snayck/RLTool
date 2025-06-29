#include "Events.hpp"
#include "../Includes.hpp"
#include "../../EngineSim/RocketSim/src/RocketSim.h"
#include "../../EngineSim/engineSim.hpp"

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

RocketSim::RotMat FRotatorToRotMat(const FRotator& rotator) {
	// Convert integer angles to radians
	float pitch = rotator.Pitch * (3.1415926535897932f / 32768);
	float yaw = rotator.Yaw * (3.1415926535897932f / 32768);
	float roll = rotator.Roll * (3.1415926535897932f / 32768);

	// Precompute sin and cos values
	float cp = cos(pitch), sp = sin(pitch);
	float cy = cos(yaw), sy = sin(yaw);
	float cr = cos(roll), sr = sin(roll);

	// Construct the rotation matrix
	RocketSim::Vec forward(cy * cp, sy * cp, sp);
	RocketSim::Vec right(cy * sp * sr - sy * cr, sy * sp * sr + cy * cr, -cp * sr);
	RocketSim::Vec up(-cy * sp * cr - sy * sr, -sy * sp * cr + cy * sr, cp * cr);

	return RocketSim::RotMat(forward, right, up);
}

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
	void JoinGame(const PostEvent& event)
	{
		if (!event.Caller() || !event.Params())
			return;

		Console.Write("Joining game...");

		if (!engineSim.arena) {
			engineSim.arena = RocketSim::Arena::Create(RocketSim::GameMode::SOCCAR);
			Console.Write("Created virtual arena!");
		}

		GameState.SetState(STATES_InGame);
	}
	void SoccarTA_Destroyed(PreEvent& event)
	{
		if (!event.Caller())
			return;

		Console.Write("Leaving game...");
		GameState.SetState(STATES_Unknown);

		if (engineSim.arena) {
			delete engineSim.arena;
			engineSim.arena = nullptr;

			Console.Write("Removed virtual arena!");
		}
	}

	void AddCar(const PostEvent& event)
	{
		if (!GameState.IsInGame() || !engineSim.enablePrediction)
			return;

		if (!event.Caller() || !event.Params())
			return;

		AGameEvent_TA_execAddCar_Params* params = event.GetParams<AGameEvent_TA_execAddCar_Params>();
		if (!params || !params->Car)
			return;
		ACar_TA* car = params->Car;
		if (!engineSim.arena)
			return;
		engineSim.arena->_aCarMap[car] = engineSim.arena->AddCar(RocketSim::Team::BLUE);
	}

	void RemoveCar(PreEvent& event)
	{
		if (!event.Caller() || !event.Params() || !engineSim.enablePrediction)
			return;

		AGameEvent_TA_execRemoveCar_Params* params = event.GetParams<AGameEvent_TA_execRemoveCar_Params>();
		if (!params || !params->Car)
			return;

		ACar_TA* car = params->Car;
		
		if (!engineSim.arena)
			return;

		engineSim.arena->RemoveCar(engineSim.arena->_aCarMap[car]);
		engineSim.arena->_aCarMap.erase(car);
	}

	void EngineTick(const PostEvent& event) {
		if (!event.Caller() || !event.Params() || !GameState.IsInGame() || !engineSim.enablePrediction)
			return;

		if (Instances.IAWorldInfo() == nullptr || Instances.IAWorldInfo()->PawnList == nullptr)
			return;
		

		if (!engineSim.arena || !engineSim.arena->ball)
			return;
			
		for (APawn* Pawn = Instances.IAWorldInfo()->PawnList; Pawn; Pawn = Pawn->NextPawn)
		{
			if (Instances.IAPlayerController() == nullptr || Pawn == nullptr || Pawn->bDeleteMe || Pawn->bHidden)
				continue;
				
			bool IsPlayer = (Pawn->PlayerReplicationInfo != nullptr);

			if (IsPlayer) {
				ACar_TA* car = (ACar_TA*)Pawn;

				RocketSim::CarState carState = {};

				carState.pos	= RocketSim::Vec(car->Location.X,			car->Location.Y,		car->Location.Z);
				carState.vel	= RocketSim::Vec(car->Velocity.X,			car->Velocity.Y,		car->Velocity.Z);
				carState.angVel = RocketSim::Vec(car->AngularVelocity.X,	car->AngularVelocity.Y, car->AngularVelocity.Z);
				carState.rotMat = FRotatorToRotMat(car->Rotation);

				RocketSim::Car* simCar = engineSim.arena->GetCar(engineSim.arena->_aCarMap[car]->id);
				if (!simCar)
					return;

				simCar->SetState(carState);
			}
			else {
				ABall_TA* ball = (ABall_TA*)Pawn;

				RocketSim::BallState ballState = {};

				ballState.pos		= RocketSim::Vec(ball->Location.X,			ball->Location.Y,			ball->Location.Z);
				ballState.vel		= RocketSim::Vec(ball->Velocity.X,			ball->Velocity.Y,			ball->Velocity.Z);
				ballState.angVel	= RocketSim::Vec(ball->AngularVelocity.X,	ball->AngularVelocity.Y,	ball->AngularVelocity.Z);
				ballState.rotMat	= FRotatorToRotMat(ball->Rotation);
				
				if (!engineSim.arena->ball)
					return;

				engineSim.arena->ball->SetState(ballState);
			}
		}
		engineSim.Predict();
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



std::vector<EventsComponent::func> EventsComponent::functions = {};
std::vector<std::string> blacklist = {};
std::vector<std::string> whitelist = {};


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
		p.type = FunctionLogger::processType(a);
		funct.properties.push_back(p);
	}

	EventsComponent::functions.push_back(funct);
}

void handleFunc(UObject* caller, UFunction* Function, void* paramsPointer) {
	if (EventsComponent::functions.size() < 1000) {

		if (GUIComponent::whitelist != "") {
			std::vector<std::string> tokens = FunctionLogger::splitString(GUIComponent::whitelist, ',');

			auto it = std::find_if(tokens.begin(), tokens.end(), [&](const std::string& str) {
				return FunctionLogger::containsSubstring(FunctionLogger::toLowercase(Function->Name.ToString()), FunctionLogger::toLowercase(str));
				});

			if (it != tokens.end()) {

			}
			else {
				return;
			}
		}

		if (GUIComponent::blacklist != "") {
			std::vector<std::string> tokens = FunctionLogger::splitString(GUIComponent::blacklist, ',');

			auto it = std::find_if(tokens.begin(), tokens.end(), [&](const std::string& str) {
				return FunctionLogger::containsSubstring(FunctionLogger::toLowercase(Function->Name.ToString()), FunctionLogger::toLowercase(str));
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


bool filterInput(std::string whitelist, std::string blacklist, std::string input)
{
	auto checkTokens = [&](const std::string& list) {
		if (list.empty()) return false;
		auto tokens = FunctionLogger::splitString(list, ',');
		auto lowerInput = FunctionLogger::toLowercase(input);
		return std::any_of(tokens.begin(), tokens.end(), [&](const std::string& token) {
			return FunctionLogger::containsSubstring(lowerInput, FunctionLogger::toLowercase(token));
			});
		};

	if (!whitelist.empty() && !checkTokens(whitelist)) return true;
	return checkTokens(blacklist);
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
	HookEventPre("Function TAGame.GameEvent_Soccar_TA.PostGoalScored.StartReplay", &Hooks::SoccarTA_Destroyed);
	HookEventPre(_("Function TAGame.GameEvent_Soccar_TA.Destroyed"), &Hooks::SoccarTA_Destroyed);
	HookEventPre(_("Function TAGame.GFxHUD_Soccar_TA.Destroyed"), &Hooks::SoccarTA_Destroyed);
	HookEventPost("Function TAGame.GameEvent_Soccar_TA.ReplayPlayback.EndState", &Hooks::JoinGame);
	HookEventPost(_("Function TAGame.GFxHUD_Soccar_TA.HandlePlayerAdded"), &Hooks::JoinGame);

	HookEventPre(_("Function TAGame.GameEvent_TA.RemoveCar"), &Hooks::RemoveCar);
	HookEventPost(_("Function TAGame.GameEvent_TA.AddCar"), &Hooks::AddCar);

	HookEventPost(_("Function TAGame.Vehicle_TA.SetVehicleInput"), &Hooks::EngineTick);

	HookEventPost("Function ProjectX.RPCQueue_X.CreateBatchSingleRPC", [&](const PostEvent& event) {

		URPCQueue_X* RPCQ = event.GetCaller<URPCQueue_X>();
		URPCQueue_X_eventCreateBatchSingleRPC_Params* msg = event.GetParams<URPCQueue_X_eventCreateBatchSingleRPC_Params>();

		if (!msg || !RPCQ || !msg->Message || !msg->Message->Headers) return;
		TMap<FString, FString> requestMap = msg->Message->Headers->Map;

		
		psyMsg newMsg;
		std::string allHeaders = "";

		for (int i = 0; i < requestMap.Elements.Num(); i++) {
			allHeaders += requestMap.at_index(i).Key.ToString() + ":" + requestMap.at_index(i).Value.ToString() + ",";

			newMsg.headersMap[requestMap.at_index(i).Key.ToString()] = requestMap.at_index(i).Value.ToString();
		}

		if (filterInput(GUIComponent::outgoingWhitelist, GUIComponent::outgoingBlacklist, allHeaders))
			return;

		newMsg.bodyText = msg->Message->GetBodyText().ToString();
		newMsg.ID = newMsg.headersMap["PsyRequestID"];
		Events.rpcOut.push_back(newMsg);
		});

	HookEventPost("Function ProjectX.PsyNetMessenger_X.EventMessageReceived", [&](const PostEvent& event) {

		UPsyNetMessenger_X* RPCQ = event.GetCaller<UPsyNetMessenger_X>();
		UPsyNetMessenger_X_execEventMessageReceived_Params* msg = event.GetParams<UPsyNetMessenger_X_execEventMessageReceived_Params>();

		if (!msg || !RPCQ || !msg->Message || !msg->Message->Headers) return;
		TMap<FString, FString> requestMap = msg->Message->Headers->Map;
		
		psyMsg newMsg;
		std::string allHeaders = "";

		for (int i = 0; i < requestMap.Elements.Num(); i++) {
			allHeaders += requestMap.at_index(i).Key.ToString() + ":" + requestMap.at_index(i).Value.ToString() + ",";

			newMsg.headersMap[requestMap.at_index(i).Key.ToString()] = requestMap.at_index(i).Value.ToString();
		}

		if (filterInput(GUIComponent::incomingWhitelist, GUIComponent::incomingBlacklist, allHeaders))
			return;
		
		newMsg.bodyText = msg->Message->GetBodyText().ToString();
		newMsg.ID = newMsg.headersMap["PsyResponseID"];
		Events.rpcIn.push_back(newMsg);

		});


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