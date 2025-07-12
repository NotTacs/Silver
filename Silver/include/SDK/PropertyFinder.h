#pragma once

namespace SDK
{
	struct PropertyInfo
	{
		void* Prop;
		std::string PropName;
		int32 Offset;
	};
	class PropertyFinder
	{
	public:
		static PropertyInfo FindPropertyByName(const std::string& ClassName, const std::string& PropName);
	};
}