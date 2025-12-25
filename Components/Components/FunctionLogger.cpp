#include "FunctionLogger.hpp"
#include "../Includes.hpp"

#include <sstream>

#include "../ImGui/Kiero/kiero.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_stdlib.h"

FunctionLogger::FunctionLogger()
	: Component("Function Logger", "Processes functions and object fields")
	, selectedIndex(-1)
{
	OnCreate();
}

FunctionLogger::~FunctionLogger()
{
	OnDestroy();
}

void FunctionLogger::OnCreate() {}

void FunctionLogger::OnDestroy() {}

std::string FunctionLogger::ToLowercase(const std::string& str)
{
	std::string result;
	result.reserve(str.length());
	for (char c : str)
		result.push_back(std::tolower(c));
	return result;
}

bool FunctionLogger::ContainsSubstring(const std::string& str, const std::string& substr)
{
	return str.find(substr) != std::string::npos;
}

std::string FunctionLogger::RemoveProperty(const std::string& input)
{
	std::string output = input;
	size_t pos = 0;
	while ((pos = output.find("Property", pos)) != std::string::npos)
		output.erase(pos, 8);
	return output;
}

std::vector<std::string> FunctionLogger::SplitString(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, delimiter))
		tokens.push_back(token);
	return tokens;
}

bool FunctionLogger::ClassOrParentMatches(UClass* Class, const std::string& query)
{
	if (!Class || query.empty())
		return false;

	if (ContainsSubstring(ToLowercase(Class->GetName()), ToLowercase(query)))
		return true;

	return ClassOrParentMatches((UClass*)Class->SuperField, query);
}

object FunctionLogger::CreateObject(UClass* Class, UObject* Object)
{
	object obj;
	if (!Class || !Object)
		return obj;
	if (IsBadReadPtr(Class, sizeof(UClass)) || IsBadReadPtr(Object, sizeof(UObject)))
		return obj;

	obj.Obj = Object;
	obj.Class = Class;
	obj.className = Class->GetName();
	obj.fullName = Class->GetFullName();

	if (!IsBadReadPtr(&Class->Children, sizeof(void*)) && Class->Children) {
		int fieldCount = 0;
		for (UField* field = Class->Children; field && fieldCount < 1000; field = field->Next) {
			if (!field || IsBadReadPtr(field, sizeof(UField)))
				break;
			fieldCount++;
			if (field->Next && IsBadReadPtr(field->Next, sizeof(UField)))
				break;

			UProperty* prop = reinterpret_cast<UProperty*>(field);
			if (!prop || !prop->Class || IsBadReadPtr(prop->Class, sizeof(UClass)))
				continue;

			if (prop->Class->GetName() == "Function")
				obj.functions.push_back(prop);
			else
				obj.fields.push_back(prop);
		}
	}
	return obj;
}

std::vector<object> FunctionLogger::GetAllInstancesOf(const std::string& query)
{
	std::vector<object> results;

	for (UObject* obj : *UObject::GObjObjects()) {
		if (!obj)
			continue;
		if (obj->GetFullName().find("Default__") != std::string::npos)
			continue;
		if (ClassOrParentMatches(obj->Class, query))
			results.push_back(CreateObject(obj->Class, obj));
	}

	return results;
}

std::string FunctionLogger::ProcessType(UProperty* prop)
{
	if (!prop)
		return "NULL";

	if (prop->IsA<UStructProperty>()) {
		UStructProperty* structProp = (UStructProperty*)prop;
		if (structProp->Struct && !IsBadReadPtr(structProp->Struct, sizeof(UStruct)))
			return "Struct " + structProp->Struct->GetName();
		return "Struct <invalid>";
	}

	if (prop->IsA<UObjectProperty>()) {
		UObjectProperty* objProp = (UObjectProperty*)prop;
		if (objProp->PropertyClass && !IsBadReadPtr(objProp->PropertyClass, sizeof(UClass)))
			return objProp->PropertyClass->GetName() + "*";
		return "Object* <invalid>";
	}

	if (prop->IsA<UArrayProperty>()) {
		UArrayProperty* arrayProp = (UArrayProperty*)prop;
		if (arrayProp->Inner && !IsBadReadPtr(arrayProp->Inner, sizeof(UProperty)))
			return "Array<" + ProcessType(arrayProp->Inner) + ">";
		return "Array<invalid>";
	}

	if (prop->IsA<UEnum>()) {
		UEnum* enumProp = (UEnum*)prop;
		if (!enumProp || IsBadReadPtr(enumProp, sizeof(UEnum)))
			return "Enum <invalid>";
		if (!IsBadReadPtr(&enumProp->Name, sizeof(FName)) && enumProp->Name.IsValid())
			return "Enum " + enumProp->Name.ToString();
		return "Enum";
	}

	if (prop->IsA<UConst>()) {
		UConst* constProp = (UConst*)prop;
		if (!constProp || IsBadReadPtr(constProp, sizeof(UConst)))
			return "Const <invalid>";
		if (!IsBadReadPtr(&constProp->Name, sizeof(FName)) && constProp->Name.IsValid())
			return "Const " + constProp->Name.ToString();
		return "Const";
	}

	return RemoveProperty(prop->Class->GetName());
}

