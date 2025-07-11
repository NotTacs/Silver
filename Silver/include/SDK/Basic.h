#pragma once

namespace SDK
{

	enum EObjectFlags
	{
		// Do not add new flags unless they truly belong here. There are alternatives.
		// if you change any the bit of any of the RF_Load flags, then you will need legacy serialization
		RF_NoFlags = 0x00000000,	///< No flags, used to avoid a cast

		// This first group of flags mostly has to do with what kind of object it is. Other than transient, these are the persistent object flags.
		// The garbage collector also tends to look at these.
		RF_Public = 0x00000001,	///< Object is visible outside its package.
		RF_Standalone = 0x00000002,	///< Keep object around for editing even if unreferenced.
		RF_MarkAsNative = 0x00000004,	///< Object (UField) will be marked as native on construction (DO NOT USE THIS FLAG in HasAnyFlags() etc)
		RF_Transactional = 0x00000008,	///< Object is transactional.
		RF_ClassDefaultObject = 0x00000010,	///< This object is its class's default object
		RF_ArchetypeObject = 0x00000020,	///< This object is a template for another object - treat like a class default object
		RF_Transient = 0x00000040,	///< Don't save object.

		// This group of flags is primarily concerned with garbage collection.
		RF_MarkAsRootSet = 0x00000080,	///< Object will be marked as root set on construction and not be garbage collected, even if unreferenced (DO NOT USE THIS FLAG in HasAnyFlags() etc)
		RF_TagGarbageTemp = 0x00000100,	///< This is a temp user flag for various utilities that need to use the garbage collector. The garbage collector itself does not interpret it.

		// The group of flags tracks the stages of the lifetime of a uobject
		RF_NeedInitialization = 0x00000200,	///< This object has not completed its initialization process. Cleared when ~FObjectInitializer completes
		RF_NeedLoad = 0x00000400,	///< During load, indicates object needs loading.
		RF_KeepForCooker = 0x00000800,	///< Keep this object during garbage collection because it's still being used by the cooker
		RF_NeedPostLoad = 0x00001000,	///< Object needs to be postloaded.
		RF_NeedPostLoadSubobjects = 0x00002000,	///< During load, indicates that the object still needs to instance subobjects and fixup serialized component references
		RF_NewerVersionExists = 0x00004000,	///< Object has been consigned to oblivion due to its owner package being reloaded, and a newer version currently exists
		RF_BeginDestroyed = 0x00008000,	///< BeginDestroy has been called on the object.
		RF_FinishDestroyed = 0x00010000,	///< FinishDestroy has been called on the object.

		// Misc. Flags
		RF_BeingRegenerated = 0x00020000,	///< Flagged on UObjects that are used to create UClasses (e.g. Blueprints) while they are regenerating their UClass on load (See FLinkerLoad::CreateExport()), as well as UClass objects in the midst of being created
		RF_DefaultSubObject = 0x00040000,	///< Flagged on subobjects that are defaults
		RF_WasLoaded = 0x00080000,	///< Flagged on UObjects that were loaded
		RF_TextExportTransient = 0x00100000,	///< Do not export object to text form (e.g. copy/paste). Generally used for sub-objects that can be regenerated from data in their parent object.
		RF_LoadCompleted = 0x00200000,	///< Object has been completely serialized by linkerload at least once. DO NOT USE THIS FLAG, It should be replaced with RF_WasLoaded.
		RF_InheritableComponentTemplate = 0x00400000, ///< Archetype of the object can be in its super class
		RF_DuplicateTransient = 0x00800000,	///< Object should not be included in any type of duplication (copy/paste, binary duplication, etc.)
		RF_StrongRefOnFrame = 0x01000000,	///< References to this object from persistent function frame are handled as strong ones.
		RF_NonPIEDuplicateTransient = 0x02000000,	///< Object should not be included for duplication unless it's being duplicated for a PIE session
		RF_Dynamic  = 0x04000000,	///< Field Only. Dynamic field - doesn't get constructed during static initialization, can be constructed multiple times  // @todo: BP2CPP_remove
		RF_WillBeLoaded = 0x08000000,	///< This object was constructed during load and will be loaded shortly
		RF_HasExternalPackage = 0x10000000,	///< This object has an external package assigned and should look it up when getting the outermost package

