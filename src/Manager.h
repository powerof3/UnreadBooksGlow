#pragma once

struct Shader
{
	bool                 enable{ true };
	std::string          formID{ "0x000C7942" };
	RE::TESEffectShader* shader{};
};

struct FormIDMod
{
	bool operator==(const FormIDMod& lhs) const
	{
		return std::tie(formID, modName) == std::tie(lhs.formID, lhs.modName);
	}

	bool operator<(const FormIDMod& lhs) const
	{
		return std::tie(formID, modName) < std::tie(lhs.formID, lhs.modName);
	}

	RE::TESFormID formID{ 0 };
	std::string   modName{};
};

class Manager : public REX::Singleton<Manager>
{
public:
	void ReadSettings();
	void ReadForms();

	void MarkAsRead(RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book);
	void ApplyBookShader(RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book);

	void SaveDatabase(const std::string& path);
	void LoadDatabase(const std::string& path);
	void DeleteDatabase(const std::string& path);
	void CleanupDatabase();

private:
	bool HasBeenRead(RE::TESForm* a_form) const;
	void MarkAsRead(RE::TESObjectBOOK* a_book);

	RE::TESEffectShader* GetShaderForBook(RE::TESObjectBOOK* a_book) const;

	std::optional<std::filesystem::path> GetSaveDirectory();
	std::optional<std::filesystem::path> GetSaveDirectoryUBG();
	std::optional<std::filesystem::path> GetSavePathUBG(const std::string& a_fileName);

	// members
	Shader bookShader{};
	Shader skillBookShader{};
	Shader scrollShader{};

	std::unordered_set<RE::TESFormID> readBooks{};
	std::set<FormIDMod>               readBooksSerialized{};

	std::optional<std::filesystem::path> saveFolder{};
	std::optional<std::filesystem::path> saveFolderUBG{};
};
