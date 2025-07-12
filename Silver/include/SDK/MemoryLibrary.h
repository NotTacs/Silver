#pragma once


namespace SDK
{
	class MemoryLibrary
	{
	public:
		MemoryLibrary()
		: Scanner(Memcury::Scanner())
		{
		}
	protected:
		Memcury::Scanner Scanner;
	public:
		void FindPattern(const std::string& Pattern);
		void FindStringRef(const std::wstring& StringRef);
		template <typename T>
		T GetAs(uint32_t Offset = 0, bool bIsAbsolute = false) {
			if (Offset && bIsAbsolute) {
				Scanner.AbsoluteOffset(Offset);
			}
			else if (Offset)
				Scanner.RelativeOffset(Offset);

			return *Scanner.GetAs<T*>();
		}
		uintptr_t Get(uint32_t Offset = 0, bool bIsAbsolute = false) {
			if (Offset && bIsAbsolute) {
				Scanner.AbsoluteOffset(Offset);
			}
			else if (Offset)
				Scanner.RelativeOffset(Offset);

			return Scanner.Get();
		}

		FORCEINLINE bool IsValid()
		{
			return Scanner.Get() != 0;
		}

		FORCEINLINE void ScanFor(std::vector<uint8_t> Bytes, bool bForward = true, int ToSkip = 0)
		{
			Scanner.ScanFor(Bytes, bForward, ToSkip);
		}

		FORCEINLINE void SetAddress(uintptr_t Addr)
		{
			Scanner = Memcury::Scanner(Addr);
		}
	};
}