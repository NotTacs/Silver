#pragma once
namespace SDK
{
	struct FLogCategoryBase
	{
		FLogCategoryBase(const std::wstring& InCategoryName, ELogVerbosity::Type InVerbosity)
			:CategoryName(InCategoryName)
		{
			Verbosity = InVerbosity;
			CompileTimeVerbosity = InVerbosity;
		}

		const std::wstring& GetCategoryName() const
		{
			return CategoryName;
		}

		ELogVerbosity::Type GetVerbosity() const
		{
			return Verbosity;
		}

		void SetVerbosity(ELogVerbosity::Type Verbosity);

		inline ELogVerbosity::Type GetCompileTimeVerbosity() const
		{
			return CompileTimeVerbosity;
		}

	private:
		ELogVerbosity::Type Verbosity;
		bool DebugBreakOnLog;
		uint8_t DefaultVerbosity;
		ELogVerbosity::Type CompileTimeVerbosity;
		std::wstring CategoryName;
	};

	constexpr ELogVerbosity::Type InDefaultVerbosity = ELogVerbosity::Log;

	struct FLogCategory : public FLogCategoryBase
	{
		FLogCategory(const std::wstring& InCategoryName, ELogVerbosity::Type InVerbosity = InDefaultVerbosity)
			: FLogCategoryBase(InCategoryName, InVerbosity)
		{

		}
	};
}