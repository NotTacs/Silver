#pragma once

namespace SDK
{
	struct FEngineVersion
	{
	private:
		int Major = 0;
		int Minor = 0;
		int Patch = 0;

	public:
		FEngineVersion(int Major = 0, int Minor = 0, int Patch = 0)
		: Major(Major), Minor(Minor), Patch(Patch) {
		}

		FEngineVersion(std::wstring Combined) {
			std::wstringstream ss(Combined);
			std::wstring token;
			const wchar_t* Delim = L".";
			std::vector<int> parts;
			while (std::getline(ss, token, *Delim))
			{
				parts.push_back(std::stoi(token));
			}

			Major = parts[0];
			Minor = parts[1];
			Patch = parts[2];
		}

		FString ToString() const;

		bool operator==(const FEngineVersion& Other) const {
			return Major == Other.Major && Minor == Other.Minor &&
				Patch == Other.Patch;
		}

		bool operator!=(const FEngineVersion& Other) const {
			return !(*this == Other);
		}

		bool operator<(const FEngineVersion& Other) const {
			if (Major != Other.Major)
				return Major < Other.Major;
			if (Minor != Other.Minor)
				return Minor < Other.Minor;
			return Patch < Other.Patch;
		}

		bool operator<=(const FEngineVersion& Other) const {
			return *this < Other || *this == Other;
		}

		bool operator>(const FEngineVersion& Other) const {
			return Other < *this;
		}

		bool operator>=(const FEngineVersion& Other) const {
			return *this > Other || *this == Other;
		}
	};

	struct FFortniteVersion
	{
	private:
		int Major = 0;
		int Minor = 0;
		int Patch = 0;

	public:
		FFortniteVersion(int InMajor = 0, int InMinor = 0, int InPatch = 0)
			: Major(InMajor), Minor(InMinor), Patch(InPatch) {
		}

		FFortniteVersion(std::wstring Combined) {
			std::wstringstream ss(Combined);
			std::wstring idk;
			std::vector<int> parts;
			while (std::getline(ss, idk, L'.')) {
				parts.push_back(std::stoi(idk));
			}

			Major = parts[0];
			Minor = parts[1];
			Patch = parts.size() == 3 ? parts[2] : 0;
		}

		FString ToString() const;

		bool operator==(const FFortniteVersion& Other) const {
			return Major == Other.Major && Minor == Other.Minor &&
				Patch == Other.Patch;
		}

		bool operator!=(const FFortniteVersion& Other) const {
			return !(*this == Other);
		}

		bool operator<(const FFortniteVersion& Other) const {
			if (Major != Other.Major)
				return Major < Other.Major;
			if (Minor != Other.Minor)
				return Minor < Other.Minor;
			return Patch < Other.Patch;
		}

		bool operator<=(const FFortniteVersion& Other) const {
			return *this < Other || *this == Other;
		}

		bool operator>(const FFortniteVersion& Other) const {
			return Other < *this;
		}

		bool operator>=(const FFortniteVersion& Other) const {
			return *this > Other || *this == Other;
		}
	};
}