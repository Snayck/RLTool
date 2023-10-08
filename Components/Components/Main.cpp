#include "Main.hpp"
#include "../Includes.hpp"
#include "../Modules/Includes.hpp"


MainComponent::MainComponent() : Component("Main", "Interface to game interacton") { OnCreate(); }

MainComponent::~MainComponent() { OnDestroy(); }

void MainComponent::OnCreate() {}

void MainComponent::OnDestroy() {}

void MainComponent::Initialize() {

	Main.SaveData = Instances.GetInstanceOf<USaveData_TA>();
	Main.PlayerTitles = Instances.GetInstanceOf<UGFxData_PlayerTitles_TA>();
	Main.ProductDatabase = Instances.GetInstanceOf<UProductDatabase_TA>();
	Main.OnlineProductStore = Instances.GetInstanceOf<UOnlineProductStore_TA>();
	Main.oSS = Instances.GetInstanceOf<UOnlineSubsystem>();

	UOnlinePlayerAuthentication_X* PlayerAuthentication = Instances.GetInstanceOf<UOnlinePlayerAuthentication_TA>();

	if (PlayerAuthentication)
	{
		Main.OriginalPlayerName = PlayerAuthentication->LoggedInPlayerName.ToString();
		Main.LocalPlayerId = PlayerAuthentication->LoggedInPlayerId;
	}
}

void MainComponent::SpawnNotification(const std::string& Title, const std::string& Content, int Duration, UClass* NotificationClass)
{
	if (UNotificationManager_TA* NotificationManager = Instances.GetInstanceOf<UNotificationManager_TA>()) {
		if (!NotificationClass)
		{
			NotificationClass = UGenericNotification_TA::StaticClass();
		}

		UNotification_TA* Notification = NotificationManager->PopUpOnlyNotification(NotificationClass);

		if (Notification) {
			Notification->SetTitle(Format::ToFString(Title));
			Notification->SetBody(Format::ToFString(Content));
			Notification->PopUpDuration = Duration;
		}
	}
}

void MainComponent::SetPlayerName(std::string Name)
{
	if (Main.EscapedNameCharacters)
	{
		Format::ParseEncodedStringInline(Name);
	}

	UOnlinePlayer_X* Player = Instances.GetInstanceOf<UOnlinePlayer_X>();
	nullcheck(Player);

	Player->SetPlayerName(Format::ToFString(Name));
}

void MainComponent::SetCustomPresence(std::string Presence)
{
	FString PresenceString = Format::ToFString(Presence);

	if (Main.EscapedNameCharacters)
	{
		Format::ParseEncodedStringInline(Presence);
	}

	UOnlinePlayerFriends_X* PlayerFriends = Instances.GetInstanceOf<UOnlinePlayerFriends_X>();
	nullcheck(PlayerFriends);

	PlayerFriends->SetPlatformRichPresence(0, PresenceString, PresenceString);
	PlayerFriends->SetEpicRichPresence(0, PresenceString, PresenceString);
}

std::vector<std::function<void()>> MainComponent::GameFunctions;

class MainComponent Main {};