#include "Manager.h"

// on book load 3d
struct Hook_LoadGraphics
{
	static std::uintptr_t LoadGraphicsFunc(RE::TESObjectREFR* a_ref)
	{
		auto result = LoadGraphicsFuncHook(a_ref);

		auto baseObject = a_ref ? a_ref->GetBaseObject() : nullptr;
		if (auto book = baseObject ? baseObject->As<RE::TESObjectBOOK>() : nullptr) {
			Manager::GetSingleton()->ApplyBookShader(a_ref, book);
		}

		return result;
	}
	static inline REL::HookVFT LoadGraphicsFuncHook{ RE::TESObjectREFR::VTABLE[0], 0x59, LoadGraphicsFunc };
};

// on read
struct Hook_CreateBookMenu
{
	static void CreateBookMenuFunc(RE::TESObjectBOOK* a_book, RE::TESObjectREFR* a_bookRef)
	{
		CreateBookMenuFuncHook(a_book, a_bookRef);

		Manager::GetSingleton()->MarkAsRead(a_bookRef, a_book);
	}
	static inline REL::Hook CreateBookMenuFuncHook{ REL::Offset(0x695D080), 0xF4, CreateBookMenuFunc };
};

// on game load
struct Hook_LoadGame
{
	static RE::VSaveFile* PrepareSaveGameFunc(std::uintptr_t a_this, RE::BSFile* file, const char* fileName, std::uint32_t action)
	{
		auto result = PrepareSaveGameFuncHook(a_this, file, fileName, action);
		if (result && result->FileIsGood()) {
			Manager::GetSingleton()->LoadDatabase(result->saveName);
		}
		return result;
	}
	static inline REL::Hook PrepareSaveGameFuncHook{ REL::Offset(0x6738A10), 0x11C, PrepareSaveGameFunc };
};

// on game save
struct Hook_SaveGame
{
	static UE::FString* GetSaveNameFunc(std::uintptr_t a_this,
		UE::FString*                                   result,
		const UE::FString*                             slotName)
	{
		auto saveName = std::format("{}", *slotName);
		Manager::GetSingleton()->SaveDatabase(saveName);

		return GetSaveNameFuncHook(a_this, result, slotName);
	}
	static inline REL::Hook GetSaveNameFuncHook{ REL::Offset(0x48E7AC0), 0x2B5, GetSaveNameFunc };
};

// on game save delete
struct Hook_DeleteGame
{
	static std::uintptr_t DeleteSaveFunc(std::uintptr_t a_this, std::uintptr_t resultOut, const UE::FString* slotName)
	{
		auto result = DeleteSaveFuncHook(a_this, resultOut, slotName);

		auto saveName = std::format("{}", *slotName);
		Manager::GetSingleton()->DeleteDatabase(saveName);

		return result;
	}
	static inline REL::Hook DeleteSaveFuncHook{ REL::Offset(0x478D840), 0x133, DeleteSaveFunc };
};

// on data loaded
struct Hook_DataLoaded
{
	static void MainLoopFunc(std::uintptr_t a_ptr)
	{
		MainLoopFuncHook(a_ptr);

		static std::once_flag once;
		std::call_once(once, [&]() {
			Manager::GetSingleton()->ReadForms();
			Manager::GetSingleton()->CleanupDatabase();
		});
	}

	static inline REL::Hook MainLoopFuncHook{ REL::Offset(0x65FB250), 0x12CB, MainLoopFunc };
};

void MessageHandler(OBSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case OBSE::MessagingInterface::kPostLoad:
		Manager::GetSingleton()->ReadSettings();
		break;
	default:
		break;
	}
}

OBSE_EXPORT constinit auto OBSEPlugin_Version = []() noexcept {
	OBSE::PluginVersionData v{};
	v.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH, 0 });
	v.PluginName("UnreadBooksGlow");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(false);
	v.UsesSigScanning(false);
	v.IsLayoutDependent(true);
	v.HasNoStructUse(false);
	v.CompatibleVersions({ OBSE::RUNTIME_LATEST });
	return v;
}();

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse, { .logName = Version::PROJECT.data(), .logPattern = "[%T.%e] [%L] %v", .trampoline = true });
	OBSE::GetMessagingInterface()->RegisterListener(MessageHandler);
	return true;
}
