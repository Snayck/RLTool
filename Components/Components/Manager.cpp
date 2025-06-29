#include "Manager.hpp"
#include "../Includes.hpp"

Setting::Setting(VariableIds variable, SettingTypes settingType, const std::string& description, const std::string& defaultValue, bool bModifiable) :
	Variable(variable),
	Type(settingType),
	Description(description),
	DefaultValue(defaultValue),
	CurrentValue(defaultValue),
	Modifiable(bModifiable),
	Callback(nullptr),
	ArgumentCallback(nullptr)
{

}

Setting::Setting(VariableIds variable, SettingTypes settingType, const std::string& description, const std::string& defaultValue, bool bModifiable, std::function<void()> callback) :
	Variable(variable),
	Type(settingType),
	Description(description),
	DefaultValue(defaultValue),
	CurrentValue(defaultValue),
	Modifiable(bModifiable),
	Callback(callback),
	ArgumentCallback(nullptr)
{

}

Setting::Setting(VariableIds variable, SettingTypes settingType, const std::string& description, const std::string& defaultValue, bool bModifiable, std::function<void(std::string)> callback) :
	Variable(variable),
	Type(settingType),
	Description(description),
	DefaultValue(defaultValue),
	CurrentValue(defaultValue),
	Modifiable(bModifiable),
	Callback(nullptr),
	ArgumentCallback(callback)
{

}

Setting::~Setting() {}

VariableIds Setting::GetId() const
{
	return Variable;
}

SettingTypes Setting::GetType() const
{
	return Type;
}

std::string Setting::GetName() const
{
	return ManagerComponent::GetVariableName(GetId());
}

const std::string& Setting::GetDescription() const
{
	return Description;
}

bool Setting::HasRange() const
{
	return (!Range.first.empty() && !Range.second.empty());
}

bool Setting::InRange(const std::string& sValue) const
{
	if (HasRange())
	{
		if (GetType() == SettingTypes::TYPE_BOOL)
		{
			return (sValue == _("0") || sValue == _("false") || sValue == _("1") || sValue == _("true"));
		}
		else if (GetType() == SettingTypes::TYPE_INT)
		{
			int32_t formattedMin = std::stoi(Range.first);
			int32_t formattedMax = std::stoi(Range.second);
			int32_t formattedValue = std::stoi(sValue);
			return (formattedValue >= formattedMin && formattedValue <= formattedMax);
		}
		else if (GetType() == SettingTypes::TYPE_FLOAT)
		{
			float formattedMin = std::stof(Range.first);
			float formattedMax = std::stof(Range.second);
			float formattedValue = std::stof(sValue);
			return ((formattedValue >= formattedMin) && (formattedValue <= formattedMax));
		}
		else if (GetType() == SettingTypes::TYPE_ROTATOR)
		{
			Rotator value = Format::ToRotator(sValue);
			Rotator rangeMin = Format::ToRotator(Range.first);
			Rotator rangeMax = Format::ToRotator(Range.second);

			if ((value.Pitch < rangeMin.Pitch) || (value.Pitch > rangeMax.Pitch)
				|| (value.Yaw < rangeMin.Yaw) || (value.Yaw > rangeMax.Yaw)
				|| (value.Roll < rangeMin.Roll) || (value.Yaw > rangeMax.Roll))
			{
				return false;
			}
		}
		else if (GetType() == SettingTypes::TYPE_VECTOR_3D)
		{
			VectorF value = Format::ToVectorF(sValue);
			VectorF rangeMin = Format::ToVectorF(Range.first);
			VectorF rangeMax = Format::ToVectorF(Range.second);

			if ((value.X < rangeMin.X) || (value.X > rangeMax.X)
				|| (value.Y < rangeMin.Y) || (value.Y > rangeMax.Y)
				|| (value.Z < rangeMin.Z) || (value.Z > rangeMax.Z))
			{
				return false;
			}
		}
		else if (GetType() == SettingTypes::TYPE_VECTOR_2D)
		{
			Vector2DF value = Format::ToVector2DF(sValue);
			Vector2DF rangeMin = Format::ToVector2DF(Range.first);
			Vector2DF rangeMax = Format::ToVector2DF(Range.second);

			if ((value.X < rangeMin.X) || (value.X > rangeMax.X)
				|| (value.Y < rangeMin.Y) || (value.Y > rangeMax.Y))
			{
				return false;
			}
		}
	}

	return true;
}