		// RF_Garbage and RF_PendingKill are mirrored in EInternalObjectFlags because checking the internal flags is much faster for the Garbage Collector
		// while checking the object flags is much faster outside of it where the Object pointer is already available and most likely cached.
		// RF_PendingKill is mirrored in EInternalObjectFlags because checking the internal flags is much faster for the Garbage Collector
		// while checking the object flags is much faster outside of it where the Object pointer is already available and most likely cached.

		RF_PendingKill  = 0x20000000,	///< Objects that are pending destruction (invalid for gameplay but valid objects). This flag is mirrored in EInternalObjectFlags as PendingKill for performance
		RF_Garbage = 0x40000000,	///< Garbage from logical point of view and should not be referenced. This flag is mirrored in EInternalObjectFlags as Garbage for performance
		RF_AllocatedInSharedPage = 0x80000000,	///< Allocated from a ref-counted page shared with other UObjects
	};

	class FName
	{
	private:
		int32 ComparisonIndex;
		uint32 Number;

	public:
		FORCEINLINE int32 GetNumber() const { return Number; }

		FORCEINLINE void SetNumber(const int32 NewNumber) {
			Number = NewNumber;
		}

		FORCEINLINE int32 GetComparisonIndex() const
		{
			return ComparisonIndex;
		}

		FString ToString() const;
	};

	class UClass;
	class UObject;

	class UObjectBase
	{
		friend class FUObjectArray;
	public:
		bool IsValidLowLevel() const;

		FORCEINLINE uint32 GetUniqueID() const
		{
			return (uint32)InternalIndex;
		}

		/** Returns the UClass that defines the fields of this object */
		FORCEINLINE UClass* GetClass() const
		{
			return ClassPrivate;
		}

		/** Returns the UObject this object resides in */
		FORCEINLINE UObject* GetOuter() const
		{
			return OuterPrivate;
		}

		/** Returns the logical name of this object */
		FORCEINLINE FName GetFName() const
		{
			return NamePrivate;
		}
	private:
		void** VTable;
		/** Flags used to track and report various object states. This needs to be 8 byte aligned on 32-bit
		platforms to reduce memory waste */
		EObjectFlags					ObjectFlags;

		/** Index into GObjectArray...very private. */
		int32							InternalIndex;

		/** Class the object belongs to. */
		UClass* ClassPrivate;

		/** Name of this object */
		FName							NamePrivate;

		/** Object this object resides in. */
		UObject* OuterPrivate;
	};

	class UObjectBaseUtility : public UObjectBase
	{

	};

	class UObject : public UObjectBaseUtility
	{
		
	};

	struct FUObjectItem
	{
		// Pointer to the allocated object
		class UObjectBase* Object;
		// Internal flags
		int32 Flags;
		// UObject Owner Cluster Index
		int32 ClusterRootIndex;
		// Weak Object Pointer Serial number associated with the object
		int32 SerialNumber;


		FUObjectItem()
			: Object(nullptr)
			, Flags(0)
			, ClusterRootIndex(0)
			, SerialNumber(0)
		{
		}

		FUObjectItem(FUObjectItem&&) = delete;
		FUObjectItem(const FUObjectItem&) = delete;
		FUObjectItem& operator=(FUObjectItem&&) = delete;
		FUObjectItem& operator=(const FUObjectItem&) = delete;

		FORCEINLINE void SetOwnerIndex(int32 OwnerIndex)
		{
			ClusterRootIndex = OwnerIndex;
		}

		FORCEINLINE int32 GetOwnerIndex() const
		{
			return ClusterRootIndex;
		}

		/** Encodes the cluster index in the ClusterRootIndex variable */
		FORCEINLINE void SetClusterIndex(int32 ClusterIndex)
		{
			ClusterRootIndex = -ClusterIndex - 1;
		}

		/** Decodes the cluster index from the ClusterRootIndex variable */
		FORCEINLINE int32 GetClusterIndex() const
		{
			checkSlow(ClusterRootIndex < 0);
			return -ClusterRootIndex - 1;
		}

		FORCEINLINE int32 GetSerialNumber() const
		{
			return SerialNumber;
		}
	};

	class FFixedUObjectArray
	{
		/** Static master table to chunks of pointers **/
		FUObjectItem* Objects;
		/** Number of elements we currently have **/
		int32 MaxElements;
		/** Current number of UObject slots */
		int32 NumElements;

