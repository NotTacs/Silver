#pragma once

namespace SDK
{
	class UEngine : public UObject 
	{
		GENERATED_CLASS_BODY()
	public:
		DEFINE_MEMBER(class UGameViewportClient*, UEngine, GameViewport);
	public:
		static UEngine* GetEngine();
	};

	class UGameViewportClient : public UObject
	{
		GENERATED_CLASS_BODY()
	public:
		DEFINE_MEMBER(class UWorld*, UGameViewportClient, World);
	};

	class UWorld : public UObject
	{
		GENERATED_CLASS_BODY()
	public:
		

	public:
		static UWorld* GetWorld();
	};
}