bool Setting::IsValueValid(const std::string& sValue) const
{
	if (GetType() == SettingTypes::TYPE_BOOL)
	{
		if (!(sValue == _("1") || sValue == _("true") || sValue == _("0") || sValue == _("false")))
		{
			Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports true or false values."));
			return false;
		}
	}
	if (GetType() == SettingTypes::TYPE_INT)
	{
		if (!Format::IsStringFloat(sValue))
		{
			Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports floating point numbers or 32 bit integer values."));
			return false;
		}
	}
	else if (GetType() == SettingTypes::TYPE_FLOAT)
	{
		if (!Format::IsStringFloat(sValue))
		{
			Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports floating point numbers or optionally 32 bit integer values."));
			return false;
		}
	}
	else if (GetType() == SettingTypes::TYPE_COLOR)
	{
		std::string cValue = Format::RemoveAllChars(sValue, '#');

		if (Format::IsStringHexadecimal(cValue) && (cValue.length() <= 8))
		{
			uint64_t dValue = Format::ToDecimal(cValue);

			if (dValue <= 0xFFFFFFFF)
			{
				return true;
			}
		}

		Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports RGBA hexadecimal values (alpha value optional)."));
		return false;
	}
	else if (GetType() == SettingTypes::TYPE_ROTATOR)
	{
		std::vector<std::string> values = Format::Split(sValue, ' ');

		if (values.size() >= 3)
		{
			if (Format::IsStringDecimal(values[0]) && Format::IsStringDecimal(values[1]) && Format::IsStringDecimal(values[2]))
			{
				return true;
			}
		}

		Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports three 32 bit integer values."));
		return false;
	}
	else if (GetType() == SettingTypes::TYPE_VECTOR_3D)
	{
		std::vector<std::string> values = Format::Split(sValue, ' ');

		if (values.size() >= 3)
		{
			if (Format::IsStringFloat(values[0]) && Format::IsStringFloat(values[1]) && Format::IsStringFloat(values[2]))
			{
				return true;
			}
		}

		Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports three floating point numbers or optionally 32 bit integer values."));
		return false;
	}
	else if (GetType() == SettingTypes::TYPE_VECTOR_2D)
	{
		std::vector<std::string> values = Format::Split(sValue, ' ');

		if (values.size() >= 2)
		{
			if (Format::IsStringFloat(values[0]) && Format::IsStringFloat(values[1]))
			{
				return true;
			}
		}

		Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is invalid, this setting only supports two floating point numbers or optionally 32 bit integer values."));
		return false;
	}

	return true;
}

bool Setting::IsModifiable() const
{
	return Modifiable;
}

bool Setting::HasCallback() const
{
	return (!!Callback);
}

bool Setting::HasArgumentCallback() const
{
	return (!!ArgumentCallback);
}

const std::string& Setting::GetDefaultValue() const
{
	return DefaultValue;
}

const std::string& Setting::GetStringValue() const
{
	return CurrentValue;
}

int32_t Setting::GetIntValue() const
{
	if (GetType() == SettingTypes::TYPE_INT)
	{
		return std::stoi(GetStringValue());
	}

	return 0;
}

bool Setting::GetBoolValue() const
{
	if (GetType() == SettingTypes::TYPE_BOOL)
	{
		if ((GetStringValue() == _("true")) || (GetStringValue() == _("1")))
		{
			return true;
		}
	}

	return false;
}

float Setting::GetFloatValue() const
{
	if (GetType() == SettingTypes::TYPE_FLOAT)
	{
		return std::stof(GetStringValue());
	}

	return 0.0f;
}

Color Setting::GetColorValue() const
{
	if (GetType() == SettingTypes::TYPE_COLOR)
	{
		return Color(GetStringValue());
	}

	return Color();
}

LinearColor Setting::GetLinearValue() const
{
	if (GetType() == SettingTypes::TYPE_COLOR)
	{
		return LinearColor(GetStringValue());
	}

	return LinearColor();
}

Rotator Setting::GetRotatorValue() const
{
	if (GetType() == SettingTypes::TYPE_ROTATOR)
	{
		return Format::ToRotator(GetStringValue());
	}

	return Rotator();
}

VectorF Setting::GetVector3DFValue() const
{
	if ((GetType() == SettingTypes::TYPE_VECTOR_3D) || (GetType() == SettingTypes::TYPE_VECTOR_2D))
	{
		return Format::ToVectorF(GetStringValue());
	}

	return VectorF();
}

VectorI Setting::GetVector3DIValue() const
{
	if ((GetType() == SettingTypes::TYPE_VECTOR_3D) || (GetType() == SettingTypes::TYPE_VECTOR_2D))
	{
		return Format::ToVectorI(GetStringValue());
	}

	return VectorI();
}

