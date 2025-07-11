#pragma once



namespace SDK
{
	template<typename ElementType>
	class TArray;

	template<typename ElementType>
	class TArray
	{
	private:

	protected:
		ElementType* Data;
		int32 ArrayNum;
		int32 ArrayMax;

	public:
		TArray()
			: Data(nullptr), ArrayNum(0), ArrayMax(0)
		{
		}

		TArray(const TArray&) = default;
		TArray(TArray&&) = default;
	public:
		TArray& operator=(TArray&&) = default;
		TArray& operator=(const TArray&) = default;

	public:
		inline void Reserve(int32_t Amount, uint64 ElementSize = sizeof(ElementType) )
		{
			Data = FMemory::Realloc(Data, (ArrayMax = Amount + ArrayNum) * ElementSize);
		}

		inline ElementType& Add(const ElementType& Element, uint64 ElementSize = sizeof(ElementType))
		{
			if (ArrayMax < (ArrayNum + 1)) {
				Reserve(1);
			}
			ElementType* Ptr = reinterpret_cast<ElementType*>(Data + (ArrayNum * ElementSize));
			new (Ptr) ElementType(Element);

			return *(Ptr + (ArrayNum++));
		}

		inline bool Remove(int32 Index, uint64 ElementSize = sizeof(ElementType))
		{
			if (Index < 0 || Index >= ArrayNum)
				return false;

			ElementType* PtrToRemove = reinterpret_cast<ElementType*>(Data + Index * ElementSize);
			PtrToRemove->~ElementType();

			for (int i = Index; i < ArrayNum - 1; ++i)
			{
				uint8* Src = Data + (i + 1) * ElementSize;
				uint8* Dst = Data + i * ElementSize;
				ElementType* SrcPtr = reinterpret_cast<ElementType*>(Src);
				new (Dst) ElementType(std::move(*SrcPtr));
				SrcPtr->~ElementType();
			}

			ArrayNum--;
			return true;
		}

		inline ElementType& GetData(int32 Index, int32 ElementSize = sizeof(ElementType))
		{
			check(Index >= 0 && Index < ArrayNum);
			return *reinterpret_cast<ElementType*>(Data + Index * ElementSize);
		}

		inline const ElementType& GetData(int32 Index, int32 ElementSize = sizeof(ElementType)) const
		{
			check(Index >= 0 && Index < ArrayNum);
			return *reinterpret_cast<const ElementType*>(Data + Index * ElementSize);
		}

		inline int32 Num() const { return ArrayNum; }
	};

	class FString : public TArray<wchar_t>
	{
	public:
		FString(const wchar_t* Str)
		{
			const uint32 NullTerminatedLength = static_cast<uint32>(wcslen(Str) + 0x1);

			Data = const_cast<wchar_t*>(Str);
			ArrayNum = NullTerminatedLength;
			ArrayMax = NullTerminatedLength;
		}
	public:
		inline const TCHAR* operator*() {
			return this->Data ? this->Data : L"";
		}
	};
}