	public:

		FFixedUObjectArray()
			: Objects(nullptr)
			, MaxElements(0)
			, NumElements(0)
		{
		}

		~FFixedUObjectArray()
		{
			delete[] Objects;
		}

		/**
		* Expands the array so that Element[Index] is allocated. New pointers are all zero.
		* @param Index The Index of an element we want to be sure is allocated
		**/
		void PreAllocate(int32 InMaxElements)
		{
			check(!Objects);
			Objects = new FUObjectItem[InMaxElements];
			MaxElements = InMaxElements;
		}

		FORCEINLINE FUObjectItem const* GetObjectPtr(int32 Index) const 
		{
			check(Index >= 0 && Index < NumElements);
			return &Objects[Index];
		}

		FORCEINLINE FUObjectItem* GetObjectPtr(int32 Index)
		{
			check(Index >= 0 && Index < NumElements);
			return &Objects[Index];
		}

		/**
		* Return the number of elements in the array
		* Thread safe, but you know, someone might have added more elements before this even returns
		* @return	the number of elements in the array
		**/
		FORCEINLINE int32 Num() const
		{
			return NumElements;
		}

		/**
		* Return the number max capacity of the array
		* Thread safe, but you know, someone might have added more elements before this even returns
		* @return	the maximum number of elements in the array
		**/
		FORCEINLINE int32 Capacity() const
		{
			return MaxElements;
		}

		/**
		* Return if this index is valid
		* Thread safe, if it is valid now, it is valid forever. Other threads might be adding during this call.
		* @param	Index	Index to test
		* @return	true, if this is a valid
		**/
		FORCEINLINE bool IsValidIndex(int32 Index) const
		{
			return Index < Num() && Index >= 0;
		}
		/**
		* Return a reference to an element
		* @param	Index	Index to return
		* @return	a reference to the pointer to the element
		* Thread safe, if it is valid now, it is valid forever. This might return nullptr, but by then, some other thread might have made it non-nullptr.
		**/
		FORCEINLINE FUObjectItem const& operator[](int32 Index) const
		{
			FUObjectItem const* ItemPtr = GetObjectPtr(Index);
			check(ItemPtr);
			return *ItemPtr;
		}

		FORCEINLINE FUObjectItem& operator[](int32 Index)
		{
			FUObjectItem* ItemPtr = GetObjectPtr(Index);
			check(ItemPtr);
			return *ItemPtr;
		}

		/**
		* Return a naked pointer to the fundamental data structure for debug visualizers.
		**/
		UObjectBase*** GetRootBlockForDebuggerVisualizers()
		{
			return nullptr;
		}
	};

	/**
	* Simple array type that can be expanded without invalidating existing entries.
	* This is critical to thread safe FNames.
	* @param ElementType Type of the pointer we are storing in the array
	* @param MaxTotalElements absolute maximum number of elements this array can ever hold
	* @param ElementsPerChunk how many elements to allocate in a chunk
	**/
	class FChunkedFixedUObjectArray
	{
		enum
		{
			NumElementsPerChunk = 64 * 1024,
		};

		/** Master table to chunks of pointers **/
		FUObjectItem** Objects;
		/** If requested, a contiguous memory where all objects are allocated **/
		FUObjectItem* PreAllocatedObjects;
		/** Maximum number of elements **/
		int32 MaxElements;
		/** Number of elements we currently have **/
		int32 NumElements;
		/** Maximum number of chunks **/
		int32 MaxChunks;
		/** Number of chunks we currently have **/
		int32 NumChunks;

	public:

		/** Constructor : Probably not thread safe **/
		FChunkedFixedUObjectArray()
			: Objects(nullptr)
			, PreAllocatedObjects(nullptr)
			, MaxElements(0)
			, NumElements(0)
			, MaxChunks(0)
			, NumChunks(0)
		{
		}

		~FChunkedFixedUObjectArray()
		{
			if (!PreAllocatedObjects)
			{
				for (int32 ChunkIndex = 0; ChunkIndex < MaxChunks; ++ChunkIndex)
				{
					delete[] Objects[ChunkIndex];
				}
			}
			else
			{
				delete[] PreAllocatedObjects;
			}
			delete[] Objects;
		}