Vector2DF Setting::GetVector2DFValue() const
{
	if ((GetType() == SettingTypes::TYPE_VECTOR_3D) || (GetType() == SettingTypes::TYPE_VECTOR_2D))
	{
		return Format::ToVector2DF(GetStringValue());
	}

	return Vector2DF();
}

Vector2DI Setting::GetVector2DIValue() const
{
	if ((GetType() == SettingTypes::TYPE_VECTOR_3D) || (GetType() == SettingTypes::TYPE_VECTOR_2D))
	{
		return Format::ToVector2DI(GetStringValue());
	}

	return Vector2DI();
}

Setting* Setting::ResetToDefault(ThreadTypes thread)
{
	SetStringValue(GetDefaultValue(), thread);
	return this;
}

Setting* Setting::SetStringValue(const std::string& sValue, ThreadTypes thread)
{
	if (thread != ThreadTypes::THREAD_GAME)
	{
		Manager.InternalCommand(GetName(), sValue, thread);
	}
	else if (IsValueValid(sValue))
	{
		if (InRange(sValue))
		{
			if (GetType() == SettingTypes::TYPE_BOOL)
			{
				if (sValue == _("1") || sValue == _("true"))
				{
					CurrentValue = _("true");
				}
				else
				{
					CurrentValue = _("false");
				}
			}
			else
			{
				CurrentValue = sValue;
			}

			TriggerCallbacks();
		}
		else
		{
			Console.Warning(_("[Setting] (") + GetName() + _(") Warning: Input is out of range, this setting has a minimum value of \"") + Range.first + _("\" and a maximum value of \"") + Range.second + _("\"."));
		}
	}

	return this;
}

Setting* Setting::SetIntValue(int32_t iValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(iValue), thread);
	return this;
}

Setting* Setting::SetBoolValue(bool bValue, ThreadTypes thread)
{
	SetStringValue((bValue ? _("true") : _("false")), thread);
	return this;
}

Setting* Setting::SetFloatValue(float fValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(fValue), thread);
	return this;
}

Setting* Setting::SetColorValue(const Color& cValue, ThreadTypes thread)
{
	SetStringValue(cValue.ToHexAlpha(), thread);
	return this;
}

Setting* Setting::SetLinearValue(const LinearColor& lValue, ThreadTypes thread)
{
	SetStringValue(lValue.ToHexAlpha(), thread);
	return this;
}

Setting* Setting::SetRotatorValue(const Rotator& rValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(rValue.Pitch) + _(" ") + std::to_string(rValue.Yaw) + _(" ") + std::to_string(rValue.Roll), thread);
	return this;
}

Setting* Setting::SetVectorFValue(const VectorF& vfValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(vfValue.X) + _(" ") + std::to_string(vfValue.Y) + _(" ") + std::to_string(vfValue.Z), thread);
	return this;
}

Setting* Setting::SetVectorIValue(const VectorI& viValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(viValue.X) + _(" ") + std::to_string(viValue.Y) + _(" ") + std::to_string(viValue.Z), thread);
	return this;
}

Setting* Setting::SetVector2DFValue(const Vector2DF& vfValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(vfValue.X) + _(" ") + std::to_string(vfValue.Y), thread);
	return this;
}

Setting* Setting::SetVector2DIValue(const Vector2DI& viValue, ThreadTypes thread)
{
	SetStringValue(std::to_string(viValue.X) + _(" ") + std::to_string(viValue.Y), thread);
	return this;
}

Setting* Setting::RemoveRange()
{
	Range.first.clear();
	Range.second.clear();
	return this;
}

Setting* Setting::SetStringRange(const std::string& minValue, const std::string& maxValue)
{
	Range = std::make_pair(minValue, maxValue);
	return this;
}

Setting* Setting::SetIntRange(int32_t minValue, int32_t maxValue)
{
	SetStringRange(std::to_string(minValue), std::to_string(maxValue));
	return this;
}

Setting* Setting::SetFloatRange(float minValue, float maxValue)
{
	SetStringRange(std::to_string(minValue), std::to_string(maxValue));
	return this;
}

Setting* Setting::SetRotatorRange(const Rotator& minValue, const Rotator& maxValue)
{
	SetStringRange(std::to_string(minValue.Pitch) + _(" ") + std::to_string(minValue.Yaw) + _(" ") + std::to_string(minValue.Roll), std::to_string(maxValue.Pitch) + _(" ") + std::to_string(maxValue.Yaw) + _(" ") + std::to_string(maxValue.Roll));
	return this;
}

