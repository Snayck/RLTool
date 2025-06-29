#include "engineSim.hpp"

struct vec2d {
	float x = 0;
	float y = 0;
};

vec2d MinMaxDistance(int i, int size) {
	float min = (((float)(i) / (float)size));
	float max = ((((float)size - (float)i) / (float)size));
	return vec2d{ min, max };
}


void EngineSim::Initialize() {
	RocketSim::Init("C:/Users/Chris/Documents/GitHub/RLTool/collision_meshes");
}

FVector VecToFVec(RocketSim::Vec in) {
	FVector out;
	out.X = in.x;
	out.Y = in.y;
	out.Z = in.z;

	return out;
}

int EngineSim::tickAmount = 1000;
void EngineSim::Predict() {
	if (!GameState.IsInGame()) {
		Console.Warning("not in game?");
		return;
	}
	else if (!engineSim.arena) {
		Console.Warning("no arena?");
		return;
	}
	else if (!engineSim.arena->ball) {
		Console.Warning("no ball?");
		return;
	}
	else if (!engineSim.enablePrediction) {
		return;
	}

	bool ballScored = false;
	predictedPoints[0].ball = arena->ball->GetState();
	for (int i = 1; i < tickAmount; i++) {
		engineSim.arena->Step(1);
		predictedPoints[i].ball = arena->ball->GetState();
		predictedPoints[i].inGoal = arena->IsBallScored();
		if (!ballScored)
			ballScored = predictedPoints[i].inGoal;
	}
	ballGoingIn = arena->IsBallProbablyGoingIn() || ballScored;
}

void EngineSim::Render(ImDrawList* IDL) {
	if (!GameState.IsInGame() || !engineSim.arena || !engineSim.arena->ball || !engineSim.enablePrediction)
		return;

	FVector location = VecToFVec(engineSim.predictedPoints[0].ball.pos);
	location = Drawing::CalculateScreenCoordinate(location);

	for (int i = (tickAmount-1); i > 0; i--) {
		FVector oldLoc = VecToFVec(engineSim.predictedPoints[i].ball.pos);
		FVector newLoc = VecToFVec(engineSim.predictedPoints[i - 1].ball.pos);

		oldLoc = Drawing::CalculateScreenCoordinate(oldLoc);
		newLoc = Drawing::CalculateScreenCoordinate(newLoc);

		vec2d distanceColor = MinMaxDistance(i, engineSim.tickAmount);

		ImVec4 stepColor;
		stepColor.x = distanceColor.x;
		stepColor.y = distanceColor.y;
		stepColor.z = 0;
		stepColor.w = 0.9;
		
		if (engineSim.predictedPoints[i].inGoal) {
			stepColor.x = 255;
			stepColor.y = 255;
			stepColor.z = 255;
			stepColor.w = 120;
		}

		if (engineSim.predictedPoints[i].inGoal != engineSim.predictedPoints[i - 1].inGoal)
			IDL->AddCircleFilled({ location.X, location.Y }, 5, ImGui::ColorConvertFloat4ToU32({ 255, 255, 255, 255 }));

		if (ballGoingIn)
			IDL->AddLine({ oldLoc.X, oldLoc.Y }, { newLoc.X, newLoc.Y }, ImGui::ColorConvertFloat4ToU32({ 255, 0, 0, 255 }), 5);

		IDL->AddLine({ oldLoc.X, oldLoc.Y }, { newLoc.X, newLoc.Y }, ImGui::ColorConvertFloat4ToU32(stepColor), 3);


	}
	IDL->AddCircleFilled({ location.X, location.Y }, 5, ImGui::ColorConvertFloat4ToU32({ 0, 255, 0, 255 }));
}

class EngineSim engineSim {};