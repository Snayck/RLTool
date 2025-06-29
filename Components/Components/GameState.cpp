#include "GameState.hpp"
#include "../Includes.hpp"

GameStateComponent::GameStateComponent() : Component(_("GameState"), _("Tracks which game state the player is currently in.")) { OnCreate(); }

GameStateComponent::~GameStateComponent() { OnDestroy(); }

void GameStateComponent::OnCreate()
{
	StateLabelMap.emplace(States::STATES_Unknown, _("Unknown"));
	StateLabelMap.emplace(States::STATES_MainMenu, _("MainMenu"));
	StateLabelMap.emplace(States::STATES_InGame, _("InGame"));
	StateLabelMap.emplace(States::STATES_All, _("All"));
}

void GameStateComponent::OnDestroy() {}

States GameStateComponent::GetState() const
{
	return CurrentState;
}

uint32_t GameStateComponent::GetRawState() const
{
	return static_cast<uint32_t>(CurrentState);
}

std::string GameStateComponent::GetStateLabel() const
{
	if (StateLabelMap.find(CurrentState) != StateLabelMap.end())
	{
		return StateLabelMap.at(CurrentState);
	}

	return "Unknown";
}

void GameStateComponent::SetState(States state)
{
	CurrentState = state;
}

bool GameStateComponent::IsInGame() const
{
	if ((GetRawState() & States::STATES_InGame))
	{
		return true;
	}

	return false;
}

void GameStateComponent::MainMenuAdded()
{
	CurrentState = States::STATES_MainMenu;
}

class GameStateComponent GameState {};