UClass* FunctionLogger::GetParent(UClass* Class)
{
	if (Class && Class->SuperField)
		return (UClass*)Class->SuperField;
	return nullptr;
}

bool FunctionLogger::IsValidNumber(const std::string& str)
{
	if (str.empty())
		return false;

	size_t i = 0;
	if (str[i] == '+' || str[i] == '-')
		i++;

	bool hasDigit = false;
	bool hasDot = false;

	for (; i < str.length(); i++) {
		if (std::isdigit(str[i])) {
			hasDigit = true;
		} else if (str[i] == '.' && !hasDot) {
			hasDot = true;
		} else {
			return false;
		}
	}

	return hasDigit;
}



std::string FunctionLogger::ProcessFieldValue(UProperty* prop, object& obj, uint32_t offset)
{
	if (!prop || !obj.Obj)
		return "NULL";
	if (IsBadReadPtr(prop, sizeof(UProperty)))
		return "Invalid Property";
	if (!prop->Class || IsBadReadPtr(prop->Class, sizeof(UClass)))
		return "Invalid Property Class";

	if (prop->PropertyFlags & (0x400 | 0x100))
		return "";

	BYTE* objPtr = (BYTE*)obj.Obj;
	void* valuePtr = reinterpret_cast<void*>(objPtr + prop->Offset + offset);

	std::string propName = "UnknownProp";
	if (!IsBadReadPtr(&prop->Name, sizeof(FName)) && prop->Name.IsValid())
		propName = prop->Name.ToString();

	std::stringstream ss;
	ss << prop;
	std::string textBoxID = "##" + ss.str() + " " + propName + std::to_string(offset);

	if (prop->IsA<UStructProperty>()) {
			ImGui::SameLine();
			UStructProperty* structProp = (UStructProperty*)prop;

			if (!structProp || !structProp->Struct || IsBadReadPtr(structProp->Struct, sizeof(UStruct))) {
				ImGui::Text("Invalid Struct");
				return "Invalid Struct";
			}

			if (IsBadReadPtr(&structProp->Struct->Children, sizeof(void*))) {
				ImGui::Text("Invalid Struct Children");
				return "Invalid Struct Children";
			}

			if (ImGui::TreeNode(textBoxID.c_str()))
			{
				offset = prop->Offset;

				if (structProp->Struct->Children) {
					int fieldCount = 0;
					for (UField* field = structProp->Struct->Children; field && fieldCount < 100; field = field->Next) {
						if (!field || IsBadReadPtr(field, sizeof(UField))) break;
						fieldCount++;
						if (field->Next && IsBadReadPtr(field->Next, sizeof(UField))) break;

						UProperty* newProp = reinterpret_cast<UProperty*>(field);
						if (!newProp || !newProp->Class) continue;

						std::string type = ProcessType(newProp);
						std::string name = "UnknownField";
						if (!IsBadReadPtr(&field->Name, sizeof(FName)) && field->Name.IsValid())
							name = field->Name.ToString();

						ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str());
						ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

						ProcessFieldValue(newProp, obj, offset);
					}
				}

				ImGui::TreePop();
			}
		}
		else if (prop->IsA<UScriptStruct>()) {
			ImGui::SameLine();
			UScriptStruct* scriptStruct = (UScriptStruct*)prop;

			if (!scriptStruct || IsBadReadPtr(scriptStruct, sizeof(UScriptStruct))) {
				ImGui::Text("Invalid ScriptStruct");
				return "Invalid ScriptStruct";
			}

			if (IsBadReadPtr(&scriptStruct->Children, sizeof(void*))) {
				ImGui::Text("Invalid ScriptStruct Children");
				return "Invalid ScriptStruct Children";
			}

			if (ImGui::TreeNode(textBoxID.c_str()))
			{
				offset = prop->Offset;

				if (scriptStruct->Children) {
					int fieldCount = 0;
					for (UField* field = scriptStruct->Children; field && fieldCount < 100; field = field->Next) {
						if (!field || IsBadReadPtr(field, sizeof(UField))) break;
						fieldCount++;
						if (field->Next && IsBadReadPtr(field->Next, sizeof(UField))) break;

						UProperty* newProp = reinterpret_cast<UProperty*>(field);
						if (!newProp || !newProp->Class) continue;

						std::string type = ProcessType(newProp);
						std::string name = "UnknownField";
						if (!IsBadReadPtr(&field->Name, sizeof(FName)) && field->Name.IsValid())
							name = field->Name.ToString();

						ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str());
						ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

						ProcessFieldValue(newProp, obj, offset);
					}
				}

				ImGui::TreePop();
			}
		}
		else if (prop->IsA<UStrProperty>()) {
			FString* valueAddress = (FString*)valuePtr;

			std::string buffer = (*valueAddress).ToString();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
				*valueAddress = Format::ToFString(buffer);
			}
		}
		else if (prop->IsA<UQWordProperty>()) {
			uint64_t* valueAddress = (uint64_t*)valuePtr;

			std::string buffer = std::to_string(*valueAddress);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
				if (IsValidNumber(buffer)) {
					*valueAddress = std::stoull(buffer);
				}
			}
		}
		else if (prop->IsA<UObjectProperty>()) {
			if (IsBadReadPtr(valuePtr, sizeof(UObject*))) {
				ImGui::SameLine();
				ImGui::Text("Invalid Memory");
				return "Invalid Memory";
			}

			UObject** valueAddress = (UObject**)valuePtr;
			UObject* objectValue = *valueAddress;

			ImGui::SameLine();
			if (objectValue && !IsBadReadPtr(objectValue, sizeof(UObject))) {
				if (objectValue->Class && !IsBadReadPtr(objectValue->Class, sizeof(UClass))) {
					if (ImGui::TreeNode(textBoxID.c_str(), propName.c_str())) {
						object tempObj = CreateObject(objectValue->Class, objectValue);
						RenderObjectProperties(tempObj);
						ImGui::TreePop();
					}
				}
				else {
					ImGui::Text("Invalid Class");
				}
			}
			else {
				ImGui::Text("NULL");
			}
		}
		else if (prop->IsA<UComponentProperty>()) {
			// Validate the memory before reading the pointer
			if (IsBadReadPtr(valuePtr, sizeof(UObject*))) {
				ImGui::SameLine();
				ImGui::Text("Invalid Memory");
				return "Invalid Memory";
			}

			UObject** valueAddress = (UObject**)valuePtr;
			UObject* componentValue = *valueAddress;

			ImGui::SameLine();
			if (componentValue && !IsBadReadPtr(componentValue, sizeof(UObject))) {
				std::string componentName = "Unknown";
				std::string componentClass = "Unknown";

				if (!IsBadReadPtr(componentValue, sizeof(UObject))) {
					componentName = componentValue->GetName();
					if (componentValue->Class && !IsBadReadPtr(componentValue->Class, sizeof(UClass))) {
						componentClass = componentValue->Class->GetName();
					}
				}

				std::string displayText = "Component: " + componentClass + ": " + componentName;
				ImGui::Text("%s", displayText.c_str());
			}
			else {
				ImGui::Text("NULL Component");
			}
		}
		else if (prop->IsA<UClassProperty>()) {
			// Validate the memory before reading the pointer
			if (IsBadReadPtr(valuePtr, sizeof(UClass*))) {
				ImGui::SameLine();
				ImGui::Text("Invalid Memory");
				return "Invalid Memory";
			}

			UClass** valueAddress = (UClass**)valuePtr;
			UClass* classValue = *valueAddress;

			ImGui::SameLine();
			if (classValue && !IsBadReadPtr(classValue, sizeof(UClass))) {
				std::string className = classValue->GetName();
				ImGui::Text("Class: %s", className.c_str());
			}
			else {
				ImGui::Text("NULL Class");
			}
		}
		else if (prop->IsA<UNameProperty>()) {
			FName* valueAddress = (FName*)valuePtr;

			std::string buffer = (*valueAddress).ToString();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
				*valueAddress = Format::ToFName(buffer);
			}
		}
		else if (prop->IsA<UMapProperty>()) {
			ImGui::SameLine();
			ImGui::Text("Map (not implemented)");
			}
		else if (prop->IsA<UIntProperty>()) {
			INT* valueAddress = (INT*)valuePtr;

			std::string buffer = std::to_string(*valueAddress);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
				if (!IsValidNumber(buffer))
					*valueAddress = 1;
				else
					*valueAddress = std::stoi(buffer);
			}
		}
		else if (prop->IsA<UInterfaceProperty>()) {
			ImGui::SameLine();
			ImGui::Text("Interface (not editable)");
}
		else if (prop->IsA<UFloatProperty>()) {
			float* valueAddress = (float*)valuePtr;

			std::string buffer = std::to_string(*valueAddress);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
				if (!IsValidNumber(buffer))
					*valueAddress = 1;
				else
					*valueAddress = std::stof(buffer);
			}
		}
		else if (prop->IsA<UDelegateProperty>()) {
			ImGui::SameLine();
			ImGui::Text("Delegate (not editable)");
		}
		else if (prop->IsA<UByteProperty>()) {
			UByteProperty* byteProp = (UByteProperty*)prop;
			BYTE* valueAddress = (BYTE*)valuePtr;

			ImGui::SameLine();
			if (byteProp->Enum && !IsBadReadPtr(byteProp->Enum, sizeof(UEnum))) {
				// It's an enum byte property - show as combobox
				UEnum* enumObj = byteProp->Enum;
				int currentValue = (int)*valueAddress;

				// Get current enum name
				std::string currentEnumName = "Unknown";
				if (!IsBadReadPtr(&enumObj->Names, sizeof(TArray<FName>))) {
					int numNames = enumObj->Names.Num();
					if (currentValue >= 0 && currentValue < numNames) {
						FName enumValueName = enumObj->Names[currentValue];
						if (enumValueName.IsValid()) {
							currentEnumName = enumValueName.ToString();
						}
					}
				}

				ImGui::SetNextItemWidth(150);
				if (ImGui::BeginCombo(textBoxID.c_str(), currentEnumName.c_str())) {
					// List all enum values
					if (!IsBadReadPtr(&enumObj->Names, sizeof(TArray<FName>))) {
						int numNames = enumObj->Names.Num();
						for (int i = 0; i < numNames && i < 100; i++) {
							FName enumValueName = enumObj->Names[i];
							if (enumValueName.IsValid()) {
								bool isSelected = (i == currentValue);
								std::string displayName = enumValueName.ToString();
								if (ImGui::Selectable(displayName.c_str(), isSelected)) {
									*valueAddress = (BYTE)i;
								}
								if (isSelected) {
									ImGui::SetItemDefaultFocus();
								}
							}
						}
					}
					ImGui::EndCombo();
				}
			}
			else {
				std::string buffer = std::to_string((int)*valueAddress);
				ImGui::SetNextItemWidth(75);
				if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
					if (IsValidNumber(buffer)) {
						int val = std::stoi(buffer);
						if (val >= 0 && val <= 255) {
							*valueAddress = (BYTE)val;
						}
					}
				}
			}
			}
		else if (prop->IsA<UBoolProperty>()) {
			BOOL* valueAddress = (BOOL*)valuePtr;
			BYTE Mask = ((UBoolProperty*)prop)->BitMask;
			int w = (*valueAddress & Mask) ? 1 : 0;

			std::string buffer = std::to_string(w);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);

			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
				if (IsValidNumber(buffer) && stoi(buffer))
					*valueAddress |= Mask;
				else
					*valueAddress &= ~Mask;
			}
		}
		else if (prop->IsA<UArrayProperty>()) {
			ImGui::SameLine();

			if (ImGui::TreeNode(textBoxID.c_str()))
			{
				UArrayProperty* arrayProp = (UArrayProperty*)(prop);
				if (!arrayProp || !arrayProp->Inner || IsBadReadPtr(arrayProp->Inner, sizeof(UProperty))) {
					ImGui::Text("Invalid array property.");
					ImGui::TreePop();
					return "Invalid array property.";
				}

				BYTE* basePtr = (BYTE*)obj.Obj + arrayProp->Offset + offset;

				if (IsBadReadPtr(basePtr, sizeof(void*) + sizeof(int32_t) * 2)) {
					ImGui::Text("Invalid array memory.");
					ImGui::TreePop();
					return "Invalid array memory.";
				}

				void** dataPtr = reinterpret_cast<void**>(basePtr);
				int* numElements = reinterpret_cast<int*>(basePtr + sizeof(void*));

				if (!dataPtr || !*dataPtr || !numElements || *numElements <= 0 || *numElements > 10000)
				{
					ImGui::Text("Array is empty or invalid.");
					ImGui::TreePop();
					return "Array is empty or invalid.";
				}

				void* arrayData = *dataPtr;
				int arraySize = *numElements;
				int elementSize = arrayProp->Inner->ElementSize;

				if (IsBadReadPtr(arrayData, elementSize * arraySize)) {
					ImGui::Text("Invalid array data memory.");
					ImGui::TreePop();
					return "Invalid array data memory.";
				}

				for (int i = 0; i < arraySize && i < 100; ++i)
				{
					BYTE* elementDataPtr = reinterpret_cast<BYTE*>(arrayData) + (i * elementSize);

					if (IsBadReadPtr(elementDataPtr, elementSize)) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[%d] Invalid element", i);
						continue;
					}

					ImGui::Text("[%d]", i);
					ImGui::SameLine();

					if (arrayProp->Inner->IsA<UObjectProperty>())
					{
						UObject* elementObject = *reinterpret_cast<UObject**>(elementDataPtr);

						if (elementObject && !IsBadReadPtr(elementObject, sizeof(UObject)) && elementObject->Class && !IsBadReadPtr(elementObject->Class, sizeof(UClass)))
						{
							object newObj = CreateObject(elementObject->Class, elementObject);
							RenderObjectProperties(newObj);
						}
						else
						{
							ImGui::Text("NULL Object");
						}
					}
					else
					{
						object tempObj = obj;
						tempObj.Obj = reinterpret_cast<UObject*>(elementDataPtr - arrayProp->Inner->Offset);

						std::string elementType = ProcessType(arrayProp->Inner);
						ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), elementType.c_str());
						ImGui::SameLine();

						ProcessFieldValue(arrayProp->Inner, tempObj, 0);
					}
				}
				ImGui::TreePop();
			}
		}
		else if (prop->IsA<UEnum>()) {
			ImGui::SameLine();
			UEnum* enumProp = (UEnum*)prop;

			// Validate enum pointer
			if (!enumProp || IsBadReadPtr(enumProp, sizeof(UEnum))) {
				ImGui::Text("Invalid Enum");
				return "Invalid Enum";
			}

			if (ImGui::TreeNode(textBoxID.c_str()))
			{
				// Validate Names array
				if (!IsBadReadPtr(&enumProp->Names, sizeof(TArray<FName>))) {
					int numValues = enumProp->Names.Num();
					for (int i = 0; i < numValues && i < 100; i++) {
						FName enumName = enumProp->Names[i];
						if (enumName.IsValid()) {
							ImGui::Text("[%d] %s", i, enumName.ToString().c_str());
						}
					}
				} else {
					ImGui::Text("Invalid enum names");
				}
				ImGui::TreePop();
			}
		}
		else if (prop->IsA<UConst>()) {
			ImGui::SameLine();
			UConst* constProp = (UConst*)prop;

			// Validate const pointer
			if (!constProp || IsBadReadPtr(constProp, sizeof(UConst))) {
				ImGui::Text("Invalid Const");
				return "Invalid Const";
			}

			// Display const value (read-only)
			if (!IsBadReadPtr(&constProp->Value, sizeof(FString))) {
				std::string constValue = constProp->Value.ToString();
				ImGui::Text("= %s", constValue.c_str());
			} else {
				ImGui::Text("Invalid const value");
			}
		}
	else {
		return "NOT HANDLED";
	}
	return "NOT PROCESSED";
}



