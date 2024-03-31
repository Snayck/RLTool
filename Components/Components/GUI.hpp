#pragma once
#include "../Component.hpp"

class GUIComponent : public Component
{
public:
	GUIComponent();
	~GUIComponent() override;

	static HANDLE InterfaceThread;

public:
	void OnCreate() override;
	void OnDestroy() override;
	static void Initialize();
	static void Unload();

public:
	static bool IsOpen;
	static bool hookEvent;
	static bool hookInternal;
	static bool hookCallFunc;
	static bool isLogging;
	static std::string whitelist;
	static std::string blacklist;

	int DisplayX;
	int DisplayY;

public:
	static void InitImGui();

	static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void Render();

};

extern class GUIComponent GUI;