Setting* Setting::SetVectorFRange(const VectorF& minValue, const VectorF& maxValue)
{
	SetStringRange(std::to_string(minValue.X) + _(" ") + std::to_string(minValue.Y) + _(" ") + std::to_string(minValue.Z), std::to_string(maxValue.X) + _(" ") + std::to_string(maxValue.Y) + _(" ") + std::to_string(maxValue.Z));
	return this;
}

Setting* Setting::SetVectorIRange(const VectorI& minValue, const VectorI& maxValue)
{
	SetStringRange(std::to_string(minValue.X) + _(" ") + std::to_string(minValue.Y) + _(" ") + std::to_string(minValue.Z), std::to_string(maxValue.X) + _(" ") + std::to_string(maxValue.Y) + _(" ") + std::to_string(maxValue.Z));
	return this;
}

Setting* Setting::SetVector2DFRange(const Vector2DF& minValue, const Vector2DF& maxValue)
{
	SetStringRange(std::to_string(minValue.X) + _(" ") + std::to_string(minValue.Y), std::to_string(maxValue.X) + _(" ") + std::to_string(maxValue.Y));
	return this;
}

Setting* Setting::SetVector2DIRange(const Vector2DI& minValue, const Vector2DI& maxValue)
{
	SetStringRange(std::to_string(minValue.X) + _(" ") + std::to_string(minValue.Y), std::to_string(maxValue.X) + _(" ") + std::to_string(maxValue.Y));
	return this;
}

std::pair<int32_t, int32_t> Setting::GetIntRange() const
{
	std::pair<float, float> floatRange = GetFloatRange();
	return std::make_pair(static_cast<int32_t>(floatRange.first), static_cast<int32_t>(floatRange.second));
}

std::pair<float, float> Setting::GetFloatRange() const
{
	std::pair<float, float> returnRange = std::make_pair(0.0f, 0.0f);

	if (HasRange())
	{
		if (Format::IsStringFloat(Range.first) && Format::IsStringFloat(Range.second))
		{
			returnRange.first = std::stof(Range.first);
			returnRange.second = std::stof(Range.second);
		}
	}

	return returnRange;
}

std::pair<Rotator, Rotator> Setting::GetRotatorRange() const
{
	std::pair<Rotator, Rotator> returnRange = std::make_pair(Rotator(), Rotator());

	if (HasRange())
	{
		returnRange.first = Format::ToRotator(Range.first);
		returnRange.second = Format::ToRotator(Range.second);
	}

	return returnRange;
}

std::pair<VectorF, VectorF> Setting::GetVectorFRange() const
{
	std::pair<VectorF, VectorF> returnRange = std::make_pair(VectorF(), VectorF());

	if (HasRange())
	{
		returnRange.first = Format::ToVectorF(Range.first);
		returnRange.second = Format::ToVectorF(Range.second);
	}

	return returnRange;
}

std::pair<VectorI, VectorI> Setting::GetVectorIRange() const
{
	std::pair<VectorI, VectorI> returnRange = std::make_pair(VectorI(), VectorI());

	if (HasRange())
	{
		returnRange.first = Format::ToVectorI(Range.first);
		returnRange.second = Format::ToVectorI(Range.second);
	}

	return returnRange;
}

std::pair<Vector2DF, Vector2DF> Setting::GetVector2DFRange() const
{
	std::pair<Vector2DF, Vector2DF> returnRange = std::make_pair(Vector2DF(), Vector2DF());

	if (HasRange())
	{
		returnRange.first = Format::ToVector2DF(Range.first);
		returnRange.second = Format::ToVector2DF(Range.second);
	}

	return returnRange;
}

std::pair<Vector2DI, Vector2DI> Setting::GetVector2DIRange() const
{
	std::pair<Vector2DI, Vector2DI> returnRange = std::make_pair(Vector2DI(), Vector2DI());

	if (HasRange())
	{
		returnRange.first = Format::ToVector2DI(Range.first);
		returnRange.second = Format::ToVector2DI(Range.second);
	}

	return returnRange;
}

Setting* Setting::UnbindCallbacks()
{
	Callback = nullptr;
	ArgumentCallback = nullptr;
	return this;
}

Setting* Setting::BindCallback(std::function<void()> callback)
{
	Callback = callback;
	return this;
}

Setting* Setting::BindCallback(std::function<void(std::string)> callback)
{
	ArgumentCallback = callback;
	return this;
}

