#include "Drawing.hpp"
#include "../Components/Includes.hpp"

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#define UCONST_Pi 3.1415926
#define URotation180  32768 
#define URotationToRadians  UCONST_Pi / URotation180 

Drawing::Drawing(const std::string& name, const std::string& description, uint32_t states) : Module(name, description, states) {}
Drawing::~Drawing() {}


void inline GetAxes(FRotator R, FVector& X, FVector& Y, FVector& Z)
{
	X = R.ToVector();
	X.Normalize();
	R.Yaw += 16384;
	FRotator R2 = R;
	R2.Pitch = 0.f;
	Y = R2.ToVector();
	Y.Normalize();
	Y.Z = 0.f;
	R.Yaw -= 16384;
	R.Pitch += 16384;
	Z = R.ToVector();
	Z.Normalize();
}

FVector Drawing::CalculateScreenCoordinate(FVector Location)
{
	APlayerController* aPC = Instances.IAPlayerController();

	if (!aPC)
	{
		return{ 0,0,0 };
	}

	if (!aPC->PlayerCamera)
	{
		return{ 0,0,0 };
	}

	FVector Return;
	
	FVector AxisX, AxisY, AxisZ, Delta, Transformed;
	FRotator MYCam = Main.cameraRotator;

	GetAxes(MYCam, AxisX, AxisY, AxisZ);

	Delta = Location - Main.cameraLocation;

	Transformed.X = Delta.DotProduct(AxisY);
	Transformed.Y = Delta.DotProduct(AxisZ);
	Transformed.Z = Delta.DotProduct(AxisX);

	if (Transformed.Z < 1.00f)
		Transformed.Z = 1.00f;

	float FOVAngle = Main.FOV;
	ImGuiIO io = ImGui::GetIO();
	
	Return.X = (io.DisplaySize.x / 2.0f) + Transformed.X * ((io.DisplaySize.x / 2.0f) / tan(FOVAngle * PI / 360.0f)) / Transformed.Z;
	Return.Y = (io.DisplaySize.y / 2.0f) + -Transformed.Y * ((io.DisplaySize.x / 2.0f) / tan(FOVAngle * PI / 360.0f)) / Transformed.Z;
	Return.Z = 0;
	return Return;
}


