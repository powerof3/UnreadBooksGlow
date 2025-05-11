#include "Manager.h"

void Manager::ReadSettings()
{
	constexpr auto obsePath = L"OBSE\\Plugins\\po3_UnreadBooksGlow.ini";
	constexpr auto rootPath = L"OblivionRemastered\\Binaries\\Win64\\OBSE\\Plugins\\po3_UnreadBooksGlow.ini";

	std::wstring path{};

	std::error_code ec;
	if (!std::filesystem::exists(obsePath)) {
		if (!std::filesystem::exists(rootPath)) {
			REX::WARN("OBSE\\Plugins folder not found ({})", ec.message());
			return;
		} else {
			path = rootPath;
		}
	} else {
		path = obsePath;
	}

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path.c_str());

	ini::get_value(ini, bookShader.enable, "Book", "bEnableShader", nullptr);
	ini::get_value(ini, bookShader.formID, "Book", "sEffectShader", nullptr);

	ini::get_value(ini, skillBookShader.enable, "SkillBook", "bEnableShader", nullptr);
	ini::get_value(ini, skillBookShader.formID, "SkillBook", "sEffectShader", nullptr);

	ini::get_value(ini, scrollShader.enable, "Scroll", "bEnableShader", nullptr);
	ini::get_value(ini, scrollShader.formID, "Scroll", "sEffectShader", nullptr);

	(void)ini.SaveFile(path.c_str());
}

void Manager::ReadForms()
{
	REX::INFO("Reading forms...");

	constexpr auto read_formID = [](const std::string& id) {
		if (const auto splitID = string::split(id, "~"); splitID.size() == 2) {
			const auto  formID = string::to_num<RE::TESFormID>(splitID[0], true);
			const auto& modName = splitID[1];
			return RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESEffectShader>(formID, modName);
		}
		if (string::is_only_hex(id, true)) {
			auto hexID = string::to_num<RE::TESFormID>(id, true);
			return RE::TESForm::LookupByID<RE::TESEffectShader>(hexID);
		}
		return RE::TESForm::LookupByEditorID<RE::TESEffectShader>(id);
	};

	if (bookShader.enable) {
		bookShader.shader = read_formID(bookShader.formID);
		if (!bookShader.shader) {
			REX::WARN("\tBook shader not found ({})", bookShader.formID);
		}
	}
	if (skillBookShader.enable) {
		skillBookShader.shader = read_formID(skillBookShader.formID);
		if (!skillBookShader.shader) {
			REX::WARN("\tSkill book shader not found ({})", skillBookShader.formID);
		}
	}
	if (scrollShader.enable) {
		scrollShader.shader = read_formID(scrollShader.formID);
		if (!scrollShader.shader) {
			REX::WARN("\tScroll shader not found ({})", scrollShader.formID);
		}
	}

	REX::INFO("\tBook Shader = {:X}", bookShader.shader ? bookShader.shader->GetFormID() : -1);
	REX::INFO("\tSkill Book Shader = {:X}", skillBookShader.shader ? skillBookShader.shader->GetFormID() : -1);
	REX::INFO("\tScroll Shader = {:X}", scrollShader.shader ? scrollShader.shader->GetFormID() : -1);
}

bool Manager::HasBeenRead(RE::TESForm* a_form) const
{
	return readBooks.contains(a_form->GetFormID());
}

void Manager::MarkAsRead(RE::TESObjectBOOK* a_book)
{
	if (!a_book->IsDynamicForm()) {
		readBooks.insert(a_book->GetFormID());
		readBooksSerialized.insert({ a_book->GetLocalFormID(), a_book->files.front()->filename });
	}
}

RE::TESEffectShader* Manager::GetShaderForBook(RE::TESObjectBOOK* a_book) const
{
	if ((a_book->data.flags & 1) != 0) {
		return scrollShader.shader;
	} else if (a_book->data.teaches != -1) {
		return skillBookShader.shader;
	} else {
		return bookShader.shader;
	}
}

