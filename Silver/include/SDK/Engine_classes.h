#pragma once

namespace SDK
{
	class UEngine : public UObject 
	{
		GENERATED_CLASS_BODY()
	public:
		DEFINE_MEMBER(UObject*&, UEngine, GameViewport);
	public:
		static UEngine* GetEngine();
	};
}