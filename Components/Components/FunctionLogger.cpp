#include "FunctionLogger.hpp"
#include "../Includes.hpp"

#include "../ImGui/Kiero/kiero.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_stdlib.h"

FunctionLogger::FunctionLogger() : Component("Function Logger", "Processes functions and object fields") { OnCreate(); }

FunctionLogger::~FunctionLogger() { OnDestroy(); }

void FunctionLogger::OnCreate() {}

void FunctionLogger::OnDestroy() {}

std::string FunctionLogger::toLowercase(const std::string& str) {
	std::string result;
	result.reserve(str.length());

	for (char c : str) {
		result.push_back(std::tolower(c));
	}

	return result;
}

bool FunctionLogger::containsSubstring(const std::string& str, const std::string& substr) {
	return str.find(substr) != std::string::npos;
}

std::vector<std::string> FunctionLogger::splitString(const std::string& input, char delimiter = ',') {
	std::vector<std::string> tokens;
	std::istringstream stream(input);
	std::string token;

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

bool FunctionLogger::ClassOrParentMatches(UClass* Class, const std::string& query, bool includeInheritance = true) {
	if (!Class)
		return false;

	// Check if the class name matches the query
	if (query != "") {
		if (containsSubstring(toLowercase(Class->GetName()), toLowercase(query))) { // checks class name for substring 'str'
			return true;
		}
		else {
			// Check if any parent class matches the query
			if (includeInheritance)
				return ClassOrParentMatches((UClass*)Class->SuperField, query); // recursive call
		}
	}
	return false; // if not getting inheritance and if there isnt a query it will just return false
}

object FunctionLogger::createObject(UClass* Class, UObject* Object) {
	object obj;
	if (!Class || !Object)
		return obj;

	// Validate Class pointer before accessing its methods
	if (IsBadReadPtr(Class, sizeof(UClass))) {
		return obj;
	}

	// Validate Object pointer
	if (IsBadReadPtr(Object, sizeof(UObject))) {
		return obj;
	}

	obj.Obj = Object;
	obj.Class = Class;
	obj.className = Class->GetName();
	obj.fullName = Class->GetFullName();

	// Validate Children pointer before iterating
	if (!IsBadReadPtr(&Class->Children, sizeof(void*)) && Class->Children) {
		int fieldCount = 0;
		for (UField* field = Class->Children; field && fieldCount < 1000; field = field->Next) {
			// Validate field pointer
			if (!field || IsBadReadPtr(field, sizeof(UField)))
				break;

			fieldCount++;

			// Validate Next pointer for next iteration
			if (field->Next && IsBadReadPtr(field->Next, sizeof(UField)))
				break;

			UProperty* prop = reinterpret_cast<UProperty*>(field);
			if (!prop || !prop->Class || IsBadReadPtr(prop->Class, sizeof(UClass)))
				continue;

			std::string propClassName = prop->Class->GetName();
			if (propClassName == _("Function"))
				obj.functions.push_back(prop);
			else
				obj.fields.push_back(prop);
		}
	}
	return obj;
}

std::vector<object> FunctionLogger::GetAllInstancesOf(std::string query)
{
	std::vector<object> objectInstances;

	for (UObject* uObject : *UObject::GObjObjects())
	{
		if (uObject)
		{
			if (uObject->GetFullName().find(_("Default__")) == std::string::npos)
			{
				bool ii = false;
				if (ClassOrParentMatches(uObject->Class, query)) {
					objectInstances.push_back(createObject(uObject->Class, uObject));
				}
			}
		}
	}

	return objectInstances;
}

std::string FunctionLogger::removeProperty(const std::string& input) {
	std::string output = input;
	size_t pos = 0;
	while ((pos = output.find(_("Property"), pos)) != std::string::npos) {
		output.erase(pos, 8); // Length of "Property" is 8
	}
	return output;
}

std::string FunctionLogger::processType(UProperty* prop) {
	if (!prop)
		return _("NULL");

	if (prop->IsA<UStructProperty>()) {
		UStructProperty* structProp = (UStructProperty*)prop;
		if (structProp->Struct && !IsBadReadPtr(structProp->Struct, sizeof(UStruct))) {
			return _("Struct ") + structProp->Struct->GetName();
		}
		return _("Struct <invalid>");
	}
	else if (prop->IsA<UStrProperty>()) {

	}
	else if (prop->IsA<UQWordProperty>()) {

	}
	else if (prop->IsA<UObjectProperty>()) {
		UObjectProperty* objProp = (UObjectProperty*)prop;
		if (objProp->PropertyClass && !IsBadReadPtr(objProp->PropertyClass, sizeof(UClass))) {
			return objProp->PropertyClass->GetName() + _("*");
		}
		return _("Object* <invalid>");
	}
	else if (prop->IsA<UComponentProperty>()) {

	}
	else if (prop->IsA<UClassProperty>()) {

	}
	else if (prop->IsA<UNameProperty>()) {

	}
	else if (prop->IsA<UMapProperty>()) {

	}
	else if (prop->IsA<UIntProperty>()) {

	}
	else if (prop->IsA<UInterfaceProperty>()) {

	}
	else if (prop->IsA<UFloatProperty>()) {

	}
	else if (prop->IsA<UDelegateProperty>()) {

	}
	else if (prop->IsA<UByteProperty>()) {

	}
	else if (prop->IsA<UBoolProperty>()) {

	}
	else if (prop->IsA<UArrayProperty>()) {
		UArrayProperty* arrayProp = (UArrayProperty*)prop;
		if (arrayProp->Inner && !IsBadReadPtr(arrayProp->Inner, sizeof(UProperty))) {
			std::string type = processType(arrayProp->Inner);
			return _("Array<") + type + _(">");
		}
		return _("Array<invalid>");
	}
	else if (prop->IsA<UEnum>()) {
		UEnum* enumProp = (UEnum*)prop;
		if (!enumProp || IsBadReadPtr(enumProp, sizeof(UEnum))) {
			return _("Enum <invalid>");
		}
		// Try to get the enum name
		if (!IsBadReadPtr(&enumProp->Name, sizeof(FName)) && enumProp->Name.IsValid()) {
			return _("Enum ") + enumProp->Name.ToString();
		}
		return _("Enum");
	}
	else if (prop->IsA<UConst>()) {
		UConst* constProp = (UConst*)prop;
		if (!constProp || IsBadReadPtr(constProp, sizeof(UConst))) {
			return _("Const <invalid>");
		}
		// Try to get the const name
		if (!IsBadReadPtr(&constProp->Name, sizeof(FName)) && constProp->Name.IsValid()) {
			return _("Const ") + constProp->Name.ToString();
		}
		return _("Const");
	}

	return removeProperty(prop->Class->GetName());
}

UClass* FunctionLogger::getParent(UClass* Class) {
	if (Class)
		if (Class->SuperField)
			return (UClass*)Class->SuperField;
	return nullptr;
}

int FunctionLogger::isValidNumber(std::string str)
{
	int i = 0, j = str.length() - 1;

	// Handling whitespaces
	while (i < str.length() && str[i] == ' ')
		i++;
	while (j >= 0 && str[j] == ' ')
		j--;

	if (i > j)
		return 0;

	// if string is of length 1 and the only
	// character is not a digit
	if (i == j && !(str[i] >= '0' && str[i] <= '9'))
		return 0;

	// If the 1st char is not '+', '-', '.' or digit
	if (str[i] != '.' && str[i] != '+'
		&& str[i] != '-' && !(str[i] >= '0' && str[i] <= '9'))
		return 0;

	// To check if a '.' or 'e' is found in given
	// string. We use this flag to make sure that
	// either of them appear only once.
	bool flagDotOrE = false;

	for (i; i <= j; i++) {
		// If any of the char does not belong to
		// {digit, +, -, ., e}
		if (str[i] != 'e' && str[i] != '.'
			&& str[i] != '+' && str[i] != '-'
			&& !(str[i] >= '0' && str[i] <= '9'))
			return 0;

		if (str[i] == '.') {
			// checks if the char 'e' has already
			// occurred before '.' If yes, return 0.
			if (flagDotOrE == true)
				return 0;

			// If '.' is the last character.
			if (i + 1 > str.length())
				return 0;

			// if '.' is not followed by a digit.
			if (!(str[i + 1] >= '0' && str[i + 1] <= '9'))
				return 0;
		}

		else if (str[i] == 'e') {
			// set flagDotOrE = 1 when e is encountered.
			flagDotOrE = true;

			// if there is no digit before 'e'.
			if (!(str[i - 1] >= '0' && str[i - 1] <= '9'))
				return 0;

			// If 'e' is the last Character
			if (i + 1 > str.length())
				return 0;

			// if e is not followed either by
			// '+', '-' or a digit
			if (str[i + 1] != '+' && str[i + 1] != '-'
				&& (str[i + 1] >= '0' && str[i] <= '9'))
				return 0;
		}
	}

	/* If the string skips all above cases, then
	it is numeric*/
	return 1;
}



std::string FunctionLogger::processFieldValue(UProperty* prop, object Object, uint32_t offset = 0) {
	if (!prop || !Object.Obj)
		return _("NULL");

	// Validate prop pointer is readable
	if (IsBadReadPtr(prop, sizeof(UProperty))) {
		return _("Invalid Property");
	}

	// Validate Class pointer before using IsA
	if (!prop->Class || IsBadReadPtr(prop->Class, sizeof(UClass))) {
		return _("Invalid Property Class");
	}

	BYTE* obj = (BYTE*)Object.Obj;
	if (prop->PropertyFlags & 0x400 || prop->PropertyFlags & 0x100) {}
	else {
		void* propPtr = reinterpret_cast<void*>((prop));
		std::stringstream ss; ss << propPtr;

		// Safely get property name
		std::string propName = "UnknownProp";
		if (!IsBadReadPtr(&prop->Name, sizeof(FName)) && prop->Name.IsValid()) {
			propName = prop->Name.ToString();
		}

		std::string textBoxID = _("## ") + ss.str() + " " + propName + std::to_string(offset);

		void* valuePtr = reinterpret_cast<void*>(obj + prop->Offset + offset);
		std::stringstream ss1; ss1 << valuePtr;
		std::string valuePointerID = _("## ") + ss1.str();


		if (prop->IsA<UStructProperty>()) {
			ImGui::SameLine();
			UStructProperty* structProp = (UStructProperty*)prop;

			// Validate struct pointer before accessing
			if (!structProp || !structProp->Struct || IsBadReadPtr(structProp->Struct, sizeof(UStruct))) {
				ImGui::Text("Invalid Struct");
				return "Invalid Struct";
			}

			// Validate Children pointer
			if (IsBadReadPtr(&structProp->Struct->Children, sizeof(void*))) {
				ImGui::Text("Invalid Struct Children");
				return "Invalid Struct Children";
			}

			if (ImGui::TreeNode((_("## am a struct") + textBoxID).c_str()))
			{
				offset = prop->Offset;

				if (structProp->Struct->Children) {
					int fieldCount = 0;
					for (UField* field = structProp->Struct->Children; field && fieldCount < 100; field = field->Next) {
						// Validate field pointer before dereferencing
						if (!field || IsBadReadPtr(field, sizeof(UField))) break;

						fieldCount++;

						// Validate Next pointer for next iteration
						if (field->Next && IsBadReadPtr(field->Next, sizeof(UField))) break;

						UProperty* newProp = reinterpret_cast<UProperty*>(field);
						if (!newProp || !newProp->Class) continue;

						std::string type = processType(newProp);

						// Safely get the field name - validate FName before calling ToString
						std::string name = "UnknownField";
						if (!IsBadReadPtr(&field->Name, sizeof(FName)) && field->Name.IsValid()) {
							name = field->Name.ToString();
						}

						ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str()); ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

						processFieldValue(newProp, Object, offset);
					}
				}

				ImGui::TreePop();
			}
		}
		else if (prop->IsA<UScriptStruct>()) {
			ImGui::SameLine();
			UScriptStruct* scriptStruct = (UScriptStruct*)prop;

			// Validate script struct pointer
			if (!scriptStruct || IsBadReadPtr(scriptStruct, sizeof(UScriptStruct))) {
				ImGui::Text("Invalid ScriptStruct");
				return "Invalid ScriptStruct";
			}

			// Validate Children pointer
			if (IsBadReadPtr(&scriptStruct->Children, sizeof(void*))) {
				ImGui::Text("Invalid ScriptStruct Children");
				return "Invalid ScriptStruct Children";
			}

			if (ImGui::TreeNode((_("## am a sstruct") + textBoxID).c_str()))
			{
				offset = prop->Offset;

				if (scriptStruct->Children) {
					int fieldCount = 0;
					for (UField* field = scriptStruct->Children; field && fieldCount < 100; field = field->Next) {
						// Validate field pointer before dereferencing
						if (!field || IsBadReadPtr(field, sizeof(UField))) break;

						fieldCount++;

						// Validate Next pointer for next iteration
						if (field->Next && IsBadReadPtr(field->Next, sizeof(UField))) break;

						UProperty* newProp = reinterpret_cast<UProperty*>(field);
						if (!newProp || !newProp->Class) continue;

						std::string type = processType(newProp);

						// Safely get the field name - validate FName before calling ToString
						std::string name = "UnknownField";
						if (!IsBadReadPtr(&field->Name, sizeof(FName)) && field->Name.IsValid()) {
							name = field->Name.ToString();
						}

						ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str()); ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

						processFieldValue(newProp, Object, offset);
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
				if (isValidNumber(buffer)) {
					*valueAddress = std::stoull(buffer);
				}
			}
		}
		else if (prop->IsA<UObjectProperty>()) {
			// Validate the memory before reading the pointer
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
					generateInfo(createObject(objectValue->Class, objectValue));
				} else {
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
				if (!isValidNumber(buffer))
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
				if (!isValidNumber(buffer))
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
				// Regular byte property
				std::string buffer = std::to_string((int)*valueAddress);
				ImGui::SetNextItemWidth(75);
				if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
					if (isValidNumber(buffer)) {
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
				if (isValidNumber(buffer) && stoi(buffer))
					*valueAddress |= Mask;
				else
					*valueAddress &= ~Mask;
			}
		}
		else if (prop->IsA<UArrayProperty>()) {
			ImGui::SameLine();

			if (ImGui::TreeNode((_("## am an arrayuyy") + textBoxID).c_str()))
			{
				// Get the UArrayProperty
				UArrayProperty* arrayProp = (UArrayProperty*)(prop);
				if (!arrayProp || !arrayProp->Inner || IsBadReadPtr(arrayProp->Inner, sizeof(UProperty))) {
					ImGui::Text("Invalid array property.");
					ImGui::TreePop();
					return "Invalid array property.";
				}

				// Calculate the pointer to the array's data in the object
				BYTE* basePtr = obj + arrayProp->Offset + offset;

				// Validate basePtr is readable
				if (IsBadReadPtr(basePtr, sizeof(void*) + sizeof(int32_t) * 2)) {
					ImGui::Text("Invalid array memory.");
					ImGui::TreePop();
					return "Invalid array memory.";
				}

				// Unreal Engine TArray structure layout:
				// struct TArray {
				//     void* Data;           // Pointer to array data
				//     int32 ArrayNum;       // Number of elements
				//     int32 ArrayMax;       // Allocated capacity
				// };

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

				// Validate array data pointer
				if (IsBadReadPtr(arrayData, elementSize * arraySize)) {
					ImGui::Text("Invalid array data memory.");
					ImGui::TreePop();
					return "Invalid array data memory.";
				}

				// Iterate over the array elements
				for (int i = 0; i < arraySize && i < 100; ++i) // Limit to 100 elements for safety
				{
					// Calculate the pointer to each element's data
					BYTE* elementDataPtr = reinterpret_cast<BYTE*>(arrayData) + (i * elementSize);

					// Validate element pointer
					if (IsBadReadPtr(elementDataPtr, elementSize)) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[%d] Invalid element", i);
						continue;
					}

					// Create a temporary object to represent this array element
					object tempObj = Object; // Copy the original object
					tempObj.Obj = reinterpret_cast<UObject*>(elementDataPtr - arrayProp->Inner->Offset); // Adjust base pointer

					std::string elementType = processType(arrayProp->Inner);
					ImGui::Text("[%d]", i);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), elementType.c_str());
					ImGui::SameLine();

					// Process the element using the array's inner property type
					// Pass the inner property and the adjusted object
					std::string elementValue = processFieldValue(arrayProp->Inner, tempObj, 0);
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
			return _("NOT HANDLED");
		}
	}
	return _("NOT PROCESSED");
}