void Setting::TriggerCallbacks() const
{
	if (HasCallback())
	{
		Callback();
	}
	else if (HasArgumentCallback())
	{
		ArgumentCallback(GetStringValue());
	}
}

Setting& Setting::operator=(const Setting& setting)
{
	Variable = setting.Variable;
	Type = setting.Type;
	Description = setting.Description;
	DefaultValue = setting.DefaultValue;
	CurrentValue = setting.CurrentValue;
	Range = setting.Range;
	Modifiable = setting.Modifiable;
	Callback = setting.Callback;
	ArgumentCallback = setting.ArgumentCallback;
	return *this;
}

Command::Command(VariableIds variable, const std::string& description, bool bSearchable) :
	Variable(variable),
	Description(description),
	Searchable(bSearchable),
	Callback(nullptr),
	ArgumentCallback(nullptr)
{

}

Command::Command(const Command& command) :
	Variable(command.Variable),
	Description(command.Description),
	Searchable(command.Searchable),
	Callback(command.Callback),
	ArgumentCallback(command.ArgumentCallback)
{

}

Command::~Command() {}

VariableIds Command::GetId() const
{
	return Variable;
}

std::string Command::GetName() const
{
	return Manager.GetVariableName(GetId());
}

const std::string& Command::GetDescription() const
{
	return Description;
}

bool Command::IsSearchable() const
{
	return Searchable;
}

bool Command::HasCallback() const
{
	return (!!Callback);
}

bool Command::HasArgumentCallback() const
{
	return (!!ArgumentCallback);
}

Command* Command::BindCallback(std::function<void()> callback)
{
	Callback = callback;
	return this;
}

Command* Command::BindCallback(std::function<void(std::string)> argumentCallback)
{
	ArgumentCallback = argumentCallback;
	return this;
}

Command* Command::UnbindCallbacks()
{
	Callback = nullptr;
	ArgumentCallback = nullptr;
	return this;
}

void Command::TriggerCallback() const
{
	if (HasCallback())
	{
		Callback();
	}
	else
	{
		Console.Warning(_("[Command] (") + GetName() + _(") Warning: Unbound callback triggered!"));
	}
}

void Command::TriggerCallback(const std::string& arguments) const
{
	if (HasArgumentCallback())
	{
		ArgumentCallback(arguments);
	}
	else
	{
		Console.Warning(_("[Command] (") + GetName() + _(") Warning: Unbound argument callback triggered!"));
	}
}

Command& Command::operator=(const Command& command)
{
	Variable = command.Variable;
	Description = command.Description;
	Searchable = command.Searchable;
	Callback = command.Callback;
	ArgumentCallback = command.ArgumentCallback;
	return *this;
}

QueueData::QueueData() : Delay(0), Delta(0), Internal(false), Expired(false) {}

QueueData::QueueData(const std::string& command, const std::string& arguments, bool bInternal) :
	Command(command),
	Arguments(arguments),
	Delay(0),
	Delta(0),
	Internal(bInternal),
	Expired(false)
{

}

QueueData::QueueData(const std::string& command, const std::string& arguments, bool bInternal, uint64_t delay, uint32_t multiplier) :
	Command(command),
	Arguments(arguments),
	Delay(delay* multiplier),
	Delta(0),
	Internal(bInternal),
	Expired(false)
{

}

QueueData::QueueData(const QueueData& queueData) :
	Command(queueData.Command),
	Arguments(queueData.Arguments),
	Delay(queueData.Delay),
	Delta(queueData.Delta),
	Internal(queueData.Internal),
	Expired(queueData.Expired)
{

}

QueueData::~QueueData() {}

bool QueueData::HasArguments() const
{
	return (!Arguments.empty());
}

bool QueueData::IsAsync() const
{
	return (Delay > 0);
}

bool QueueData::OnTick()
{
	if (IsAsync())
	{
		Delta++;
		return (Delta >= Delay);
	}

	return true;
}

QueueData& QueueData::operator=(const QueueData& queueData)
{
	Command = queueData.Command;
	Arguments = queueData.Arguments;
	Delay = queueData.Delay;
	Delta = queueData.Delta;
	Internal = queueData.Internal;
	Expired = queueData.Expired;
	return *this;
}

ManagerComponent::ManagerComponent() : Component(_("Manager"), _("Manages settings, commands, and mods.")) { OnCreate(); }

ManagerComponent::~ManagerComponent() { OnDestroy(); }

void ManagerComponent::OnCreate() {}

void ManagerComponent::OnDestroy()
{
	ModuleMap.clear();
	CommandMap.clear();
	SettingMap.clear();
}

