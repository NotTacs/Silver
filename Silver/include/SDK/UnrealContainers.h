#pragma once



namespace SDK
{
	template<typename ElementType>
	class TArray;

	template<bool Predicate, typename TrueClass, typename FalseClass>
	class TChooseClass;

	template<typename TrueClass, typename FalseClass>
	class TChooseClass<true, TrueClass, FalseClass>
	{
	public:
		typedef TrueClass Result;
	};

	template<typename TrueClass, typename FalseClass>
	class TChooseClass<false, TrueClass, FalseClass>
	{
	public:
		typedef FalseClass Result;
	};

	template <int IndexSize>
	struct TBitsToSizeType
	{
		static_assert(IndexSize, "Unsupported allocator index size.");
	};

	template <> struct TBitsToSizeType<8> { using Type = int8; };
	template <> struct TBitsToSizeType<16> { using Type = int16; };
	template <> struct TBitsToSizeType<32> { using Type = int32; };
	template <> struct TBitsToSizeType<64> { using Type = int64; };

	template<typename A, typename B>
	struct TAreTypesEqual;

	template<typename, typename>
	struct TAreTypesEqual
	{
		enum { Value = false };
	};

	template<typename A>
	struct TAreTypesEqual<A, A>
	{
		enum { Value = true };
	};

#define ARE_TYPES_EQUAL(A,B) TAreTypesEqual<A,B>::Value

#define RESTRICT __restrict

	typedef char				ANSICHAR;
	typedef wchar_t				WIDECHAR;
	enum UTF8CHAR : unsigned char {};
	typedef uint16				UCS2CHAR;
	typedef uint16 UTF16CHAR;
	typedef uint32 UTF32CHAR;

	template <typename T> struct TRemoveCV { typedef T Type; };
	template <typename T> struct TRemoveCV<const T> { typedef T Type; };
	template <typename T> struct TRemoveCV<volatile T> { typedef T Type; };
	template <typename T> struct TRemoveCV<const volatile T> { typedef T Type; };

	template<typename T> struct TIsReferenceType { enum { Value = false }; };
	template<typename T> struct TIsReferenceType<T&> { enum { Value = true }; };
	template<typename T> struct TIsReferenceType<T&&> { enum { Value = true }; };

	template <typename T, typename Arg>
	struct TIsBitwiseConstructible
	{
		static_assert(
			!TIsReferenceType<T  >::Value &&
			!TIsReferenceType<Arg>::Value,
			"TIsBitwiseConstructible is not designed to accept reference types");

		static_assert(
			TAreTypesEqual<T, typename TRemoveCV<T  >::Type>::Value&&
			TAreTypesEqual<Arg, typename TRemoveCV<Arg>::Type>::Value,
			"TIsBitwiseConstructible is not designed to accept qualified types");

		// Assume no bitwise construction in general
		enum { Value = false };
	};


