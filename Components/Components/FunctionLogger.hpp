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
private:
	std::vector<object> searchResults;
	int selectedIndex;
	std::string searchQuery;

public:
	FunctionLogger();
	~FunctionLogger() override;

	void OnCreate() override;
	void OnDestroy() override;

	void RenderObjectExplorer();
	void RenderFunctionLogger();

	static std::string ProcessType(UProperty* prop);
	static std::string ToLowercase(const std::string& str);
	static bool ContainsSubstring(const std::string& str, const std::string& substr);
	static std::string RemoveProperty(const std::string& input);
	static std::vector<std::string> SplitString(const std::string& str, char delimiter);

private:
	object CreateObject(UClass* Class, UObject* Object);
	std::vector<object> GetAllInstancesOf(const std::string& query);
	UClass* GetParent(UClass* Class);

	void RenderObjectList();
	void RenderObjectProperties(object& obj);
	void RenderObjectInfo(object& obj);
	void RenderFields(object& obj);
	void RenderFunctions(object& obj);

	std::string ProcessFieldValue(UProperty* prop, object& obj, uint32_t offset);

	bool ClassOrParentMatches(UClass* Class, const std::string& query);
	bool IsValidNumber(const std::string& str);
};

extern class FunctionLogger FuncLog;