void ManagerComponent::UnrealCommand(const std::string& unrealCommand, bool bLogToConsole)
{
	static AActor* defaultActor = nullptr;

	if (!defaultActor)
	{
		defaultActor = Instances.GetDefaultInstanceOf<AActor>();
	}

	if (defaultActor)
	{
		if (unrealCommand.find(_("unreal_command")) == 0)
		{
			std::string formattedCommand = unrealCommand;
			formattedCommand.erase(0, 15);

			if (bLogToConsole)
			{
				Console.Write(GetNameFormatted() + _("Executing unreal command \"") + formattedCommand + _("\"."));
			}

			defaultActor->ConsoleCommand(Format::ToFString(formattedCommand), 1);
		}
		else
		{
			if (bLogToConsole)
			{
				Console.Write(GetNameFormatted() + _("Executing unreal command \"") + unrealCommand + _("\"."));
			}

			defaultActor->ConsoleCommand(Format::ToFString(unrealCommand), 1);
		}
	}
}

std::pair<CommandTypes, std::string> ManagerComponent::InternalCommand(const QueueData& queueData, ThreadTypes thread)
{
	if ((thread != ThreadTypes::THREAD_GAME) || (queueData.IsAsync()))
	{
		CommandQueue.push_back(queueData);
		return std::make_pair(CommandTypes::TYPE_QUEUED, "");
	}
	else
	{
		std::shared_ptr<Command> consoleCommand = GetCommand(queueData.Command);

		if (consoleCommand)
		{
			if (consoleCommand->HasCallback() && !queueData.HasArguments())
			{
				consoleCommand->TriggerCallback();
			}
			else if (consoleCommand->HasArgumentCallback())
			{
				if (queueData.HasArguments())
				{
					consoleCommand->TriggerCallback(queueData.Arguments);
				}
				else
				{
					return std::make_pair(CommandTypes::TYPE_EMPTY_ARGUMENTS, _(""));
				}
			}
			else
			{
				return std::make_pair(CommandTypes::TYPE_INVALID_ARGUMENTS, _(""));
			}
		}
		else
		{
			std::shared_ptr<Setting> consoleSetting = GetSetting(queueData.Command);

			if (consoleSetting && consoleSetting->IsModifiable())
			{
				if (queueData.HasArguments())
				{
					std::string oldValue = consoleSetting->GetStringValue();
					consoleSetting->SetStringValue(queueData.Arguments);
					return std::make_pair(CommandTypes::TYPE_MODIFY_SETTING, oldValue);
				}
				else
				{
					return std::make_pair(CommandTypes::TYPE_PRINT_SETTING, _(""));
				}
			}
			else
			{
				return std::make_pair(CommandTypes::TYPE_UNRECOGNIZED, _(""));
			}
		}
	}

	return std::make_pair(CommandTypes::TYPE_NONE, _(""));
}

std::pair<CommandTypes, std::string> ManagerComponent::InternalCommand(const std::string& command, const std::string& arguments, ThreadTypes thread)
{
	return InternalCommand(QueueData(command, arguments, true), thread);
}

std::pair<CommandTypes, std::string> ManagerComponent::InternalCommand(const std::string& command, ThreadTypes thread)
{
	return InternalCommand(command, _(""), thread);
}

void ManagerComponent::ConsoleCommand(const QueueData& queueData, ThreadTypes thread)
{
	if (thread != ThreadTypes::THREAD_GAME)
	{
		CommandQueue.push_back(queueData);
	}
	else
	{
		std::pair<CommandTypes, std::string> returnType = InternalCommand(queueData, thread);

		if (returnType.first != CommandTypes::TYPE_NONE)
		{
			if (returnType.first == CommandTypes::TYPE_UNRECOGNIZED)
			{
				Console.Error(GetNameFormatted() + _("Unrecognized command or setting: \"") + queueData.Command + _("\"."));
			}
			else if (returnType.first == CommandTypes::TYPE_EMPTY_ARGUMENTS)
			{
				Console.Error(GetNameFormatted() + _("Invalid arguments provided for: \"") + queueData.Command + _("\"."));
			}
			else if (returnType.first == CommandTypes::TYPE_INVALID_ARGUMENTS)
			{
				Console.Error(GetNameFormatted() + _("Arguments are not supported for: \"") + queueData.Command + _("\"."));
			}
			else if (returnType.first == CommandTypes::TYPE_MODIFY_SETTING)
			{
				std::shared_ptr<Setting> consoleSetting = GetSetting(queueData.Command);

				if (consoleSetting)
				{
					Console.Notify(GetNameFormatted() + consoleSetting->GetName() + _(": ") + returnType.second + _(" -> ") + consoleSetting->GetStringValue());
				}
			}
			else if (returnType.first == CommandTypes::TYPE_PRINT_SETTING)
			{
				std::shared_ptr<Setting> consoleSetting = GetSetting(queueData.Command);

				if (consoleSetting)
				{
					Console.Notify(GetNameFormatted() + consoleSetting->GetName());
					Console.Notify(GetNameFormatted() + _("Description: ") + consoleSetting->GetDescription());
					Console.Notify(GetNameFormatted() + _("Current Value: ") + consoleSetting->GetStringValue());
				}
			}
		}
	}
}

