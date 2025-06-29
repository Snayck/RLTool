#include "../EngineSim/RocketSim/src/RocketSim.h"
#include "../Components/Includes.hpp"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_stdlib.h"
#include "RocketSim/src/RLConst.h"

class EngineSim
{
private:

public:
	void Initialize();

public:
	void Predict();
	void Render(ImDrawList* IDL);
	///static bool isInGoal(Ball ball);
public:
	struct ballState {
		RocketSim::BallState ball;
		bool inGoal = false;
	};

public:
	static int tickAmount;
	static int stepSize;

	bool enablePrediction = false;

	RocketSim::Arena* arena = nullptr;

	bool ballGoingIn = false;
	ballState predictedPoints[5000];
};

extern class EngineSim engineSim;