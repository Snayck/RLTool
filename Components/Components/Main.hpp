#pragma once
#include "../Component.hpp"

class MainComponent : public Component
{
public:
	MainComponent();
	~MainComponent() override;

	void OnCreate() override;
	void OnDestroy() override;

	void Initialize();

public:
	// IMPORTANT

	static std::vector<std::function<void()>> GameFunctions;

	static void Execute(std::function<void()> FunctionToExecute)
	{
		GameFunctions.push_back(FunctionToExecute);
	}

public:
	// VARIABLES

	bool FieldOfViewEnabled = false;
	float FieldOfViewValue = 110;

	float FOV = 110;
	FRotator cameraRotator = {};
	FVector cameraLocation = {};

	std::string PlayerName;
	std::string OriginalPlayerName;
	bool EscapedNameCharacters = false;

	FUniqueNetId LocalPlayerId{};

	bool ToggleFPS = false;

	TArray<int32_t> LoadoutBlue = {};
	TArray<int32_t> LoadoutOrange = {};

	std::vector<FPlayerTitleData> TitlesData;
	FName PlayerCurrentTitle;

public:
	// CLASS POINTERS

	USaveData_TA* SaveData = nullptr;
	UGFxData_PlayerTitles_TA* PlayerTitles = nullptr;
	UProductDatabase_TA* ProductDatabase = nullptr;
	UOnlineProductStore_TA* OnlineProductStore = nullptr;
	URocketPassConfig_TA* PassConfig = nullptr;
	URocketPass_TA* RocketPass = nullptr;
	UOnlineSubsystem* oSS = nullptr;

public:
	// FUNCTIONS

	static void SpawnNotification(const std::string& Title, const std::string& Content, int Duration, UClass* NotificationClass = nullptr);

	static void SetPlayerName(std::string Name);

	static void SetCustomPresence(std::string Text);
};

extern class MainComponent Main;