void ManagerComponent::ConsoleCommand(const std::string& command, const std::string& arguments, ThreadTypes thread)
{
	ConsoleCommand(QueueData(command, arguments, false), thread);
}

void ManagerComponent::ConsoleCommand(const std::string& command, ThreadTypes thread)
{
	ConsoleCommand(command, _(""), thread);
}

void ManagerComponent::AsyncCommand(const std::string& command, const std::string& arguments, uint64_t delay, ThreadTypes thread)
{
	ConsoleCommand(QueueData(command, arguments, false, delay), thread);
}

void ManagerComponent::AsyncCommand(const std::string& command, uint64_t delay, ThreadTypes thread)
{
	ConsoleCommand(QueueData(command, _(""), false, delay), thread);
}

void ManagerComponent::OnTick()
{
	if (!CommandQueue.empty())
	{
		bool safeToClear = true;

		for (QueueData& data : CommandQueue)
		{
			if (!data.Expired)
			{
				if (data.IsAsync())
				{
					if (data.OnTick())
					{
						data.Delay = 0;
					}
					else
					{
						safeToClear = false;
						continue;
					}
				}

				if (data.Internal)
				{
					safeToClear = false;
					data.Expired = true;
					InternalCommand(data.Command, data.Arguments, ThreadTypes::THREAD_GAME);
				}
				else
				{
					safeToClear = false;
					data.Expired = true;
					ConsoleCommand(data.Command, data.Arguments, ThreadTypes::THREAD_GAME);
				}
			}
		}

		if (safeToClear && !CommandQueue.empty())
		{
			CommandQueue.clear();
		}
	}

	GRainbowColor::OnTick();
}

void ManagerComponent::ResetSetting(const std::string& settingName, bool bLogToConsole)
{
	std::shared_ptr<Setting> setting = GetSetting(settingName);

	if (setting)
	{
		setting->ResetToDefault();

		if (bLogToConsole)
		{
			Console.Notify(_("[Manager Component] Reset setting to its default value!"));
			Console.Notify(_("[Manager Component] Name: ") + setting->GetName());
			Console.Notify(_("[Manager Component] Description: ") + setting->GetDescription());
			Console.Notify(_("[Manager Component] Current Value: ") + setting->GetStringValue());
		}
	}
	else
	{
		if (bLogToConsole)
		{
			Console.Error(_("[Manager Component] Unrecognized setting: \"") + settingName + _("\""));
		}
	}
}

void ManagerComponent::PrintModule(const std::string& moduleName)
{
	if (ModuleMap.find(moduleName) != ModuleMap.end())
	{
		std::shared_ptr<Module> mod = ModuleMap[moduleName];

		bool first = true;
		uint32_t states = mod->GetAllowedStates();
		std::string allowedStates;

		// Again, these states will all be game specific, please check ou the "GameState.hpp" file for more info.
		if (states & States::STATES_MainMenu) { allowedStates += (first ? _("(") : _(" | ")) + std::string(_("MainMenu")); first = false; }
		if (states & States::STATES_InGame) { allowedStates += (first ? _("(") : _(" | ")) + std::string(_("InGame")); first = false; }
		if (states & States::STATES_All) { allowedStates += (first ? _("(") : _(" | ")) + std::string(_("All")); first = false; }

		if (!first)
		{
			allowedStates += _(")");
		}

		Console.Notify(_("[Manager Component] Module Name: ") + mod->GetName());
		Console.Notify(_("[Manager Component] Module Description: ") + mod->GetDescription());
		Console.Notify(_("[Manager Component] Allowed States/Permissions: ") + allowedStates);
		Console.Notify(_("[Manager Component] Allowed In Current State: ") + std::string((mod->IsAllowed() ? _("true") : _("false"))));
	}
	else
	{
		Console.Error("[Manager Component] Failed to find module \"" + moduleName + "\"!");
	}
}

