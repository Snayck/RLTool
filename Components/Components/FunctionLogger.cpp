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

	obj.Obj = Object;
	obj.Class = Class;
	obj.className = Class->GetName();
	obj.fullName = Class->GetFullName();

	for (UField* field = Class->Children; field; field = field->Next) {
		if (!field) break; // Additional safety check

		UProperty* prop = reinterpret_cast<UProperty*>(field);
		if (!prop || !prop->Class) continue; // Null check for prop and prop->Class

		if (prop->Class->GetName() == _("Function"))
			obj.functions.push_back(prop);
		else
			obj.fields.push_back(prop);
	}
	return obj;
}

std::vector<object> FunctionLogger::GetAllInstancesOf(std::string query)
{
	std::vector<object> objectInstances;

	// Null check for GObjObjects
	if (!UObject::GObjObjects()) {
		return objectInstances;
	}

	for (UObject* uObject : *UObject::GObjObjects())
	{
		if (uObject && uObject->Class) // Added Class null check
		{
			if (uObject->GetFullName().find(_("Default__")) == std::string::npos)
			{
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
	if (!prop || !prop->Class)
		return _("NULL");

	if (prop->IsA<UStructProperty>()) {
		UStructProperty* structProp = (UStructProperty*)prop;
		if (structProp->Struct) {
			return _("Struct ") + structProp->Struct->GetName();
		}
		return _("Struct NULL");
	}
	else if (prop->IsA<UStrProperty>()) {
		return _("String");
	}
	else if (prop->IsA<UQWordProperty>()) {
		return _("QWord");
	}
	else if (prop->IsA<UObjectProperty>()) {
		UObjectProperty* objProp = (UObjectProperty*)prop;
		if (objProp->PropertyClass) {
			return objProp->PropertyClass->GetName() + _("*");
		}
		return _("Object*");
	}
	else if (prop->IsA<UComponentProperty>()) {
		return _("Component");
	}
	else if (prop->IsA<UClassProperty>()) {
		return _("Class");
	}
	else if (prop->IsA<UNameProperty>()) {
		return _("Name");
	}
	else if (prop->IsA<UMapProperty>()) {
		return _("Map");
	}
	else if (prop->IsA<UIntProperty>()) {
		return _("Int");
	}
	else if (prop->IsA<UInterfaceProperty>()) {
		return _("Interface");
	}
	else if (prop->IsA<UFloatProperty>()) {
		return _("Float");
	}
	else if (prop->IsA<UDelegateProperty>()) {
		return _("Delegate");
	}
	else if (prop->IsA<UByteProperty>()) {
		return _("Byte");
	}
	else if (prop->IsA<UBoolProperty>()) {
		return _("Bool");
	}
	else if (prop->IsA<UArrayProperty>()) {
		UArrayProperty* arrayProp = (UArrayProperty*)prop;
		if (arrayProp->Inner) {
			std::string type = processType(arrayProp->Inner);
			return _("Array<") + type + _(">");
		}
		return _("Array<Unknown>");
	}

	return removeProperty(prop->Class->GetName());
}

UClass* FunctionLogger::getParent(UClass* Class) {
	if (Class && Class->SuperField)
		return (UClass*)Class->SuperField;
	return nullptr;
}

int FunctionLogger::isValidNumber(std::string str)
{
	if (str.empty()) return 0; // Check for empty string

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
	if (!prop || !Object.Obj || !prop->Class)
		return _("NULL");

	BYTE* obj = (BYTE*)Object.Obj;
	if (prop->PropertyFlags & 0x400 || prop->PropertyFlags & 0x100) {
		return _("PROTECTED_PROPERTY");
	}

	void* propPtr = reinterpret_cast<void*>((prop));
	std::stringstream ss; ss << propPtr;
	std::string textBoxID = _("## ") + ss.str() + " " + prop->Name.ToString() + std::to_string(offset);

	void* valuePtr = reinterpret_cast<void*>(obj + prop->Offset + offset);
	std::stringstream ss1; ss1 << valuePtr;
	std::string valuePointerID = _("## ") + ss1.str();

	if (prop->IsA<UStructProperty>()) {
		UStructProperty* structProp = (UStructProperty*)prop;
		if (!structProp->Struct) {
			ImGui::SameLine();
			ImGui::Text("NULL Struct");
			return _("NULL_STRUCT");
		}

		ImGui::SameLine();
		if (ImGui::TreeNode((_("## am a struct") + textBoxID).c_str()))
		{
			offset = prop->Offset;
			for (UField* field = structProp->Struct->Children; field; field = field->Next) {
				if (!field) break; // Safety check

				std::string type = processType((UProperty*)field);
				std::string name = field->Name.ToString();

				ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str()); ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

				UProperty* newProp = reinterpret_cast<UProperty*>(field);
				if (newProp) {
					std::string item = processFieldValue(newProp, Object, offset);
				}
			}
			ImGui::TreePop();
		}
	}
	else if (prop->IsA<UScriptStruct>()) {
		UScriptStruct* scriptStruct = (UScriptStruct*)prop;
		ImGui::SameLine();
		if (ImGui::TreeNode((_("## am a sstruct") + textBoxID).c_str()))
		{
			offset = prop->Offset;
			for (UField* field = scriptStruct->Children; field; field = field->Next) {
				if (!field) break; // Safety check

				std::string type = processType((UProperty*)field);
				std::string name = field->Name.ToString();

				ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str()); ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

				UProperty* newProp = reinterpret_cast<UProperty*>(field);
				if (newProp) {
					std::string item = processFieldValue(newProp, Object, offset);
				}
			}
			ImGui::TreePop();
		}
	}
	else if (prop->IsA<UStrProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL String");
			return _("NULL_STRING");
		}

		FString* valueAddress = (FString*)valuePtr;
		try {
			std::string buffer = (*valueAddress).ToString();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
				*valueAddress = Format::ToFString(buffer);
			}
		}
		catch (...) {
			ImGui::SameLine();
			ImGui::Text("Invalid String");
			return _("INVALID_STRING");
		}
	}
	else if (prop->IsA<UQWordProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL QWord");
			return _("NULL_QWORD");
		}

		uint64_t* valueAddress = (uint64_t*)valuePtr;
		std::string buffer = std::to_string(*valueAddress);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(75);
		if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
			if (isValidNumber(buffer)) {
				try {
					*valueAddress = std::stoull(buffer);
				}
				catch (...) {
					// Handle conversion error
				}
			}
		}
	}
	else if (prop->IsA<UObjectProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Object Ptr");
			return _("NULL_OBJECT_PTR");
		}

		UObject** valueAddress = (UObject**)valuePtr;
		UObject* objectValue = *valueAddress;

		ImGui::SameLine();
		if (objectValue && objectValue->Class) {
			generateInfo(createObject(objectValue->Class, objectValue));
		}
		else {
			ImGui::Text("NULL");
		}
	}
	else if (prop->IsA<UComponentProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Component Ptr");
			return _("NULL_COMPONENT_PTR");
		}

		UObject** valueAddress = (UObject**)valuePtr;
		UObject* componentValue = *valueAddress;

		ImGui::SameLine();
		if (componentValue) {
			std::string componentName = componentValue->GetName();
			std::string componentClass = (componentValue->Class) ? componentValue->Class->GetName() : "Unknown";

			std::string displayText = "Component: " + componentClass + ": " + componentName;
			ImGui::Text("%s", displayText.c_str());
		}
		else {
			ImGui::Text("NULL Component");
		}
	}
	else if (prop->IsA<UClassProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Class Ptr");
			return _("NULL_CLASS_PTR");
		}

		UClass** valueAddress = (UClass**)valuePtr;
		UClass* classValue = *valueAddress;

		ImGui::SameLine();
		if (classValue) {
			std::string className = classValue->GetName();
			ImGui::Text("Class: %s", className.c_str());
		}
		else {
			ImGui::Text("NULL Class");
		}
	}
	else if (prop->IsA<UNameProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Name");
			return _("NULL_NAME");
		}

		FName* valueAddress = (FName*)valuePtr;
		try {
			std::string buffer = (*valueAddress).ToString();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
				*valueAddress = Format::ToFName(buffer);
			}
		}
		catch (...) {
			ImGui::SameLine();
			ImGui::Text("Invalid Name");
			return _("INVALID_NAME");
		}
	}
	else if (prop->IsA<UMapProperty>()) {
		ImGui::SameLine();
		ImGui::Text("Map (not implemented)");
	}
	else if (prop->IsA<UIntProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Int");
			return _("NULL_INT");
		}

		INT* valueAddress = (INT*)valuePtr;
		std::string buffer = std::to_string(*valueAddress);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(75);
		if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
			if (!isValidNumber(buffer))
				*valueAddress = 1;
			else {
				try {
					*valueAddress = std::stoi(buffer);
				}
				catch (...) {
					*valueAddress = 1;
				}
			}
		}
	}
	else if (prop->IsA<UInterfaceProperty>()) {
		ImGui::SameLine();
		ImGui::Text("Interface (not editable)");
	}
	else if (prop->IsA<UFloatProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Float");
			return _("NULL_FLOAT");
		}

		float* valueAddress = (float*)valuePtr;
		std::string buffer = std::to_string(*valueAddress);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(75);
		if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
			if (!isValidNumber(buffer))
				*valueAddress = 1.0f;
			else {
				try {
					*valueAddress = std::stof(buffer);
				}
				catch (...) {
					*valueAddress = 1.0f;
				}
			}
		}
	}
	else if (prop->IsA<UDelegateProperty>()) {
		ImGui::SameLine();
		ImGui::Text("Delegate (not editable)");
	}
	else if (prop->IsA<UByteProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Byte");
			return _("NULL_BYTE");
		}

		UByteProperty* byteProp = (UByteProperty*)prop;
		BYTE* valueAddress = (BYTE*)valuePtr;

		ImGui::SameLine();
		if (byteProp->Enum) {
			std::string enumName = byteProp->Enum->GetName();
			ImGui::Text("Enum %s: %d", enumName.c_str(), (int)*valueAddress);
		}
		else {
			std::string buffer = std::to_string((int)*valueAddress);
			ImGui::SetNextItemWidth(75);
			if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
				if (isValidNumber(buffer)) {
					try {
						int val = std::stoi(buffer);
						if (val >= 0 && val <= 255) {
							*valueAddress = (BYTE)val;
						}
					}
					catch (...) {
						// Handle conversion error
					}
				}
			}
		}
	}
	else if (prop->IsA<UBoolProperty>()) {
		if (!valuePtr) {
			ImGui::SameLine();
			ImGui::Text("NULL Bool");
			return _("NULL_BOOL");
		}

		UBoolProperty* boolProp = (UBoolProperty*)prop;
		BOOL* valueAddress = (BOOL*)valuePtr;
		BYTE Mask = boolProp->BitMask;
		int w = (*valueAddress & Mask) ? 1 : 0;

		std::string buffer = std::to_string(w);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(75);

		if (ImGui::InputText(textBoxID.c_str(), &buffer, ImGuiInputTextFlags_EnterReturnsTrue || ImGuiInputTextFlags_CharsDecimal)) {
			if (isValidNumber(buffer)) {
				try {
					if (stoi(buffer))
						*valueAddress |= Mask;
					else
						*valueAddress &= ~Mask;
				}
				catch (...) {
					// Handle conversion error
				}
			}
		}
	}
	else if (prop->IsA<UArrayProperty>()) {
		ImGui::SameLine();

		if (ImGui::TreeNode((_("## am an arrayuyy") + textBoxID).c_str()))
		{
			UArrayProperty* arrayProp = (UArrayProperty*)(prop);
			if (!arrayProp || !arrayProp->Inner) {
				ImGui::Text("Invalid array property.");
				ImGui::TreePop();
				return _("INVALID_ARRAY");
			}

			if (!valuePtr) {
				ImGui::Text("NULL array pointer.");
				ImGui::TreePop();
				return _("NULL_ARRAY_PTR");
			}

			BYTE* basePtr = obj + arrayProp->Offset + offset;
			void** dataPtr = reinterpret_cast<void**>(basePtr);
			int* numElements = reinterpret_cast<int*>(basePtr + sizeof(void*));

			if (!dataPtr || !*dataPtr || !numElements || *numElements <= 0)
			{
				ImGui::Text("Array is empty or invalid.");
				ImGui::TreePop();
				return _("EMPTY_ARRAY");
			}

			void* arrayData = *dataPtr;
			int arraySize = *numElements;
			int elementSize = arrayProp->Inner->ElementSize;

			// Clamp array size to prevent crashes
			int maxElements = min(arraySize, 100);

			for (int i = 0; i < maxElements; ++i)
			{
				try {
					BYTE* elementDataPtr = reinterpret_cast<BYTE*>(arrayData) + (i * elementSize);

					// Validate element pointer
					if (!elementDataPtr) continue;

					object tempObj = Object;
					tempObj.Obj = reinterpret_cast<UObject*>(elementDataPtr - arrayProp->Inner->Offset);

					std::string elementType = processType(arrayProp->Inner);
					ImGui::Text("[%d]", i);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), elementType.c_str());
					ImGui::SameLine();

					std::string elementValue = processFieldValue(arrayProp->Inner, tempObj, 0);
				}
				catch (...) {
					ImGui::Text("[%d] Error processing element", i);
					break; // Stop processing if we hit an error
				}
			}

			if (arraySize > 100) {
				ImGui::Text("... and %d more elements (truncated for safety)", arraySize - 100);
			}

			ImGui::TreePop();
		}
	}
	else if (prop->IsA<UEnum>()) {
		ImGui::SameLine();
		ImGui::Text("Enum (not implemented)");
	}
	else {
		ImGui::SameLine();
		ImGui::Text("NOT HANDLED");
		return _("NOT_HANDLED");
	}

	return _("PROCESSED");
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
			for (size_t i = 0; i < obj.fields.size(); i++) {
				if (!obj.fields[i]) continue; // Null check for field

				std::string type = processType(obj.fields[i]);
				std::string name = obj.fields[i]->Name.ToString();

				ImGui::TextColored(ImVec4(0.36f, 0.52f, 0.93f, 1.0f), type.c_str()); ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.93f, 0.72f, 0.36f, 1.0f), name.c_str());

				try {
					processFieldValue(obj.fields[i], obj);
				}
				catch (...) {
					ImGui::SameLine();
					ImGui::Text("Error processing field");
				}
			}

			ImGui::TreePop();
		}
		if (ImGui::TreeNode(("Functions" + ID).c_str()))
		{
			for (size_t i = 0; i < obj.functions.size(); i++) {
				if (!obj.functions[i]) continue; // Null check for function

				std::string functionName = obj.functions[i]->Name.ToString();
				ImGui::Text("%s", functionName.c_str());
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}

class FunctionLogger FuncLog{};