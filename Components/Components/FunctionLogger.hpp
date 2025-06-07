#pragma once
#include "../Component.hpp"

struct object {
	UObject* Obj;
	UClass* Class;
	std::string className;
	std::string fullName;
	std::vector<UProperty*> fields;
	std::vector<UProperty*> functions;
};

class FunctionLogger : public Component
{
public:
	FunctionLogger();
	~FunctionLogger() override;

public:
	void OnCreate() override;
	void OnDestroy() override;

	static std::string toLowercase(const std::string& str);
	static bool containsSubstring(const std::string& str, const std::string& substr);
	static std::vector<std::string> splitString(const std::string& input, char delimiter);
	bool ClassOrParentMatches(UClass* Class, const std::string& query, bool includeInheritance);
	object createObject(UClass* Class, UObject* Object);
	std::vector<object> GetAllInstancesOf(std::string query);
	static std::string removeProperty(const std::string& input);
	static std::string processType(UProperty* prop);
	UClass* getParent(UClass* Class);
	int isValidNumber(std::string str);
	std::string processFieldValue(UProperty* prop, object Object, uint32_t offset);
	void generateInfo(object obj);

public:

};

extern class FunctionLogger FuncLog;