std::string ManagerComponent::GetVariableName(VariableIds variable)
{
	if (VariableMap.find(variable) != VariableMap.end())
	{
		return VariableMap[variable];
	}

	return "UnknownVariable";
}

void ManagerComponent::CreateVariable(std::string name, VariableIds variable)
{
	name = Format::ToLower(name);

	for (auto variableIt = VariableMap.begin(); variableIt != VariableMap.end(); variableIt++)
	{
		if (variableIt->second == name)
		{
			Console.Warning(_("[Manager Component] Warning: Duplicate variable name detected for \"") + name + _("\"!"));
			return;
		}
	}

	if (VariableMap.find(variable) == VariableMap.end())
	{
		VariableMap[variable] = name;
	}
	else
	{
		Console.Warning(_("[Manager Component] Warning: Duplicate variable id detected for \"") + std::to_string(static_cast<int32_t>(variable)) + _("\"!"));
	}
}

template <typename T> std::shared_ptr<T> ManagerComponent::CreateModule(Module* mod, std::shared_ptr<T>& moduleToBind)
{
	if (mod)
	{
		std::string moduleName = mod->GetName();
		moduleToBind = nullptr;

		if (ModuleMap.find(moduleName) == ModuleMap.end())
		{
			ModuleMap.emplace(std::make_pair(moduleName, std::shared_ptr<Module>(mod)));
			moduleToBind = std::static_pointer_cast<T>(ModuleMap[moduleName]);
			return moduleToBind;
		}
		else
		{
			Console.Warning(_("[Manager Component] Warning: Duplicate module name detected for \"") + moduleName + _("\"!"));
		}
	}
	else
	{
		Console.Error(_("[Manager Component] Error: Failed to create module, given pointer is invalid!"));
	}

	return nullptr;
}

template <typename T> std::shared_ptr<T> ManagerComponent::GetModule(const std::string& moduleName)
{
	if (ModuleMap.find(moduleName) != ModuleMap.end())
	{
		return std::static_pointer_cast<T>(ModuleMap[moduleName]);
	}

	return nullptr;
}

std::shared_ptr<Command> ManagerComponent::CreateCommand(Command* command)
{
	if (command)
	{
		std::string commandName = command->GetName();

		if (CommandMap.find(commandName) == CommandMap.end())
		{
			CommandMap.emplace(std::make_pair(commandName, std::shared_ptr<Command>(command)));
			return CommandMap[commandName];
		}
		else
		{
			Console.Warning(_("[Manager Component] Warning: Duplicate command name detected for \"") + commandName + _("\"!"));
		}
	}
	else
	{
		Console.Error(_("[Manager Component] Error: Failed to create command, given pointer is invalid!"));
	}

	return nullptr;
}

std::shared_ptr<Command> ManagerComponent::GetCommand(const std::string& commandName)
{
	if (CommandMap.find(commandName) != CommandMap.end())
	{
		return CommandMap[commandName];
	}

	return nullptr;
}

std::shared_ptr<Command> ManagerComponent::GetCommand(VariableIds variable)
{
	return GetCommand(GetVariableName(variable));
}

std::shared_ptr<Setting> ManagerComponent::CreateSetting(Setting* setting)
{
	if (setting)
	{
		std::string settingName = setting->GetName();

		if (SettingMap.find(settingName) == SettingMap.end())
		{
			SettingMap.emplace(std::make_pair(settingName, std::shared_ptr<Setting>(setting)));
			return SettingMap[settingName];
		}
		else
		{
			Console.Warning(_("[Manager Component] Warning: Duplicate setting name detected for \"") + settingName + _("\"!"));
		}
	}
	else
	{
		Console.Error(_("[Manager Component] Error: Failed to create setting, given pointer is invalid!"));
	}

	return nullptr;
}

std::shared_ptr<Setting> ManagerComponent::GetSetting(const std::string& settingName)
{
	if (SettingMap.find(settingName) != SettingMap.end())
	{
		return SettingMap[settingName];
	}

	return nullptr;
}

std::shared_ptr<Setting> ManagerComponent::GetSetting(VariableIds variable)
{
	return GetSetting(GetVariableName(variable));
}

void ManagerComponent::Initialize()
{
	Console.Write(GetNameFormatted() + std::to_string(CommandMap.size()) + _(" Command(s) Initialized!"));
	Console.Write(GetNameFormatted() + std::to_string(SettingMap.size()) + _(" Setting(s) Initialized!"));
}

class ManagerComponent Manager {};