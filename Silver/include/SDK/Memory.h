#pragma once

typedef uint8_t		uint8;
/// A 16-bit unsigned integer.
typedef uint16_t		uint16;
/// A 32-bit unsigned integer.
typedef uint32_t		uint32;
/// A 64-bit unsigned integer.
typedef uint64_t		uint64;

//~ Signed base types.
/// An 8-bit signed integer.
typedef	int8_t		int8;
/// A 16-bit signed integer.
typedef int16_t		int16;
/// A 32-bit signed integer.
typedef int32_t		int32;
/// A 64-bit signed integer.
typedef int64_t		int64;

namespace SDK
{
	template <typename T>
	struct TIsPointer
	{
		enum { Value = false };
	};

	template <typename T> struct TIsPointer<T*> { enum { Value = true }; };

	template <typename T> struct TIsPointer<const          T> { enum { Value = TIsPointer<T>::Value }; };
	template <typename T> struct TIsPointer<      volatile T> { enum { Value = TIsPointer<T>::Value }; };
	template <typename T> struct TIsPointer<const volatile T> { enum { Value = TIsPointer<T>::Value }; };


	class FMemory
	{
	public:
		static void Free(void* Ptr)
		{
			if (Ptr) {
				Realloc(Ptr, 0, 0);
			}
		}

		static void* Realloc(void* Ptr, UINT64 Size, uint32 Alignment = 0)
		{
			static void* (*InternalRealloc)(void*, UINT64, uint32_t) = decltype(InternalRealloc)(Offsets::FMemory_Realloc);

			return InternalRealloc(Ptr, Size, Alignment);
		}

		static size_t QuantizeSize(size_t Size, uint32_t Alignment = 16) {
			const uint32_t MinAlignment = 16;

			Alignment = max(Alignment, MinAlignment);

			size_t QuantizedSize = (Size + (Alignment - 1)) & ~(Alignment - 1);

			if (QuantizedSize <= 16) return 16;
			if (QuantizedSize <= 32) return 32;
			if (QuantizedSize <= 64) return 64;
			if (QuantizedSize <= 128) return 128;
			if (QuantizedSize <= 256) return 256;
			if (QuantizedSize <= 512) return 512;
			if (QuantizedSize <= 1024) return 1024;
			if (QuantizedSize <= 2048) return 2048;

			const size_t PageSize = 4096;
			return (QuantizedSize + (PageSize - 1)) & ~(PageSize - 1);
		}

		static void* Memmove(void* Dest, const void* Src, SIZE_T Count) {
			return memmove(Dest, Src, Count);
		}

		static void* Memcpy(void* Dest, const void* Src, SIZE_T Count) {
			return memcpy(Dest, Src, Count);
		}

		template< class T >
		static FORCEINLINE void Memcpy(T& Dest, const T& Src)
		{
			static_assert(!TIsPointer<T>::Value, "For pointers use the three parameters function");
			Memcpy(&Dest, &Src, sizeof(T));
		}

		static FORCEINLINE void* Memzero(void* Dest, SIZE_T Count)
		{
			return memset(Dest, 0, Count);
		}

		template< class T >
		static FORCEINLINE void Memzero(T& Src)
		{
			static_assert(!TIsPointer<T>::Value, "For pointers use the two parameters function");
			Memzero(&Src, sizeof(T));
		}
	};

	
}