		/**
		* Expands the array so that Element[Index] is allocated. New pointers are all zero.
		* @param Index The Index of an element we want to be sure is allocated
		**/
		void PreAllocate(int32 InMaxElements, bool bPreAllocateChunks)
		{
			check(!Objects);
			MaxChunks = InMaxElements / NumElementsPerChunk + 1;
			MaxElements = MaxChunks * NumElementsPerChunk;
			Objects = new FUObjectItem * [MaxChunks];
			FMemory::Memzero(Objects, sizeof(FUObjectItem*) * MaxChunks);
			if (bPreAllocateChunks)
			{
				// Fully allocate all chunks as contiguous memory
				PreAllocatedObjects = new FUObjectItem[MaxElements];
				for (int32 ChunkIndex = 0; ChunkIndex < MaxChunks; ++ChunkIndex)
				{
					Objects[ChunkIndex] = PreAllocatedObjects + ChunkIndex * NumElementsPerChunk;
				}
				NumChunks = MaxChunks;
			}
		}

		/**
		* Return the number of elements in the array
		* Thread safe, but you know, someone might have added more elements before this even returns
		* @return	the number of elements in the array
		**/
		FORCEINLINE int32 Num() const
		{
			return NumElements;
		}

		/**
		* Return the number max capacity of the array
		* Thread safe, but you know, someone might have added more elements before this even returns
		* @return	the maximum number of elements in the array
		**/
		FORCEINLINE int32 Capacity() const
		{
			return MaxElements;
		}

		/**
		* Return if this index is valid
		* Thread safe, if it is valid now, it is valid forever. Other threads might be adding during this call.
		* @param	Index	Index to test
		* @return	true, if this is a valid
		**/
		FORCEINLINE bool IsValidIndex(int32 Index) const
		{
			return Index < Num() && Index >= 0;
		}

		/**
		* Return a pointer to the pointer to a given element
		* @param Index The Index of an element we want to retrieve the pointer-to-pointer for
		**/
		FORCEINLINE FUObjectItem const* GetObjectPtr(int32 Index) const
		{
			const int32 ChunkIndex = Index / NumElementsPerChunk;
			const int32 WithinChunkIndex = Index % NumElementsPerChunk;
			checkf(IsValidIndex(Index), TEXT("IsValidIndex(%d)"), Index);
			checkf(ChunkIndex < NumChunks, TEXT("ChunkIndex (%d) < NumChunks (%d)"), ChunkIndex, NumChunks);
			checkf(Index < MaxElements, TEXT("Index (%d) < MaxElements (%d)"), Index, MaxElements);
			FUObjectItem* Chunk = Objects[ChunkIndex];
			check(Chunk);
			return Chunk + WithinChunkIndex;
		}
		FORCEINLINE FUObjectItem* GetObjectPtr(int32 Index)
		{
			const int32 ChunkIndex = Index / NumElementsPerChunk;
			const int32 WithinChunkIndex = Index % NumElementsPerChunk;
			checkf(IsValidIndex(Index), TEXT("IsValidIndex(%d)"), Index);
			checkf(ChunkIndex < NumChunks, TEXT("ChunkIndex (%d) < NumChunks (%d)"), ChunkIndex, NumChunks);
			checkf(Index < MaxElements, TEXT("Index (%d) < MaxElements (%d)"), Index, MaxElements);
			FUObjectItem* Chunk = Objects[ChunkIndex];
			check(Chunk);
			return Chunk + WithinChunkIndex;
		}

		/**
		* Return a reference to an element
		* @param	Index	Index to return
		* @return	a reference to the pointer to the element
		* Thread safe, if it is valid now, it is valid forever. This might return nullptr, but by then, some other thread might have made it non-nullptr.
		**/
		FORCEINLINE FUObjectItem const& operator[](int32 Index) const
		{
			FUObjectItem const* ItemPtr = GetObjectPtr(Index);
			check(ItemPtr);
			return *ItemPtr;
		}
		FORCEINLINE FUObjectItem& operator[](int32 Index)
		{
			FUObjectItem* ItemPtr = GetObjectPtr(Index);
			check(ItemPtr);
			return *ItemPtr;
		}

		/**
		* Return a naked pointer to the fundamental data structure for debug visualizers.
		**/
		FUObjectItem*** GetRootBlockForDebuggerVisualizers()
		{
			return nullptr;
		}

