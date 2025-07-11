#pragma once

#define CONSTEXPR constexpr

namespace SDK
{
	class FMath
	{
	public:
		template< class T >
		static CONSTEXPR FORCEINLINE T Abs(const T A)
		{
			return (A >= (T)0) ? A : -A;
		}

		/** Returns 1, 0, or -1 depending on relation of T to 0 */
		template< class T >
		static CONSTEXPR FORCEINLINE T Sign(const T A)
		{
			return (A > (T)0) ? (T)1 : ((A < (T)0) ? (T)-1 : (T)0);
		}

		/** Returns higher value in a generic way */
		template< class T >
		static CONSTEXPR FORCEINLINE T Max(const T A, const T B)
		{
			return (A >= B) ? A : B;
		}

		/** Returns lower value in a generic way */
		template< class T >
		static CONSTEXPR FORCEINLINE T Min(const T A, const T B)
		{
			return (A <= B) ? A : B;
		}
	};
}