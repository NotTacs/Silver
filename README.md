# Silver

Pretty Good Fortnite Universal SDK, its kinda difficult to use, just look around in like some of the main files and you will see the macros you need. Ill upload an example of a class in a minute.

```C++
class AFortGameStateAthena : public AFortGameState
	{
		GENERATED_CLASS_BODY()
	public:
		DEFINE_MEMBER(AFortAthenaMapInfo*, AFortGameStateAthena, MapInfo)
		inline struct FPlaylistPropertyArray& getprop_CurrentPlaylistInfo() {
			static int PropOffset = -1; if (PropOffset == -1) {
				PropOffset = SDK::PropertyFinder::FindPropertyByName(("AFortGameStateAthena" + 1), "CurrentPlaylistInfo").Offset;
			} return *reinterpret_cast<std::remove_reference_t<struct FPlaylistPropertyArray&>*>(uintptr_t(this) + PropOffset);
		} __declspec(property(get = getprop_CurrentPlaylistInfo)) struct FPlaylistPropertyArray& CurrentPlaylistInfo;
		DEFINE_MEMBER(int32, AFortGameStateAthena, CurrentPlaylistId)
		DEFINE_MEMBER(TArray<struct FAdditionalLevelStreamed>&, AFortGameStateAthena, AdditionalPlaylistLevelsStreamed);
		DEFINE_MEMBER(int32, AFortGameStateAthena, SafeZonePhase);
		DEFINE_BOOL(AFortGameStateAthena, bAircraftIsLocked);
		DEFINE_MEMBER(float, AFortGameStateAthena, SafeZonesStartTime);

	public:
		void OnRep_CurrentPlaylistInfo();
		void OnRep_CurrentPlaylistId();
		void OnRep_AdditionalPlaylistLevelsStreamed();
	};
```