void FunctionLogger::RenderObjectInfo(object& obj)
{
	std::stringstream ss;
	ss << obj.Obj;
	std::string ID = "##" + ss.str();

	if (ImGui::TreeNode(("Info" + ID).c_str())) {
		std::string fullName = obj.Class->GetFullName();
		std::string ptrStr = ss.str();
		ImGui::InputText((ID + "fn").c_str(), &fullName, ImGuiInputTextFlags_ReadOnly);
		ImGui::InputText((ID + "ptr").c_str(), &ptrStr, ImGuiInputTextFlags_ReadOnly);
		ImGui::TreePop();
	}
}

void FunctionLogger::RenderFields(object& obj)
{
	std::stringstream ss;
	ss << obj.Obj;
	std::string ID = "##" + ss.str();

	if (ImGui::TreeNode(("Fields" + ID).c_str())) {
		for (UProperty* field : obj.fields) {
			if (!field)
				continue;

			std::string type = ProcessType(field);
			std::string name = "UnknownField";
			if (!IsBadReadPtr(&field->Name, sizeof(FName)) && field->Name.IsValid())
				name = field->Name.ToString();

			ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str());
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

			ProcessFieldValue(field, obj, 0);
		}
		ImGui::TreePop();
	}
}

void FunctionLogger::RenderFunctions(object& obj)
{
	std::stringstream ss;
	ss << obj.Obj;
	std::string ID = "##" + ss.str();

	if (ImGui::TreeNode(("Functions" + ID).c_str())) {
		for (UProperty* func : obj.functions) {
			if (!func)
				continue;

			std::string name = "UnknownFunction";
			if (!IsBadReadPtr(&func->Name, sizeof(FName)) && func->Name.IsValid())
				name = func->Name.ToString();

			ImGui::TextColored(ImVec4(0.72f, 0.52f, 0.93f, 1.0f), name.c_str());
		}
		ImGui::TreePop();
	}
}