void FunctionLogger::generateInfo(object obj) {
	if (!obj.Class || !obj.Obj)
		return;

	int* ptr = reinterpret_cast<int*>((obj.Obj));
	std::stringstream ss; ss << ptr;

	std::string rawPointer = ss.str();
	std::string ID = "##" + rawPointer;

	if (ImGui::TreeNode((obj.Class->GetName() + ID).c_str()))
	{
		if (ImGui::TreeNode((_("Info") + ID).c_str()))
		{
			std::string fullName = obj.Class->GetFullName();
			ImGui::InputText((ID + fullName).c_str(), &fullName, ImGuiInputTextFlags_ReadOnly);
			ImGui::InputText((ID).c_str(), &rawPointer, ImGuiInputTextFlags_ReadOnly);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode((_("Fields") + ID).c_str()))
		{
			for (int i = 0; i < obj.fields.size(); i++) {
				UProperty* field = obj.fields[i];
				if (!field) continue;

				std::string type = processType(field);

				// Safely get the field name - validate FName before calling ToString
				std::string name = "UnknownField";
				if (!IsBadReadPtr(&field->Name, sizeof(FName)) && field->Name.IsValid()) {
					name = field->Name.ToString();
				}

				ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str()); ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

				processFieldValue(field, obj);

			}



			ImGui::TreePop();
		}
		if (ImGui::TreeNode(("Functions" + ID).c_str()))
		{


			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}

class FunctionLogger FuncLog{};