	template <typename SizeType>
	FORCEINLINE SizeType DefaultCalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T BytesPerElement, bool bAllowQuantize,
		uint32 Alignment = 0)
	{
		const SIZE_T FirstGrow = 4;
		const SIZE_T ConstantGrow = 16;
		SizeType Retval;
		checkSlow(NumElements > NumAllocatedElements && NumElements > 0);

		SIZE_T Grow = FirstGrow;

		if (NumAllocatedElements)
		{
			// Allocate slack for the array proportional to its size.
			Grow = SIZE_T(NumElements) + 3 * SIZE_T(NumElements) / 8 + ConstantGrow;
		}
		else if (SIZE_T(NumElements) > Grow)
		{
			Grow = SIZE_T(NumElements);
		}

		if (bAllowQuantize)
		{
			Retval = (SizeType)(FMemory::QuantizeSize(Grow * BytesPerElement, Alignment) / BytesPerElement);
		}
		else
		{
			Retval = (SizeType)Grow;
		}
		// NumElements and MaxElements are stored in 32 bit signed integers so we must be careful not to overflow here.
		if (NumElements > Retval)
		{
			Retval = TNumericLimits<SizeType>::Max();
		}

		return Retval;
	}

	template <typename SizeType>
	FORCEINLINE SizeType DefaultCalculateSlackReserve(SizeType NumElements, SIZE_T BytesPerElement, bool bAllowQuantize, uint32 Alignment = 0)
	{
		SizeType Retval = NumElements;
		checkSlow(NumElements > 0);
		if (bAllowQuantize)
		{
			Retval = (SizeType)(FMemory::QuantizeSize(SIZE_T(Retval) * SIZE_T(BytesPerElement), Alignment) / BytesPerElement);
			if (NumElements > Retval)
			{
				Retval = TNumericLimits<SizeType>::Max();
			}
		}

		return Retval;
	}

	template <typename SizeType>
	FORCEINLINE SizeType DefaultCalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T BytesPerElement, bool bAllowQuantize, uint32 Alignment = 8)
	{
		SizeType Retval;
		checkSlow(NumElements < NumAllocatedElements);

		// If the container has too much slack, shrink it to exactly fit the number of elements.
		const SizeType CurrentSlackElements = NumAllocatedElements - NumElements;
		const SIZE_T CurrentSlackBytes = (NumAllocatedElements - NumElements) * BytesPerElement;
		const bool bTooManySlackBytes = CurrentSlackBytes >= 16384;
		const bool bTooManySlackElements = 3 * NumElements < 2 * NumAllocatedElements;
		if ((bTooManySlackBytes || bTooManySlackElements) && (CurrentSlackElements > 64 || !NumElements)) //  hard coded 64 :-(
		{
			Retval = NumElements;
			if (Retval > 0)
			{
				if (bAllowQuantize)
				{
					Retval = (SizeType)(FMemory::QuantizeSize(Retval * BytesPerElement, Alignment) / BytesPerElement);
				}
			}
		}
		else
		{
			Retval = NumAllocatedElements;
		}

		return Retval;
	}

	/** A type which is used to represent a script type that is unknown at compile time. */
	struct FScriptContainerElement
	{
	};

	template<int IndexSize>
	class TSizedHeapAllocator
	{
	public:
		using SizeType = typename TBitsToSizeType<IndexSize>::Type;

		enum { NeedsElementType = true };
		enum { RequireRangeCheck = true };

		class ForAnyElementType
		{
			template <int>
			friend class TSizedHeapAllocator;
			template<typename ElementType>
			friend class TArray;

		public:
			/** Default constructor. */
			ForAnyElementType()
				: Data(nullptr)
			{
			}

			template <typename OtherAllocator>
			FORCEINLINE void MoveToEmptyFromOtherAllocator(typename OtherAllocator::ForAnyElementType& Other)
			{
				checkSlow((void*)this != (void*)&Other);

				if (Data)
				{
					FMemory::Free(Data);
				}

				Data = Other.Data;
				Other.Data = nullptr;
			}

			FORCEINLINE void MoveToEmpty(ForAnyElementType& Other)
			{
				this->MoveToEmptyFromOtherAllocator<TSizedHeapAllocator>(Other);
			}

			FORCEINLINE ~ForAnyElementType()
			{
				if (Data)
				{
					FMemory::Free(Data);
				}
			}

			FORCEINLINE FScriptContainerElement* GetAllocation() const
			{
				return Data;
			}

			FORCEINLINE void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements, SIZE_T NumBytesPerElement)
			{
				// Avoid calling FMemory::Realloc( nullptr, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
				if (Data || NumElements)
				{
					//checkSlow(((uint64)NumElements*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
					Data = (FScriptContainerElement*)FMemory::Realloc(Data, NumElements * NumBytesPerElement);
				}
			}
			FORCEINLINE void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement)
			{
				// Avoid calling FMemory::Realloc( nullptr, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
				if (Data || NumElements)
				{
					//checkSlow(((uint64)NumElements*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
					Data = (FScriptContainerElement*)FMemory::Realloc(Data, NumElements * NumBytesPerElement, AlignmentOfElement);
				}
			}

			FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement) const
			{
				return DefaultCalculateSlackReserve(NumElements, NumBytesPerElement, true);
			}
			FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement) const
			{
				return DefaultCalculateSlackReserve(NumElements, NumBytesPerElement, true, (uint32)AlignmentOfElement);
			}
			FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
			{
				return DefaultCalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement, true);
			}
			FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement) const
			{
				return DefaultCalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement, true, (uint32)AlignmentOfElement);
			}
			FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
			{
				return DefaultCalculateSlackGrow(NumElements, NumAllocatedElements, NumBytesPerElement, true);
			}
			FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement) const
			{
				return DefaultCalculateSlackGrow(NumElements, NumAllocatedElements, NumBytesPerElement, true, (uint32)AlignmentOfElement);
			}

			SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
			{
				return NumAllocatedElements * NumBytesPerElement;
			}

			bool HasAllocation() const
			{
				return !!Data;
			}

			SizeType GetInitialCapacity() const
			{
				return 0;
			}

		private:
			ForAnyElementType(const ForAnyElementType&);
			ForAnyElementType& operator=(const ForAnyElementType&);

			/** A pointer to the container's elements. */
			FScriptContainerElement* Data;
		};

		template<typename ElementType>
		class ForElementType : public ForAnyElementType
		{
		public:
			/** Default constructor. */
			ForElementType()
			{
			}

			FORCEINLINE ElementType* GetAllocation() const
			{
				return (ElementType*)ForAnyElementType::GetAllocation();
			}
		};
	};

	template <typename T>
	struct TIsAbstract
	{
		enum { Value = __is_abstract(T) };
	};

	template <bool Predicate, typename Result = void>
	class TEnableIf;

	template <typename Result>
	class TEnableIf<true, Result>
	{
	public:
		using type = Result;
		using Type = Result;
	};

	template <typename Result>
	class TEnableIf<false, Result>
	{
	};

	enum { INDEX_NONE = -1 };
	enum { UNICODE_BOM = 0xfeff };

	template <typename AllocatorType>
	struct TAllocatorTraitsBase
	{
		enum { SupportsMove = false };
		enum { IsZeroConstruct = false };
		enum { SupportsFreezeMemoryImage = false };
		enum { SupportsElementAlignment = false };
	};

	template <typename AllocatorType>
	struct TAllocatorTraits : TAllocatorTraitsBase<AllocatorType>
	{
	};


	template <typename FromAllocatorType, typename ToAllocatorType>
	struct TCanMoveBetweenAllocators
	{
		enum { Value = false };
	};

	template<typename T>
	struct TIsZeroConstructType
	{
		enum { Value = TOr<TIsEnum<T>, TIsArithmetic<T>, TIsPointer<T>>::Value };
	};

	template <typename... Types>
	struct TAnd;

	template <bool LHSValue, typename... RHS>
	struct TAndValue
	{
		static constexpr bool Value = TAnd<RHS...>::value;
		static constexpr bool value = TAnd<RHS...>::value;
	};

	template <typename... RHS>
	struct TAndValue<false, RHS...>
	{
		static constexpr bool Value = false;
		static constexpr bool value = false;
	};

	template <typename LHS, typename... RHS>
	struct TAnd<LHS, RHS...> : TAndValue<LHS::Value, RHS...>
	{
	};

	template <>
	struct TAnd<>
	{
		static constexpr bool Value = true;
		static constexpr bool value = true;
	};

	/**
	 * Does a boolean OR of the ::Value static members of each type, but short-circuits if any Type::Value == true.
	 */
	template <typename... Types>
	struct TOr;

	template <bool LHSValue, typename... RHS>
	struct TOrValue
	{
		static constexpr bool Value = TOr<RHS...>::value;
		static constexpr bool value = TOr<RHS...>::value;
	};

	template <typename... RHS>
	struct TOrValue<true, RHS...>
	{
		static constexpr bool Value = true;
		static constexpr bool value = true;
	};

	template <typename LHS, typename... RHS>
	struct TOr<LHS, RHS...> : TOrValue<LHS::Value, RHS...>
	{
	};

	template <>
	struct TOr<>
	{
		static constexpr bool Value = false;
		static constexpr bool value = false;
	};

	/**
	 * Does a boolean NOT of the ::Value static members of the type.
	 */
	template <typename Type>
	struct TNot
	{
		static constexpr bool Value = !Type::Value;
		static constexpr bool value = !Type::Value;
	};

	template <typename T>
	struct TIsPODType
	{
		enum { Value = __is_pod(T) };
	};

	template <typename T>
	struct TIsArithmetic
	{
		enum { Value = false };
	};

	template <> struct TIsArithmetic<float> { enum { Value = true }; };
	template <> struct TIsArithmetic<double> { enum { Value = true }; };
	template <> struct TIsArithmetic<long double> { enum { Value = true }; };
	template <> struct TIsArithmetic<uint8> { enum { Value = true }; };
	template <> struct TIsArithmetic<uint16> { enum { Value = true }; };
	template <> struct TIsArithmetic<uint32> { enum { Value = true }; };
	template <> struct TIsArithmetic<uint64> { enum { Value = true }; };
	template <> struct TIsArithmetic<int8> { enum { Value = true }; };
	template <> struct TIsArithmetic<int16> { enum { Value = true }; };
	template <> struct TIsArithmetic<int32> { enum { Value = true }; };
	template <> struct TIsArithmetic<int64> { enum { Value = true }; };
	template <> struct TIsArithmetic<long> { enum { Value = true }; };
	template <> struct TIsArithmetic<unsigned long> { enum { Value = true }; };
	template <> struct TIsArithmetic<bool> { enum { Value = true }; };
	template <> struct TIsArithmetic<WIDECHAR> { enum { Value = true }; };
	template <> struct TIsArithmetic<ANSICHAR> { enum { Value = true }; };

	template <typename T> struct TIsArithmetic<const          T> { enum { Value = TIsArithmetic<T>::Value }; };
	template <typename T> struct TIsArithmetic<      volatile T> { enum { Value = TIsArithmetic<T>::Value }; };
	template <typename T> struct TIsArithmetic<const volatile T> { enum { Value = TIsArithmetic<T>::Value }; };


	template <typename T, bool TypeIsSmall>
	struct TCallTraitsParamTypeHelper
	{
		typedef const T& ParamType;
		typedef const T& ConstParamType;
	};
	template <typename T>
	struct TCallTraitsParamTypeHelper<T, true>
	{
		typedef const T ParamType;
		typedef const T ConstParamType;
	};
	template <typename T>
	struct TCallTraitsParamTypeHelper<T*, true>
	{
		typedef T* ParamType;
		typedef const T* ConstParamType;
	};

	template <typename T>
	struct TCallTraitsBase
	{
	private:
		enum { PassByValue = TOr<TAndValue<(sizeof(T) <= sizeof(void*)), TIsPODType<T>>, TIsArithmetic<T>>::Value };

	public:
		typedef T ValueType;
		typedef T& Reference;
		typedef const T& ConstReference;
		typedef typename TCallTraitsParamTypeHelper<T, PassByValue>::ParamType ParamType;
		typedef typename TCallTraitsParamTypeHelper<T, PassByValue>::ConstParamType ConstPointerType;
	};

	/**
	 * TCallTraits
	 */
	template <typename T>
	struct TCallTraits : public TCallTraitsBase<T> {};

	template<typename T>
	struct TTypeTraitsBase
	{
		typedef typename TCallTraits<T>::ParamType ConstInitType;
		typedef typename TCallTraits<T>::ConstPointerType ConstPointerType;

		// There's no good way of detecting this so we'll just assume it to be true for certain known types and expect
		// users to customize it for their custom types.
		enum { IsBytewiseComparable = TOr<TIsEnum<T>, TIsArithmetic<T>, TIsPointer<T>>::Value };
	};

	/**
	 * Traits for types.
	 */
	template<typename T> struct TTypeTraits : public TTypeTraitsBase<T> {};

	using FHeapAllocator = TSizedHeapAllocator<32>;

	template <int IndexSize> class TSizedDefaultAllocator : public TSizedHeapAllocator<IndexSize> { public: typedef TSizedHeapAllocator<IndexSize> Typedef; };

	using FDefaultAllocator = TSizedDefaultAllocator<32>;

	template<typename InElementType>
	class TArray
	{
		template<typename OtherElementType>
		friend class TArray;
	public:
		typedef typename FDefaultAllocator::SizeType SizeType;
		typedef InElementType ElementType;
		typedef FDefaultAllocator AllocatorType;
		typedef typename TChooseClass<
			AllocatorType::NeedsElementType,
			typename AllocatorType::template ForElementType<ElementType>,
			typename AllocatorType::ForAnyElementType
		>::Result ElementAllocatorType;

		FORCEINLINE TArray()
			: ArrayNum(0)
			, ArrayMax(AllocatorInstance.GetInitialCapacity())
		{
		}

		TArray& operator=(std::initializer_list<InElementType> InitList)
		{
			DestructItems(GetData(), ArrayNum);
			CopyToEmpty(InitList.begin(), (SizeType)InitList.size(), ArrayMax, 0);
			return *this;
		}

		template<typename OtherAllocatorType>
		TArray& operator=(const TArray<ElementType> Other)
		{
			DestructItems(GetData(), ArrayNum);
			CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);
			return *this;
		}

		TArray& operator=(const TArray& Other)
		{
			if (this != &Other)
			{
				DestructItems(GetData(), ArrayNum);
				CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);
			}
			return *this;
		}

	public:

		TArray& operator=(TArray&& Other)
		{
			if (this != &Other)
			{
				DestructItems(GetData(), ArrayNum);
				MoveOrCopy(*this, Other, ArrayMax);
			}
			return *this;
		}

		FORCEINLINE ElementType* GetData()
		{
			return (ElementType*)AllocatorInstance.GetAllocation();
		}

		FORCEINLINE const ElementType* GetData() const
		{
			return (const ElementType*)AllocatorInstance.GetAllocation();
		}

		FORCEINLINE ElementType* GetItemPtr(SizeType Index, int32 ElementSize = sizeof(ElementType))
		{
			return reinterpret_cast<ElementType*>(reinterpret_cast<uint8_t*>(GetData()) + Index * ElementSize);
		}

		FORCEINLINE const ElementType* GetItemPtr(SizeType Index, int32 ElementSize = sizeof(ElementType)) const
		{
			return reinterpret_cast<const ElementType*>(reinterpret_cast<const uint8_t*>(GetData()) + Index * ElementSize);
		}

		FORCEINLINE uint32 GetTypeSize() const
		{
			return sizeof(ElementType);
		}

		FORCEINLINE SIZE_T GetAllocatedSize(void) const
		{
			return AllocatorInstance.GetAllocatedSize(ArrayMax, sizeof(ElementType));
		}

		FORCEINLINE SizeType GetSlack() const
		{
			return ArrayMax - ArrayNum;
		}

		FORCEINLINE void CheckInvariants() const
		{
			checkSlow((ArrayNum >= 0) & (ArrayMax >= ArrayNum)); // & for one branch
		}

		FORCEINLINE void RangeCheck(SizeType Index) const
		{
			CheckInvariants();

			// Template property, branch will be optimized out
			if (AllocatorType::RequireRangeCheck)
			{
				//checkf((Index >= 0) & (Index < ArrayNum), TEXT("Array index out of bounds: %i from an array of size %i"), Index, ArrayNum); // & for one branch
			}
		}

		FORCEINLINE bool IsValidIndex(SizeType Index) const
		{
			return Index >= 0 && Index < ArrayNum;
		}

		bool IsEmpty() const
		{
			return ArrayNum == 0;
		}

		FORCEINLINE SizeType Num() const
		{
			return ArrayNum;
		}

		FORCEINLINE SizeType Max() const
		{
			return ArrayMax;
		}

		/*Do Not Use unless the size of your class is defined*/
		FORCEINLINE ElementType& operator[](SizeType Index)
		{
			RangeCheck(Index);
			return *GetItemPtr(Index);
		}
		/*Do Not Use unless the size of your class is defined*/
		FORCEINLINE const ElementType& operator[](SizeType Index) const
		{
			RangeCheck(Index);
			return *GetItemPtr(Index);
		}

		template<typename ET = InElementType>
		FORCEINLINE typename TEnableIf<!TIsAbstract<ET>::Value, ElementType>::Type Pop(bool bAllowShrinking = true)
		{
			RangeCheck(0);
			ElementType Result = MoveTempIfPossible(GetData()[ArrayNum - 1]);
			RemoveAt(ArrayNum - 1, 1, bAllowShrinking);
			return Result;
		}

		FORCEINLINE void Push(ElementType&& Item)
		{
			Add(MoveTempIfPossible(Item));
		}

		FORCEINLINE void Push(const ElementType& Item)
		{
			Add(Item);
		}

		FORCEINLINE ElementType& Top()
		{
			return Last();
		}

		FORCEINLINE const ElementType& Top() const
		{
			return Last();
		}

		FORCEINLINE ElementType& Last(SizeType IndexFromTheEnd = 0, int32 ElementSize = sizeof(ElementType))
		{
			RangeCheck(ArrayNum - IndexFromTheEnd - 1);
			return *GetItemPtr(ArrayNum - IndexFromTheEnd - 1, ElementSize);
		}

		FORCEINLINE const ElementType& Last(SizeType IndexFromTheEnd = 0, int32 ElementSize = sizeof(ElementType)) const
		{
			RangeCheck(ArrayNum - IndexFromTheEnd - 1);
			return *GetItemPtr(ArrayNum - IndexFromTheEnd - 1, ElementSize);
		}

		FORCEINLINE void Shrink()
		{
			CheckInvariants();
			if (ArrayMax != ArrayNum)
			{
				ResizeTo(ArrayNum);
			}
		}

		FORCEINLINE bool Find(const ElementType& Item, SizeType& Index) const
		{
			Index = this->Find(Item);
			return Index != INDEX_NONE;
		}

		SizeType Find(const ElementType& Item) const
		{
			const ElementType* RESTRICT Start = GetData();
			for (const ElementType* RESTRICT Data = Start, *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
			{
				if (*Data == Item)
				{
					return static_cast<SizeType>(Data - Start);
				}
			}
			return INDEX_NONE;
		}

		FORCEINLINE bool FindLast(const ElementType& Item, SizeType& Index) const
		{
			Index = this->FindLast(Item);
			return Index != INDEX_NONE;
		}

		SizeType FindLast(const ElementType& Item) const
		{
			for (const ElementType* RESTRICT Start = GetData(), *RESTRICT Data = Start + ArrayNum; Data != Start; )
			{
				--Data;
				if (*Data == Item)
				{
					return static_cast<SizeType>(Data - Start);
				}
			}
			return INDEX_NONE;
		}

		template <typename Predicate>
		SizeType FindLastByPredicate(Predicate Pred, SizeType Count) const
		{
			check(Count >= 0 && Count <= this->Num());
			for (const ElementType* RESTRICT Start = GetData(), *RESTRICT Data = Start + Count; Data != Start; )
			{
				--Data;
				if (::Invoke(Pred, *Data))
				{
					return static_cast<SizeType>(Data - Start);
				}
			}
			return INDEX_NONE;
		}

		template <typename Predicate>
		FORCEINLINE SizeType FindLastByPredicate(Predicate Pred) const
		{
			return FindLastByPredicate(Pred, ArrayNum);
		}

		template <typename KeyType>
		SizeType IndexOfByKey(const KeyType& Key) const
		{
			const ElementType* RESTRICT Start = GetData();
			for (const ElementType* RESTRICT Data = Start, *RESTRICT DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
			{
				if (*Data == Key)
				{
					return static_cast<SizeType>(Data - Start);
				}
			}
			return INDEX_NONE;
		}

		template <typename Predicate>
		SizeType IndexOfByPredicate(Predicate Pred) const
		{
			const ElementType* RESTRICT Start = GetData();
			for (const ElementType* RESTRICT Data = Start, *RESTRICT DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
			{
				if (::Invoke(Pred, *Data))
				{
					return static_cast<SizeType>(Data - Start);
				}
			}
			return INDEX_NONE;
		}

		template <typename KeyType>
		FORCEINLINE const ElementType* FindByKey(const KeyType& Key) const
		{
			return const_cast<TArray*>(this)->FindByKey(Key);
		}

		template <typename KeyType>
		ElementType* FindByKey(const KeyType& Key)
		{
			for (ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
			{
				if (*Data == Key)
				{
					return Data;
				}
			}

			return nullptr;
		}

		template <typename Predicate>
		FORCEINLINE const ElementType* FindByPredicate(Predicate Pred) const
		{
			return const_cast<TArray*>(this)->FindByPredicate(Pred);
		}

		template <typename Predicate>
		ElementType* FindByPredicate(Predicate Pred)
		{
			for (ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
			{
				if (::Invoke(Pred, *Data))
				{
					return Data;
				}
			}

			return nullptr;
		}

		template <typename Predicate>
		TArray<ElementType> FilterByPredicate(Predicate Pred) const
		{
			TArray<ElementType> FilterResults;
			for (const ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
			{
				if (::Invoke(Pred, *Data))
				{
					FilterResults.Add(*Data);
				}
			}
			return FilterResults;
		}

		template <typename ComparisonType>
		bool Contains(const ComparisonType& Item) const
		{
			for (const ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
			{
				if (*Data == Item)
				{
					return true;
				}
			}
			return false;
		}

		template <typename Predicate>
		FORCEINLINE bool ContainsByPredicate(Predicate Pred) const
		{
			return FindByPredicate(Pred) != nullptr;
		}

		bool operator==(const TArray& OtherArray) const
		{
			SizeType Count = Num();

			return Count == OtherArray.Num() && CompareItems(GetData(), OtherArray.GetData(), Count);
		}

		FORCEINLINE bool operator!=(const TArray& OtherArray) const
		{
			return !(*this == OtherArray);
		}

		FORCEINLINE SizeType AddUninitialized(SizeType Count = 1, int32_t ElementSize = sizeof(ElementType))
		{
			CheckInvariants();
			checkSlow(Count >= 0);

			const SizeType OldNum = ArrayNum;
			if ((ArrayNum += Count) > ArrayMax)
			{
				ResizeGrow(OldNum, ElementSize);
			}
			return OldNum;
		}

		FORCEINLINE void CheckAddress(const ElementType* Addr) const
		{
			//checkf(Addr < GetData() || Addr >= (GetData() + ArrayMax), TEXT("Attempting to use a container element (%p) which already comes from the container being modified (%p, ArrayMax: %d, ArrayNum: %d, SizeofElement: %d)!"), Addr, GetData(), ArrayMax, ArrayNum, sizeof(ElementType));
		}

	private:
		void RemoveAtImpl(SizeType Index, SizeType Count, bool bAllowShrinking, int32 ElementSize = sizeof(ElementType))
		{
			if (Count)
			{
				CheckInvariants();
				checkSlow((Count >= 0) & (Index >= 0) & (Index + Count <= ArrayNum));

				DestructItems(GetItemPtr(Index), Count);

				// Skip memmove in the common case that there is nothing to move.
				SizeType NumToMove = ArrayNum - Index - Count;
				if (NumToMove)
				{
					FMemory::Memmove
					(
						(uint8*)AllocatorInstance.GetAllocation() + (Index)*ElementSize,
						(uint8*)AllocatorInstance.GetAllocation() + (Index + Count) * ElementSize,
						NumToMove * ElementSize
					);
				}
				ArrayNum -= Count;

				if (bAllowShrinking)
				{
					ResizeShrink();
				}
			}
		}

	public:
		FORCEINLINE void RemoveAt(SizeType Index, int32 ElementSize = sizeof(ElementType))
		{
			RemoveAtImpl(Index, 1, true, ElementSize);
		}

		template <typename CountType>
		FORCEINLINE void RemoveAt(SizeType Index, CountType Count, bool bAllowShrinking = true)
		{
			static_assert(!TAreTypesEqual<CountType, bool>::Value, "TArray::RemoveAt: unexpected bool passed as the Count argument");
			RemoveAtImpl(Index, (SizeType)Count, bAllowShrinking);
		}

		void Reset(SizeType NewSize = 0)
		{
			// If we have space to hold the excepted size, then don't reallocate
			if (NewSize <= ArrayMax)
			{
				DestructItems(GetData(), ArrayNum);
				ArrayNum = 0;
			}
			else
			{
				Empty(NewSize);
			}
		}

		void Empty(SizeType Slack = 0)
		{
			DestructItems(GetData(), ArrayNum);

			checkSlow(Slack >= 0);
			ArrayNum = 0;

			if (ArrayMax != Slack)
			{
				ResizeTo(Slack);
			}
		}

		void SetNum(SizeType NewNum, SizeType ElementSize = sizeof(ElementType), bool bAllowShrinking = true)
		{
			if (NewNum > Num())
			{
				const SizeType Diff = NewNum - ArrayNum;
				const SizeType Index = AddUninitialized(Diff);
				DefaultConstructItems<ElementType>((uint8*)AllocatorInstance.GetAllocation() + Index * ElementSize, Diff);
			}
			else if (NewNum < Num())
			{
				RemoveAt(NewNum, Num() - NewNum, bAllowShrinking);
			}
		}

		template <typename ElementType, typename SizeType>
		FORCEINLINE bool CompareItems(const ElementType* A, const ElementType* B, SizeType Count)
		{
			if constexpr (TTypeTraits<ElementType>::IsBytewiseComparable)
			{
				return !FMemory::Memcmp(A, B, sizeof(ElementType) * Count);
			}
			else
			{
				while (Count)
				{
					if (!(*A == *B))
					{
						return false;
					}

					++A;
					++B;
					--Count;
				}

				return true;
			}
		}

		template <typename ElementType, typename SizeType>
		FORCEINLINE void DefaultConstructItems(void* Address, SizeType Count)
		{
			if constexpr (TIsZeroConstructType<ElementType>::Value)
			{
				FMemory::Memset(Address, 0, sizeof(ElementType) * Count);
			}
			else
			{
				ElementType* Element = (ElementType*)Address;
				while (Count)
				{
					new (Element) ElementType;
					++Element;
					--Count;
				}
			}
		}

		template <typename... ArgsType>
		FORCEINLINE SizeType Emplace(ArgsType&&... Args, SizeType ElementSize = sizeof(ElementType))
		{
			const SizeType Index = AddUninitialized(1, ElementSize);
			new (GetItemPtr(Index)) ElementType(Forward<ArgsType>(Args)...);
			return Index;
		}

		FORCEINLINE SizeType Emplace(const ElementType& Item, SizeType ElementSize = sizeof(ElementType)) const
		{
			const SizeType Index = AddUninitialized(1, ElementSize);
			GetItemPtr(Index) = Item;
			return Index;
		}

		template <typename... ArgsType>
		FORCEINLINE ElementType& Emplace_GetRef(ArgsType&&... Args, SizeType ElementSize = sizeof(ElementType))
		{
			const SizeType Index = AddUninitialized(1, ElementSize);
			ElementType* Ptr = GetItemPtr(Index);
			new (Ptr) ElementType(Forward<ArgsType>(Args)...);
			return *Ptr;
		}

		FORCEINLINE SizeType Add(ElementType&& Item, SizeType ElementSize = sizeof(ElementType))
		{
			CheckAddress(&Item);
			return Emplace(MoveTempIfPossible(Item), ElementSize);
		}

		FORCEINLINE SizeType Add(const ElementType& Item, int32_t ElementSize = sizeof(ElementType))
		{
			CheckAddress(&Item);
			return Emplace(Item, ElementSize);
		}

		SizeType AddZeroed(SizeType Count = 1, SizeType ElementSize = sizeof(ElementType))
		{
			const SizeType Index = AddUninitialized(Count, ElementSize);
			FMemory::Memzero((uint8*)AllocatorInstance.GetAllocation() + Index * ElementSize, Count * ElementSize);
			return Index;
		}

		template <typename DestinationElementType, typename SourceElementType, typename SizeType>
		FORCEINLINE void ConstructItems(void* Dest, const SourceElementType* Source, SizeType Count)
		{
			if constexpr (TIsBitwiseConstructible<DestinationElementType, SourceElementType>::Value)
			{
				FMemory::Memcpy(Dest, Source, sizeof(SourceElementType) * Count);
			}
			else
			{
				while (Count)
				{
					new (Dest) DestinationElementType(*Source);
					++(DestinationElementType*&)Dest;
					++Source;
					--Count;
				}
			}
		}

		void AllocatorResizeAllocation(SizeType CurrentArrayNum, SizeType NewArrayMax, int32_t ElementSize = sizeof(ElementType))
		{
			if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
			{
				AllocatorInstance.ResizeAllocation(CurrentArrayNum, NewArrayMax, ElementSize, alignof(ElementType));
			}
			else
			{
				AllocatorInstance.ResizeAllocation(CurrentArrayNum, NewArrayMax, ElementSize);
			}
		}

		SizeType AllocatorCalculateSlackShrink(SizeType CurrentArrayNum, SizeType NewArrayMax, int32_t ElementSize = sizeof(ElementType))
		{
			if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
			{
				return AllocatorInstance.CalculateSlackShrink(CurrentArrayNum, NewArrayMax, ElementSize, alignof(ElementType));
			}
			else
			{
				return AllocatorInstance.CalculateSlackShrink(CurrentArrayNum, NewArrayMax, ElementSize);
			}
		}

		SizeType AllocatorCalculateSlackGrow(SizeType CurrentArrayNum, SizeType NewArrayMax, int32_t ElementSize = sizeof(ElementType))
		{
			if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
			{
				return AllocatorInstance.CalculateSlackGrow(CurrentArrayNum, NewArrayMax, ElementSize, alignof(ElementType));
			}
			else
			{
				return AllocatorInstance.CalculateSlackGrow(CurrentArrayNum, NewArrayMax, ElementSize);
			}
		}

		SizeType AllocatorCalculateSlackReserve(SizeType NewArrayMax, int32_t ElementSize = sizeof(ElementType))
		{
			if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
			{
				return AllocatorInstance.CalculateSlackReserve(NewArrayMax, ElementSize, alignof(ElementType));
			}
			else
			{
				return AllocatorInstance.CalculateSlackReserve(NewArrayMax, ElementSize);
			}
		}

		FORCEINLINE void ResizeGrow(SizeType OldNum, int32_t ElementSize = sizeof(ElementType))
		{
			ArrayMax = AllocatorCalculateSlackGrow(ArrayNum, ArrayMax, ElementSize);
			AllocatorResizeAllocation(OldNum, ArrayMax, ElementSize);
		}
		FORCEINLINE void ResizeShrink()
		{
			const SizeType NewArrayMax = AllocatorCalculateSlackShrink(ArrayNum, ArrayMax);
			if (NewArrayMax != ArrayMax)
			{
				ArrayMax = NewArrayMax;
				//check(ArrayMax >= ArrayNum);
				AllocatorResizeAllocation(ArrayNum, ArrayMax);
			}
		}
		FORCEINLINE void ResizeTo(SizeType NewMax)
		{
			if (NewMax)
			{
				NewMax = AllocatorCalculateSlackReserve(NewMax);
			}
			if (NewMax != ArrayMax)
			{
				ArrayMax = NewMax;
				AllocatorResizeAllocation(ArrayNum, ArrayMax);
			}
		}
		FORCEINLINE void ResizeForCopy(SizeType NewMax, SizeType PrevMax)
		{
			if (NewMax)
			{
				NewMax = AllocatorCalculateSlackReserve(NewMax);
			}
			if (NewMax > PrevMax)
			{
				AllocatorResizeAllocation(0, NewMax);
				ArrayMax = NewMax;
			}
			else
			{
				ArrayMax = PrevMax;
			}
		}

		template <typename T, bool bIsTriviallyTriviallyDestructible = __is_enum(T)>
		struct TImpl
		{
			enum { Value = true };
		};

		template <typename T>
		struct TImpl<T, false>
		{
			enum { Value = __has_trivial_destructor(T) };
		};

		template <typename T>
		struct TIsTriviallyDestructible
		{
			enum { Value = TImpl<T>::Value };
		};

		template <typename ElementType, typename SizeType>
		FORCEINLINE void DestructItems(ElementType* Element, SizeType Count)
		{
			if constexpr (!TIsTriviallyDestructible<ElementType>::Value)
			{
				while (Count)
				{
					// We need a typedef here because VC won't compile the destructor call below if ElementType itself has a member called ElementType
					typedef ElementType DestructItemsElementTypeTypedef;

					Element->DestructItemsElementTypeTypedef::~DestructItemsElementTypeTypedef();
					++Element;
					--Count;
				}
			}
		}

		template <typename OtherElementType, typename OtherSizeType>
		void CopyToEmpty(const OtherElementType* OtherData, OtherSizeType OtherNum, SizeType PrevMax, SizeType ExtraSlack)
		{
			check(ArrayNum == 0);
			check(AllocatorInstance.GetAllocation() == nullptr || ArrayMax == 0);
			ArrayMax = OtherNum + ExtraSlack;
			ArrayMax = AllocatorCalculateSlackReserve(ArrayMax);
			AllocatorResizeAllocation(0, ArrayMax);
			if (OtherNum > 0)
			{
				uint8* Dest = reinterpret_cast<uint8*>(AllocatorInstance.GetAllocation());
				const uint8* Src = reinterpret_cast<const uint8*>(OtherData);

				for (SizeType Index = 0; Index < OtherNum; ++Index)
				{
					new (Dest + Index * sizeof(OtherElementType)) OtherElementType(*reinterpret_cast<const OtherElementType*>(Src + Index * sizeof(OtherElementType)));
				}
			}

			ArrayNum = OtherNum;
		}

		FORCEINLINE void Reserve(SizeType Number)
		{
			checkSlow(Number >= 0);
			if (Number > ArrayMax)
			{
				ResizeTo(Number);
			}
		}

	protected:
		ElementAllocatorType AllocatorInstance;
		SizeType             ArrayNum;
		SizeType             ArrayMax;
	};




	template<typename T> struct TContainerTraitsBase
	{
		// This should be overridden by every container that supports emptying its contents via a move operation.
		enum { MoveWillEmptyContainer = false };
	};

	template<typename T> struct TContainerTraits : public TContainerTraitsBase<T> {};

	template <typename T> struct TRemoveReference { typedef T Type; };
	template <typename T> struct TRemoveReference<T& > { typedef T Type; };
	template <typename T> struct TRemoveReference<T&&> { typedef T Type; };

	template<typename T> struct TIsLValueReferenceType { enum { Value = false }; };
	template<typename T> struct TIsLValueReferenceType<T&> { enum { Value = true }; };

	template <typename T>
	FORCEINLINE typename TRemoveReference<T>::Type&& MoveTemp(T&& Obj)
	{
		typedef typename TRemoveReference<T>::Type CastType;

		// Validate that we're not being passed an rvalue or a const object - the former is redundant, the latter is almost certainly a mistake
		static_assert(TIsLValueReferenceType<T>::Value, "MoveTemp called on an rvalue");
		static_assert(!TAreTypesEqual<CastType&, const CastType&>::Value, "MoveTemp called on a const object");

		return (CastType&&)Obj;
	}



	template <typename SrcEncoding, typename DestEncoding>
	static constexpr bool IsCharEncodingCompatibleWith()
	{
		if constexpr (std::is_same_v<SrcEncoding, DestEncoding>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, ANSICHAR> && std::is_same_v<DestEncoding, UTF8CHAR>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, UCS2CHAR> && std::is_same_v<DestEncoding, UTF16CHAR>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, WIDECHAR> && std::is_same_v<DestEncoding, UCS2CHAR>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, UCS2CHAR> && std::is_same_v<DestEncoding, WIDECHAR>)
		{
			return true;
		}
#if PLATFORM_TCHAR_IS_CHAR16
		else if constexpr (std::is_same_v<SrcEncoding, WIDECHAR> && std::is_same_v<DestEncoding, wchar_t>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, wchar_t> && std::is_same_v<DestEncoding, WIDECHAR>)
		{
			return true;
		}
#endif
		else
		{
			return false;
		}
	};

	template<typename T> struct TIsCharType { enum { Value = false }; };
	template<>           struct TIsCharType<ANSICHAR> { enum { Value = true }; };
	template<>           struct TIsCharType<UCS2CHAR> { enum { Value = true }; };
	template<>           struct TIsCharType<WIDECHAR> { enum { Value = true }; };
	template<>           struct TIsCharType<UTF8CHAR> { enum { Value = true }; };
	template<>           struct TIsCharType<UTF32CHAR> { enum { Value = true }; };
#if PLATFORM_TCHAR_IS_CHAR16
	template<>           struct TIsCharType<wchar_t> { enum { Value = true }; };
#endif

	template <typename Encoding>
	static constexpr bool IsFixedWidthEncoding()
	{
		static_assert(TIsCharType<Encoding>::Value, "Encoding is not a char type");

		return
			std::is_same_v<Encoding, ANSICHAR> ||
			std::is_same_v<Encoding, UCS2CHAR> || // this may not be true when PLATFORM_UCS2CHAR_IS_UTF16CHAR == 1, but this is the legacy behavior
			std::is_same_v<Encoding, WIDECHAR> || // the UCS2CHAR comment also applies to WIDECHAR
#if PLATFORM_TCHAR_IS_CHAR16
			std::is_same_v<Encoding, wchar_t> || // the UCS2CHAR comment also applies to wchar_t
#endif
			std::is_same_v<Encoding, UTF32CHAR>;
	}

	FORCEINLINE bool IsValidCodepoint(const uint32 Codepoint)
	{
		if ((Codepoint > 0x10FFFF) ||						// No Unicode codepoints above 10FFFFh, (for now!)
			(Codepoint == 0xFFFE) || (Codepoint == 0xFFFF)) // illegal values.
		{
			return false;
		}
		return true;
	}

#define UNICODE_BOGUS_CHAR_CODEPOINT '?'
#define HIGH_SURROGATE_START_CODEPOINT    ((uint16)0xD800)
#define HIGH_SURROGATE_END_CODEPOINT      ((uint16)0xDBFF)
#define LOW_SURROGATE_START_CODEPOINT     ((uint16)0xDC00)
#define LOW_SURROGATE_END_CODEPOINT       ((uint16)0xDFFF)
#define ENCODED_SURROGATE_START_CODEPOINT ((uint32)0x10000)
#define ENCODED_SURROGATE_END_CODEPOINT   ((uint32)0x10FFFF)
#define UE_PTRDIFF_TO_INT32(argument) static_cast<int32>(argument)
#define UE_PTRDIFF_TO_UINT32(argument) static_cast<uint32>(argument)


	static FORCEINLINE bool IsHighSurrogate(const uint32 Codepoint)
	{
		return Codepoint >= HIGH_SURROGATE_START_CODEPOINT && Codepoint <= HIGH_SURROGATE_END_CODEPOINT;
	}

	static FORCEINLINE bool IsLowSurrogate(const uint32 Codepoint)
	{
		return Codepoint >= LOW_SURROGATE_START_CODEPOINT && Codepoint <= LOW_SURROGATE_END_CODEPOINT;
	}

	static FORCEINLINE uint32 EncodeSurrogate(const uint16 HighSurrogate, const uint16 LowSurrogate)
	{
		return ((HighSurrogate - HIGH_SURROGATE_START_CODEPOINT) << 10) + (LowSurrogate - LOW_SURROGATE_START_CODEPOINT) + 0x10000;
	}

	static FORCEINLINE void DecodeSurrogate(const uint32 Codepoint, uint16& OutHighSurrogate, uint16& OutLowSurrogate)
	{
		const uint32 TmpCodepoint = Codepoint - 0x10000;
		OutHighSurrogate = (uint16)((TmpCodepoint >> 10) + HIGH_SURROGATE_START_CODEPOINT);
		OutLowSurrogate = (TmpCodepoint & 0x3FF) + LOW_SURROGATE_START_CODEPOINT;
	}

	/** Is the provided Codepoint outside of the range of the basic multilingual plane, but within the valid range of UTF8/16? */
	static FORCEINLINE bool IsEncodedSurrogate(const uint32 Codepoint)
	{
		return Codepoint >= ENCODED_SURROGATE_START_CODEPOINT && Codepoint <= ENCODED_SURROGATE_END_CODEPOINT;
	}

	template <typename BufferType>
	static bool WriteCodepointToBuffer(uint32 Codepoint, BufferType& OutputIterator, int32& OutputIteratorByteSizeRemaining)
	{
		// Ensure we have at least one character in size to write
		if (OutputIteratorByteSizeRemaining == 0)
		{
			return false;
		}

		if (!IsValidCodepoint(Codepoint))
		{
			Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
		}
		else if (IsHighSurrogate(Codepoint) || IsLowSurrogate(Codepoint)) // UTF-8 Characters are not allowed to encode codepoints in the surrogate pair range
		{
			Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		// Do the encoding...
		if (Codepoint < 0x80)
		{
			*OutputIterator++ = (UTF8CHAR)Codepoint;

			OutputIteratorByteSizeRemaining -= 1;
		}
		else if (Codepoint < 0x800)
		{
			if (OutputIteratorByteSizeRemaining < 2)
			{
				return false;
			}

			*OutputIterator++ = (UTF8CHAR)((Codepoint >> 6) | 128 | 64);
			*OutputIterator++ = (UTF8CHAR)((Codepoint & 0x3F) | 128);

			OutputIteratorByteSizeRemaining -= 2;
		}
		else if (Codepoint < 0x10000)
		{
			if (OutputIteratorByteSizeRemaining < 3)
			{
				return false;
			}

			*OutputIterator++ = (UTF8CHAR)((Codepoint >> 12) | 128 | 64 | 32);
			*OutputIterator++ = (UTF8CHAR)(((Codepoint >> 6) & 0x3F) | 128);
			*OutputIterator++ = (UTF8CHAR)((Codepoint & 0x3F) | 128);

			OutputIteratorByteSizeRemaining -= 3;
		}
		else
		{
			if (OutputIteratorByteSizeRemaining < 4)
			{
				return false;
			}

			*OutputIterator++ = (UTF8CHAR)((Codepoint >> 18) | 128 | 64 | 32 | 16);
			*OutputIterator++ = (UTF8CHAR)(((Codepoint >> 12) & 0x3F) | 128);
			*OutputIterator++ = (UTF8CHAR)(((Codepoint >> 6) & 0x3F) | 128);
			*OutputIterator++ = (UTF8CHAR)((Codepoint & 0x3F) | 128);

			OutputIteratorByteSizeRemaining -= 4;
		}

		return true;
	}



	template <typename DestBufferType, typename FromType>
	static int32 ConvertToUTF8(DestBufferType& Dest, int32 DestLen, const FromType* Source, const int32 SourceLen)
	{
		DestBufferType DestStartingPosition = Dest;
		if constexpr (sizeof(FromType) == 4)
		{
			for (int32 i = 0; i < SourceLen; ++i)
			{
				uint32 Codepoint = static_cast<uint32>(Source[i]);

				if (!WriteCodepointToBuffer(Codepoint, Dest, DestLen))
				{
					// Could not write data, bail out
					return -1;
				}
			}
		}
		else
		{
			uint32 HighSurrogate = MAX_uint32;

			for (int32 i = 0; i < SourceLen; ++i)
			{
				const bool bHighSurrogateIsSet = HighSurrogate != MAX_uint32;
				uint32 Codepoint = static_cast<uint32>(Source[i]);

				// Check if this character is a high-surrogate
				if (IsHighSurrogate(Codepoint))
				{
					// Ensure we don't already have a high-surrogate set or end without a matching low-surrogate
					if (bHighSurrogateIsSet || i == SourceLen - 1)
					{
						// Already have a high-surrogate in this pair or string ends with lone high-surrogate
						// Write our stored value (will be converted into bogus character)
						if (!WriteCodepointToBuffer(HighSurrogate, Dest, DestLen))
						{
							// Could not write data, bail out
							return -1;
						}
					}

					// Store our code point for our next character
					HighSurrogate = Codepoint;
					continue;
				}

				// If our High Surrogate is set, check if this character is the matching low-surrogate
				if (bHighSurrogateIsSet)
				{
					if (IsLowSurrogate(Codepoint))
					{
						const uint32 LowSurrogate = Codepoint;
						// Combine our high and low surrogates together to a single Unicode codepoint
						Codepoint = EncodeSurrogate((uint16)HighSurrogate, (uint16)LowSurrogate);
					}
					else
					{
						// Did not find matching low-surrogate, write out a bogus character for our stored HighSurrogate
						if (!WriteCodepointToBuffer(HighSurrogate, Dest, DestLen))
						{
							// Could not write data, bail out
							return -1;
						}
					}

					// Reset our high-surrogate now that we've used (or discarded) its value
					HighSurrogate = MAX_uint32;
				}

				if (!WriteCodepointToBuffer(Codepoint, Dest, DestLen))
				{
					// Could not write data, bail out
					return -1;
				}
			}
		}

		return UE_PTRDIFF_TO_INT32(Dest - DestStartingPosition);
	}

	template <typename T>
	struct TIsIntegral
	{
		enum { Value = false };
	};

	template <> struct TIsIntegral<         bool> { enum { Value = true }; };
	template <> struct TIsIntegral<         char> { enum { Value = true }; };
	template <> struct TIsIntegral<signed   char> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned char> { enum { Value = true }; };
	template <> struct TIsIntegral<         char16_t> { enum { Value = true }; };
	template <> struct TIsIntegral<         char32_t> { enum { Value = true }; };
	template <> struct TIsIntegral<         wchar_t> { enum { Value = true }; };
	template <> struct TIsIntegral<         short> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned short> { enum { Value = true }; };
	template <> struct TIsIntegral<         int> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned int> { enum { Value = true }; };
	template <> struct TIsIntegral<         long> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned long> { enum { Value = true }; };
	template <> struct TIsIntegral<         long long> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned long long> { enum { Value = true }; };

	template <typename T>
	FORCEINLINE constexpr bool IsAligned(T Val, uint64 Alignment)
	{
		static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "IsAligned expects an integer or pointer type");

		return !((uint64)Val & (Alignment - 1));
	}


	static uint32 CodepointFromUtf8(const UTF8CHAR*& SourceString, const uint32 SourceLengthRemaining)
	{
		checkSlow(SourceLengthRemaining > 0);

		const UTF8CHAR* OctetPtr = SourceString;

		uint32 Codepoint = 0;
		uint32 Octet = (uint32)((uint8)*SourceString);
		uint32 Octet2, Octet3, Octet4;

		if (Octet < 128)  // one octet char: 0 to 127
		{
			++SourceString;  // skip to next possible start of codepoint.
			return Octet;
		}
		else if (Octet < 192)  // bad (starts with 10xxxxxx).
		{
			// Apparently each of these is supposed to be flagged as a bogus
			//  char, instead of just resyncing to the next valid codepoint.
			++SourceString;  // skip to next possible start of codepoint.
			return UNICODE_BOGUS_CHAR_CODEPOINT;
		}
		else if (Octet < 224)  // two octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 2)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet -= (128 + 64);
			Octet2 = (uint32)((uint8) * (++OctetPtr));
			if ((Octet2 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Codepoint = ((Octet << 6) | (Octet2 - 128));
			if ((Codepoint >= 0x80) && (Codepoint <= 0x7FF))
			{
				SourceString += 2;  // skip to next possible start of codepoint.
				return Codepoint;
			}
		}
		else if (Octet < 240)  // three octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 3)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet -= (128 + 64 + 32);
			Octet2 = (uint32)((uint8) * (++OctetPtr));
			if ((Octet2 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet3 = (uint32)((uint8) * (++OctetPtr));
			if ((Octet3 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Codepoint = (((Octet << 12)) | ((Octet2 - 128) << 6) | ((Octet3 - 128)));

			// UTF-8 characters cannot be in the UTF-16 surrogates range
			if (IsHighSurrogate(Codepoint) || IsLowSurrogate(Codepoint))
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			// 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge.
			if ((Codepoint >= 0x800) && (Codepoint <= 0xFFFD))
			{
				SourceString += 3;  // skip to next possible start of codepoint.
				return Codepoint;
			}
		}
		else if (Octet < 248)  // four octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 4)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet -= (128 + 64 + 32 + 16);
			Octet2 = (uint32)((uint8) * (++OctetPtr));
			if ((Octet2 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet3 = (uint32)((uint8) * (++OctetPtr));
			if ((Octet3 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet4 = (uint32)((uint8) * (++OctetPtr));
			if ((Octet4 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Codepoint = (((Octet << 18)) | ((Octet2 - 128) << 12) |
				((Octet3 - 128) << 6) | ((Octet4 - 128)));
			if ((Codepoint >= 0x10000) && (Codepoint <= 0x10FFFF))
			{
				SourceString += 4;  // skip to next possible start of codepoint.
				return Codepoint;
			}
		}
		// Five and six octet sequences became illegal in rfc3629.
		//  We throw the codepoint away, but parse them to make sure we move
		//  ahead the right number of bytes and don't overflow the buffer.
		else if (Octet < 252)  // five octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 5)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			SourceString += 5;  // skip to next possible start of codepoint.
			return UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		else  // six octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 6)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32)((uint8) * (++OctetPtr));
			if ((Octet & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			SourceString += 6;  // skip to next possible start of codepoint.
			return UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
		return UNICODE_BOGUS_CHAR_CODEPOINT;  // catch everything else.
	}

	template <typename DestType, typename DestBufferType>
	static int32 ConvertFromUTF8(DestBufferType& ConvertedBuffer, int32 DestLen, const UTF8CHAR* Source, const int32 SourceLen)
	{
		DestBufferType DestStartingPosition = ConvertedBuffer;

		const UTF8CHAR* SourceEnd = Source + SourceLen;

		const uint64 ExtendedCharMask = 0x8080808080808080;
		while (Source < SourceEnd)
		{
			if (DestLen == 0)
			{
				return -1;
			}

			// In case we're given an unaligned pointer, we'll
			// fallback to the slow path until properly aligned.
			if (IsAligned(Source, 8))
			{
				// Fast path for most common case
				while (Source < SourceEnd - 8 && DestLen >= 8)
				{
					// Detect any extended characters 8 chars at a time
					if ((*(const uint64*)Source) & ExtendedCharMask)
					{
						// Move to slow path since we got extended characters to process
						break;
					}

					// This should get unrolled on most compiler
					// ROI of diminished return to vectorize this as we 
					// would have to deal with alignment, endianness and
					// rewrite the iterators to support bulk writes
					for (int32 Index = 0; Index < 8; ++Index)
					{
						*(ConvertedBuffer++) = (DestType)(uint8) * (Source++);
					}
					DestLen -= 8;
				}
			}

			// Slow path for extended characters
			while (Source < SourceEnd && DestLen > 0)
			{
				// Read our codepoint, advancing the source pointer
				uint32 Codepoint = CodepointFromUtf8(Source, UE_PTRDIFF_TO_UINT32(SourceEnd - Source));

				if constexpr (sizeof(DestType) != 4)
				{
					// We want to write out two chars
					if (IsEncodedSurrogate(Codepoint))
					{
						// We need two characters to write the surrogate pair
						if (DestLen >= 2)
						{
							uint16 HighSurrogate = 0;
							uint16 LowSurrogate = 0;
							DecodeSurrogate(Codepoint, HighSurrogate, LowSurrogate);

							*(ConvertedBuffer++) = (DestType)HighSurrogate;
							*(ConvertedBuffer++) = (DestType)LowSurrogate;
							DestLen -= 2;
							continue;
						}

						// If we don't have space, write a bogus character instead (we should have space for it)
						Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
					}
					else if (Codepoint > ENCODED_SURROGATE_END_CODEPOINT)
					{
						// Ignore values higher than the supplementary plane range
						Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
					}
				}

				*(ConvertedBuffer++) = (DestType)Codepoint;
				--DestLen;

				// Return to the fast path once aligned and back to simple ASCII chars
				if (Codepoint < 128 && IsAligned(Source, 8))
				{
					break;
				}
			}
		}

		return UE_PTRDIFF_TO_INT32(ConvertedBuffer - DestStartingPosition);;
	}

	template <typename DestType>
	struct TCountingOutputIterator
	{
		TCountingOutputIterator()
			: Counter(0)
		{
		}

		const TCountingOutputIterator& operator* () const { return *this; }
		const TCountingOutputIterator& operator++() { ++Counter; return *this; }
		const TCountingOutputIterator& operator++(int) { ++Counter; return *this; }
		const TCountingOutputIterator& operator+=(const int32 Amount) { Counter += Amount; return *this; }

		const DestType& operator=(const DestType& Val) const
		{
			return Val;
		}

		friend int32 operator-(TCountingOutputIterator Lhs, TCountingOutputIterator Rhs)
		{
			return Lhs.Counter - Rhs.Counter;
		}

		int32 GetCount() const { return Counter; }

	private:
		int32 Counter;
	};

	FORCEINLINE int32 GetConvertedLength(const UTF8CHAR*, const ANSICHAR* Source, int32 SourceLen)
	{
		TCountingOutputIterator<UTF8CHAR> Dest;
		int32 Result = ConvertToUTF8(Dest, INT32_MAX, Source, SourceLen);
		return Result;
	}
	FORCEINLINE int32 GetConvertedLength(const UTF8CHAR*, const WIDECHAR* Source, int32 SourceLen)
	{
		TCountingOutputIterator<UTF8CHAR> Dest;
		int32 Result = ConvertToUTF8(Dest, INT32_MAX, Source, SourceLen);
		return Result;
	}
	FORCEINLINE int32 GetConvertedLength(const UTF8CHAR*, const UCS2CHAR* Source, int32 SourceLen)
	{
		TCountingOutputIterator<UTF8CHAR> Dest;
		int32 Result = ConvertToUTF8(Dest, INT32_MAX, Source, SourceLen);
		return Result;
	}
	FORCEINLINE int32 GetConvertedLength(const ANSICHAR*, const UTF8CHAR* Source, int32 SourceLen)
	{
		TCountingOutputIterator<ANSICHAR> Dest;
		int32 Result = ConvertFromUTF8<ANSICHAR>(Dest, INT32_MAX, Source, SourceLen);
		return Result;
	}
	FORCEINLINE int32 GetConvertedLength(const WIDECHAR*, const UTF8CHAR* Source, int32 SourceLen)
	{
		TCountingOutputIterator<WIDECHAR> Dest;
		int32 Result = ConvertFromUTF8<WIDECHAR>(Dest, INT32_MAX, Source, SourceLen);
		return Result;
	}
	FORCEINLINE int32 GetConvertedLength(const UCS2CHAR*, const UTF8CHAR* Source, int32 SourceLen)
	{
		TCountingOutputIterator<UCS2CHAR> Dest;
		int32 Result = ConvertFromUTF8<UCS2CHAR>(Dest, INT32_MAX, Source, SourceLen);
		return Result;
	}

	template <typename SourceEncoding, typename DestEncoding>
	static constexpr bool IsCharEncodingSimplyConvertibleTo()
	{
		if constexpr (IsCharEncodingCompatibleWith<SourceEncoding, DestEncoding>())
		{
			// Binary-compatible conversions are always simple
			return true;
		}
		else if constexpr (IsFixedWidthEncoding<SourceEncoding>() && sizeof(DestEncoding) >= sizeof(SourceEncoding))
		{
			// Converting from a fixed-width encoding to a wider or same encoding should always be possible,
			// as should ANSICHAR->UTF8CHAR and UCS2CHAR->UTF16CHAR,
			return true;
		}
		else
		{
			return false;
		}
	}

	template <typename DestEncoding, typename SourceEncoding>
	static int32 ConvertedLength(const SourceEncoding* Src, int32 SrcSize)
	{
		if constexpr (IsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>() || (IsFixedWidthEncoding<SourceEncoding>() && IsFixedWidthEncoding<DestEncoding>()))
		{
			return SrcSize;
		}
		else
		{
			return GetConvertedLength((DestEncoding*)nullptr, Src, SrcSize);
		}
	}

	FORCEINLINE UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const ANSICHAR* Src, int32 SrcLen)
	{
		if (ConvertToUTF8(Dest, DestLen, Src, SrcLen) == -1)
		{
			return nullptr;
		}
		return Dest;
	}
	FORCEINLINE UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const WIDECHAR* Src, int32 SrcLen)
	{
		if (ConvertToUTF8(Dest, DestLen, Src, SrcLen) == -1)
		{
			return nullptr;
		}
		return Dest;
	}
	FORCEINLINE UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const UCS2CHAR* Src, int32 SrcLen)
	{
		if (ConvertToUTF8(Dest, DestLen, Src, SrcLen) == -1)
		{
			return nullptr;
		}
		return Dest;
	}
	FORCEINLINE ANSICHAR* Convert(ANSICHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen)
	{
		if (ConvertFromUTF8<ANSICHAR>(Dest, DestLen, Src, SrcLen) == -1)
		{
			return nullptr;
		}
		return Dest;
	}
	FORCEINLINE WIDECHAR* Convert(WIDECHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen)
	{
		if (ConvertFromUTF8<WIDECHAR>(Dest, DestLen, Src, SrcLen) == -1)
		{
			return nullptr;
		}
		return Dest;
	}
	FORCEINLINE UCS2CHAR* Convert(UCS2CHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen)
	{
		if (ConvertFromUTF8<UCS2CHAR>(Dest, DestLen, Src, SrcLen) == -1)
		{
			return nullptr;
		}
		return Dest;
	}

	namespace UE::Private
	{
		template <typename SourceEncoding, typename DestEncoding>
		static FORCEINLINE DestEncoding* Convert(DestEncoding* Dest, int32 DestSize, const SourceEncoding* Src, int32 SrcSize)
		{
			if constexpr (IsCharEncodingCompatibleWith<SourceEncoding, DestEncoding>())
			{
				if (DestSize < SrcSize)
				{
					return nullptr;
				}

				return (DestEncoding*)FMemory::Memcpy(Dest, Src, SrcSize * sizeof(SourceEncoding)) + SrcSize;
			}
			else if constexpr (IsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>())
			{
				const int32 Size = DestSize <= SrcSize ? DestSize : SrcSize;
				for (int I = 0; I < Size; ++I)
				{
					SourceEncoding SrcCh = Src[I];
					Dest[I] = (DestEncoding)SrcCh;
				}

				return DestSize < SrcSize ? nullptr : Dest + Size;
			}
			else if constexpr (IsFixedWidthEncoding<SourceEncoding>() && IsFixedWidthEncoding<DestEncoding>())
			{
				const int32 Size = DestSize <= SrcSize ? DestSize : SrcSize;
				bool bInvalidChars = false;
				for (int I = 0; I < Size; ++I)
				{
					SourceEncoding SrcCh = Src[I];
					Dest[I] = (DestEncoding)SrcCh;
					bInvalidChars |= !CanConvertCodepoint<DestEncoding>(SrcCh);
				}

				if (bInvalidChars)
				{
					for (int I = 0; I < Size; ++I)
					{
						if (!CanConvertCodepoint<DestEncoding>(Src[I]))
						{
							Dest[I] = UNICODE_BOGUS_CHAR_CODEPOINT;
						}
					}

					LogBogusChars<DestEncoding>(Src, Size);
				}

				return DestSize < SrcSize ? nullptr : Dest + Size;
			}
			else
			{
				return Convert(Dest, DestSize, Src, SrcSize);
			}
		}
	}


	static FORCEINLINE int32 Strlen(const ANSICHAR* String)
	{
		return strlen(String);
	}

	static FORCEINLINE int32 Strnlen(const ANSICHAR* String, SIZE_T StringSize)
	{
		return strnlen(String, StringSize);
	}

	template<typename A, typename B>	struct TIsSame { enum { Value = false }; };
	template<typename T>				struct TIsSame<T, T> { enum { Value = true }; };

	FORCEINLINE void ConstructFromCString(/* Out */ TArray<TCHAR>& Data, const wchar_t* Src)
	{
		if (Src && *Src)
		{
			int32 SrcLen = wcslen(Src) + 1;
			int32 DestLen = ConvertedLength<TCHAR>(Src, SrcLen);
			Data.Reserve(DestLen);
			Data.AddUninitialized(DestLen);

			UE::Private::Convert(Data.GetData(), DestLen, Src, SrcLen);
		}
	}

	FORCEINLINE void ConstructFromCString(TArray<TCHAR>& Data, const char* Src)
	{
		if (Src && *Src)
		{
			int32 SrcLen = strlen(Src) + 1;
			int32 DestLen = ConvertedLength<TCHAR>(Src, SrcLen);
			Data.Reserve(DestLen);
			Data.AddUninitialized(DestLen);

			UE::Private::Convert(Data.GetData(), DestLen, Src, SrcLen);
		}
	}

	template<typename CharType>
	FORCEINLINE void ConstructWithLength(/* Out */ TArray<TCHAR>& Data, int32 InCount, const CharType* InSrc)
	{
		if (InSrc)
		{
			int32 DestLen = ConvertedLength<TCHAR>(InSrc, InCount);
			if (DestLen > 0 && *InSrc)
			{
				Data.Reserve(DestLen + 1);
				Data.AddUninitialized(DestLen + 1);

				UE::Private::Convert(Data.GetData(), DestLen, InSrc, InCount);
				*(Data.GetData() + Data.Num() - 1) = TEXT('\0');
			}
		}
	}


	FORCEINLINE void ConstructWithSlack(/* Out */ TArray<TCHAR>& Data, const wchar_t* Src, int32 ExtraSlack)
	{
		if (Src && *Src)
		{
			int32 SrcLen = wcslen(Src) + 1;
			int32 DestLen = ConvertedLength<TCHAR>(Src, SrcLen);
			Data.Reserve(DestLen + ExtraSlack);
			Data.AddUninitialized(DestLen);

			UE::Private::Convert(Data.GetData(), DestLen, Src, SrcLen);
		}
		else if (ExtraSlack > 0)
		{
			Data.Reserve(ExtraSlack + 1);
		}
	}

	FORCEINLINE void ConstructWithSlack(/* Out */ TArray<TCHAR>& Data, const char* Src, int32 ExtraSlack)
	{
		if (Src && *Src)
		{
			int32 SrcLen = strlen(Src) + 1;
			int32 DestLen = ConvertedLength<TCHAR>(Src, SrcLen);
			Data.Reserve(DestLen + ExtraSlack);
			Data.AddUninitialized(DestLen);

			UE::Private::Convert(Data.GetData(), DestLen, Src, SrcLen);
		}
		else if (ExtraSlack > 0)
		{
			Data.Reserve(ExtraSlack + 1);
		}
	}

	template <typename T>
	struct TIsContiguousContainer
	{
		enum { Value = false };
	};

	template <typename T> struct TIsContiguousContainer<             T& > : TIsContiguousContainer<T> {};
	template <typename T> struct TIsContiguousContainer<             T&&> : TIsContiguousContainer<T> {};
	template <typename T> struct TIsContiguousContainer<const          T> : TIsContiguousContainer<T> {};
	template <typename T> struct TIsContiguousContainer<      volatile T> : TIsContiguousContainer<T> {};
	template <typename T> struct TIsContiguousContainer<const volatile T> : TIsContiguousContainer<T> {};

	template <typename T>           struct TIsArray { enum { Value = false }; };
	template <typename T>           struct TIsArray<T[]> { enum { Value = true }; };
	template <typename T, uint32 N> struct TIsArray<T[N]> { enum { Value = true }; };



	template <typename T> struct TRemovePointer { typedef T Type; };
	template <typename T> struct TRemovePointer<T*> { typedef T Type; };



	template <typename T>
	T&& DeclVal();

	inline const TCHAR* GetData(const class FString& String);


	class FString
	{
	public:
		using AllocatorType = TSizedDefaultAllocator<32>;
	private:
		friend struct TContainerTraits<FString>;
		typedef TArray<TCHAR> DataType;
		DataType Data;

		template <typename RangeType>
		using TRangeElementType = typename TRemoveCV<typename TRemovePointer<decltype(GetData(DeclVal<RangeType>()))>::Type>::Type;

		using ElementType = TCHAR;

		template <typename CharRangeType>
		struct TIsRangeOfTCHAR : TIsSame<TCHAR, TRangeElementType<CharRangeType>>
		{
		};

		template <typename CharRangeType>
		using TIsTCharRangeNotCArray = TAnd<
			TIsContiguousContainer<CharRangeType>,
			TNot<TIsArray<typename TRemoveReference<CharRangeType>::Type>>,
			TIsRangeOfTCHAR<CharRangeType>>;

	public:
		FString() = default;
		FString(FString&&) = default;
		FString(const FString& OtherString) {
			this->Data.ResizeTo(OtherString.Data.Max());
			this->Data.Emplace(*OtherString.Data.GetData())
		}
		FString& operator=(FString&&) = default;
		FString& operator=(const FString&) = default;

		FString(const ANSICHAR* Str);
		FString(const WIDECHAR* Str);

		FString(int32 Len, const ANSICHAR* Str);
		FString(int32 Len, const WIDECHAR* Str);

		FString(const ANSICHAR* Str, int32 ExtraSlack);
		FString(const WIDECHAR* Str, int32 ExtraSlack);

	public:
		//FString& operator=(const TCHAR* Str);

		template <typename CharRangeType, typename TEnableIf<TIsTCharRangeNotCArray<CharRangeType>::Value>::Type* = nullptr>
		FORCEINLINE FString& operator=(CharRangeType&& Range)
		{
			AssignRange(GetData(Range), GetNum(Range));
			return *this;
		}

		FORCEINLINE int32 Len() const
		{
			return Data.Num() ? Data.Num() - 1 : 0;
		}

		void Empty()
		{
			Data.Empty(0);
		}

		FORCEINLINE const DataType& GetCharArray() const
		{
			return Data;
		}

		FORCEINLINE const TCHAR* operator*() const
		{
			return Data.Num() ? Data.GetData() : TEXT("");
		}

	private:
		void AssignRange(const TCHAR* Str, int32 Len);
	};

	inline const TCHAR* GetData(const class FString& String)
	{
		return String.GetCharArray().GetData();
	}





	template<>
	struct TContainerTraits<class FString> : public TContainerTraitsBase < class FString >
	{
		enum { MoveWillEmptyContainer = TContainerTraits<FString::DataType>::MoveWillEmptyContainer };
	};


}