void FunctionLogger::RenderObjectProperties(object& currentObject)
{
	if (!currentObject.Class || !currentObject.Obj)
		return;

	object obj = currentObject;
	int depth = 0;

	while (depth <= 10) {
		if (!obj.Class || !obj.Obj)
			return;

		std::stringstream ss;
		ss << obj.Obj;
		std::string ID = "##" + ss.str();

		if (ImGui::TreeNode((obj.Class->GetName() + ID).c_str())) {
			RenderObjectInfo(obj);
			RenderFields(obj);
			RenderFunctions(obj);
			ImGui::TreePop();
		}

		obj = CreateObject(GetParent(obj.Class), obj.Obj);
		if (obj.Class && obj.Class->GetName() == "Object")
			break;
		depth++;
	}
}

void FunctionLogger::RenderObjectList()
{
	if (ImGui::InputTextWithHint("##classSearchQuery", "Class Name", &searchQuery)) {}
	ImGui::SameLine();
	if (ImGui::Button("Search"))
		searchResults = GetAllInstancesOf(searchQuery);

	if (ImGui::BeginListBox("##searchList", ImGui::GetContentRegionAvail())) {
		for (int i = 0; i < searchResults.size(); i++) {
			bool isSelected = (selectedIndex == i);
			std::string label = searchResults[i].className + "##" + std::to_string(i);

			if (ImGui::Selectable(label.c_str(), isSelected))
				selectedIndex = i;
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndListBox();
	}
}

void FunctionLogger::RenderObjectExplorer()
{
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	ImGuiStyle& style = ImGui::GetStyle();
	float halfWidth = 0.5f * windowSize.x - style.ItemSpacing.x / 2;

	ImGui::BeginChild("##Search", ImVec2(halfWidth, windowSize.y), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Search");
		ImGui::EndMenuBar();
	}
	RenderObjectList();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##Properties", ImVec2(halfWidth, windowSize.y), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Properties");
		ImGui::EndMenuBar();
	}
	if (selectedIndex >= 0 && selectedIndex < searchResults.size())
	{
		RenderObjectProperties(searchResults[selectedIndex]);
	}
	ImGui::EndChild();
}

void FunctionLogger::RenderFunctionLogger()
{
}

class FunctionLogger FuncLog{};