		int64 GetAllocatedSize() const
		{
			return MaxChunks * sizeof(FUObjectItem*) + NumChunks * NumElementsPerChunk * sizeof(FUObjectItem);
		}
	};

	class FUObjectArray {


	public:
		FORCEINLINE int32 ObjectToIndex(const UObject* Object) const {
			return Object->InternalIndex;
		}

		FUObjectArray() = default;

		FUObjectArray(void* ObjObjectsPtr, bool IsChunked)
		{
			m_ObjObjects = ObjObjectsPtr;
			m_Chunked = IsChunked;
		}

	private:
		void* m_ObjObjects;
		bool m_Chunked;

	public:
		void* GetObjObjects() { return m_ObjObjects; };
		void* GetObjObjects() const { return m_ObjObjects; };

		/**
		 * Returns the UObject corresponding to index. Be advised this is only
		 * for very low level use.
		 *
		 * @param Index index of object to return
		 * @return Object at this index
		 */
		FORCEINLINE FUObjectItem* IndexToObject(int32 Index) {
			check(Index >= 0);
			void* ObjObjects = GetObjObjects();
			if (m_Chunked) {
				FChunkedFixedUObjectArray* chunkedObjObjects =
					(FChunkedFixedUObjectArray*)ObjObjects;
				if (Index < chunkedObjObjects->Num()) {
					return chunkedObjObjects->GetObjectPtr(Index);
				}
			}
			else {
				FFixedUObjectArray* FixedObjObjects =
					(FFixedUObjectArray*)ObjObjects;
				if (Index < FixedObjObjects->Num()) {
					return FixedObjObjects->GetObjectPtr(Index);
				}
			}

			return nullptr;
		}

		FORCEINLINE FUObjectItem* IndexToObject(int32 Index) const {
			check(Index >= 0);
			void* ObjObjects = GetObjObjects();
			if (m_Chunked) {
				FChunkedFixedUObjectArray* chunkedObjObjects =
					(FChunkedFixedUObjectArray*)ObjObjects;
				if (Index < chunkedObjObjects->Num()) {
					return chunkedObjObjects->GetObjectPtr(Index);
				}
			}
			else {
				FFixedUObjectArray* FixedObjObjects =
					(FFixedUObjectArray*)ObjObjects;
				if (Index < FixedObjObjects->Num()) {
					return FixedObjObjects->GetObjectPtr(Index);
				}
			}

			return nullptr;
		}

		int64 GetAllocatedSize() const {
			void* ObjObjects = GetObjObjects();
			if (m_Chunked) {
				FChunkedFixedUObjectArray* chunkedObjObjects =
					(FChunkedFixedUObjectArray*)ObjObjects;
				return chunkedObjObjects->GetAllocatedSize();
			}
			else {
				printf("[FortSDK] Illegal Call on %s on a version "
					"with FixedObjects",
					__FUNCTION__);
			}

			return 0;
		}

		FORCEINLINE int32 GetObjectArrayNum() const
		{
			void* ObjObjects = GetObjObjects();
			if (m_Chunked) {
				FChunkedFixedUObjectArray* chunkedObjObjects =
					(FChunkedFixedUObjectArray*)ObjObjects;
				return chunkedObjObjects->Num();
			}
			else {
				FFixedUObjectArray* FixedObjObjects =
					(FFixedUObjectArray*)ObjObjects;
				return FixedObjObjects->Num();
			}

			return 0;
		}

		FORCEINLINE UObject* FindObject(std::wstring Name) const {
			for (int i = 0; i < this->GetObjectArrayNum(); i++) {
				FUObjectItem* ObjectItem = this->IndexToObject(i);
				if (!ObjectItem)
					continue;
				UObjectBase* Object = ObjectItem->Object;
				if (!Object)
					continue;
				if (*Object->GetFName().ToString() == Name)
					return (UObject*)Object;
			}
			return nullptr;
		}

		FORCEINLINE int32 GetSerialNumber(int32 Index) {
			FUObjectItem* ObjectItem = IndexToObject(Index);
			checkSlow(ObjectItem);
			return ObjectItem->GetSerialNumber();
		}

		FORCEINLINE FUObjectItem*
			ObjectToObjectItem(const UObjectBase* Object) {
			FUObjectItem* ObjectItem =
				IndexToObject(Object->InternalIndex);
			return ObjectItem;
		}
	};
}