void Manager::MarkAsRead(RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book)
{
	MarkAsRead(a_book);

	if (auto shader = GetShaderForBook(a_book)) {
		if (a_ref) {
			RE::ProcessLists::GetSingleton()->FinishMagicShaderHitEffect(a_ref, shader);
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		auto pcCell = player ? player->parentCell : nullptr;

		if (pcCell) {
			RE::TESObjectCELL::CellRefObjectThreadLock()->LockEnter(pcCell);
			for (auto& ref : pcCell->listReferences) {
				if (auto baseObject = ref ? ref->GetBaseObject() : nullptr; baseObject && baseObject == a_book) {
					RE::ProcessLists::GetSingleton()->FinishMagicShaderHitEffect(ref, shader);
				}
			}
			RE::TESObjectCELL::CellRefObjectThreadLock()->LockLeave(pcCell);
		}
	}
}

void Manager::ApplyBookShader(RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book)
{
	if (HasBeenRead(a_book)) {
		return;
	}

	if (auto effectShader = Manager::GetSingleton()->GetShaderForBook(a_book)) {
		//auto effectShader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x000C7942);

		auto effect = RE::MagicShaderHitEffect::Create(a_ref, effectShader, -1.0);
		if (effect && effect->Init()) {
			RE::ProcessLists::GetSingleton()->MagicEffectList.emplace_front(effect);
		}
	}
}

std::optional<std::filesystem::path> Manager::GetSaveDirectory()
{
	if (saveFolder) {
		return saveFolder;
	}

	wchar_t*                                                       knownBuffer{ nullptr };
	const auto                                                     knownResult = REX::W32::SHGetKnownFolderPath(REX::W32::FOLDERID_Documents, REX::W32::KF_FLAG_DEFAULT, nullptr, std::addressof(knownBuffer));
	std::unique_ptr<wchar_t[], decltype(&REX::W32::CoTaskMemFree)> knownPath(knownBuffer, REX::W32::CoTaskMemFree);
	if (!knownPath || knownResult != 0) {
		REX::WARN("failed to get known folder path");
		return {};
	}

	saveFolder = knownPath.get();
	*saveFolder /= std::format("My Games/{}/Saved/SaveGames", OBSE::GetSaveFolderName());

	std::error_code ec;
	if (!std::filesystem::exists(*saveFolder, ec)) {
		std::filesystem::create_directory(*saveFolder);
	}

	return saveFolder;
}

std::optional<std::filesystem::path> Manager::GetSavePath(const std::string& a_fileName)
{
	auto bevePath = GetSaveDirectory();

	if (!bevePath) {
		return {};
	}

	*bevePath /= std::format("UnreadBooksGlow/{}.beve", a_fileName);

	return bevePath;
}

void Manager::SaveDatabase(const std::string& path)
{
	auto savePath = GetSavePath(path);

	if (!savePath) {
		return;
	}

	REX::INFO("Saving {} ({} books read)", savePath->string(), readBooks.size());

	std::string buffer;
	auto        ec = glz::write_file_beve(readBooksSerialized, savePath->string(), buffer);

	if (ec) {
		REX::WARN("\tFailed to save {} file: (error: {})", path, glz::format_error(ec, buffer));
	}
}

void Manager::LoadDatabase(const std::string& path)
{
	auto savePath = GetSavePath(path);

	if (!savePath) {
		return;
	}

	readBooks.clear();
	readBooksSerialized.clear();

	std::error_code err;
	if (std::filesystem::exists(*savePath, err)) {
		std::string buffer;
		auto        ec = glz::read_file_beve(readBooksSerialized, savePath->string(), buffer);
		if (ec) {
			REX::WARN("\tFailed to load {} file (error: {})", path, glz::format_error(ec, buffer));
		}
	} else {
		REX::WARN("\tFailed to load {} file (error: {})", path, err.value());
	}

	for (auto& [formID, modName] : readBooksSerialized) {
		if (auto form = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESObjectBOOK>(formID, modName)) {
			readBooks.insert(form->GetFormID());
		}
	}

	REX::INFO("Loading {} ({} books read)", savePath->string(), readBooks.size());
}

void Manager::DeleteDatabase(const std::string& path)
{
	auto savePath = GetSavePath(path);

	if (!savePath) {
		return;
	}

	REX::INFO("Deleting {}", savePath->string());

	std::error_code err;
	if (std::filesystem::exists(*savePath, err)) {
		std::filesystem::remove(*savePath, err);
		if (err) {
			REX::WARN("\tFailed to delete {} file (error: {})", savePath->string(), err.message());
		}
	}
}

void Manager::CleanupDatabase()
{
	auto savePath = GetSaveDirectory();
	if (!savePath) {
		return;
	}

	*savePath /= "UnreadBooksGlow";

	std::vector<std::filesystem::path> missingFiles;

	for (const auto iterator = std::filesystem::directory_iterator(*savePath); const auto& entry : iterator) {
		if (entry.exists()) {
			if (const auto& path = entry.path(); !path.empty() && (path.extension() == ".beve"sv)) {
				auto saveDir = GetSaveDirectory();
				*saveDir /= path.filename().string();
				saveDir->replace_extension(".sav");

				std::error_code err;
				if (!std::filesystem::exists(*saveDir, err)) {
					missingFiles.push_back(path);
				}
			}
		}
	}

	for (auto& file : missingFiles) {
		std::error_code err;
		std::filesystem::remove(file, err);
		REX::INFO("Deleting orphaned save file {}", file.string());
	}
}
