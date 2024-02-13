/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_bulk_data_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace settings {

namespace {
base::FilePath GetProfilePathByName(const std::u16string& name) {
  std::vector<ProfileAttributesEntry*> entries =
      g_browser_process->profile_manager()
          ->GetProfileAttributesStorage()
          .GetAllProfilesAttributesSortedByNameWithCheck();
  for (auto* it : entries) {
    if (it->GetName() == name) {
      return it->GetPath();
    }
  }
  return base::FilePath();
}
}  // namespace

BraveImportBulkDataHandler::BraveImportBulkDataHandler() = default;
BraveImportBulkDataHandler::~BraveImportBulkDataHandler() = default;

void BraveImportBulkDataHandler::RegisterMessages() {
  BraveImportDataHandler::RegisterMessages();
  web_ui()->RegisterMessageCallback(
      "importDataBulk",
      base::BindRepeating(&BraveImportBulkDataHandler::HandleImportDataBulk,
                          base::Unretained(this)));
}

void BraveImportBulkDataHandler::PrepareProfile(
    const std::u16string& name,
    ProfileReadyCallback profile_ready_callback) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  auto profile_path = GetProfilePathByName(name);
  // If a profile with the given path is currently managed by this object and
  // fully initialized, return a pointer to the corresponding Profile object.
  Profile* loaded_profile = profile_manager->GetProfileByPath(profile_path);
  if (loaded_profile) {
    std::move(profile_ready_callback).Run(loaded_profile);
    return;
  }
  // Asynchronously loads an existing profile given its |profile_base_name|
  // (which is the directory name within the user data directory), optionally in
  // |incognito| mode. The |callback| will be called with the Profile when it
  // has been loaded, or with a nullptr otherwise.
  profile_manager->LoadProfileByPath(
      profile_path, false,
      base::BindOnce(
          [](ProfileReadyCallback profile_callback, const std::u16string& name,
             Profile* existing_profile) {
            // Existing profile loaded, reuse it.
            if (existing_profile) {
              std::move(profile_callback).Run(existing_profile);
              return;
            }
            // Asynchronously creates a new profile in the next available
            // multiprofile directory. Directories are named "profile_1",
            // "profile_2", etc., in sequence of creation.
            DCHECK_LT(profiles::GetModernAvatarIconStartIndex(),
                      profiles::GetDefaultAvatarIconCount());
            auto avatar_index =
                base::RandInt(profiles::GetModernAvatarIconStartIndex(),
                              profiles::GetDefaultAvatarIconCount() - 1);
            ProfileManager::CreateMultiProfileAsync(
                name, avatar_index, false,
                base::BindOnce(
                    [](ProfileReadyCallback initialized_callback,
                       Profile* created_profile) {
                      CHECK(created_profile);
                      // Migrate welcome page flag to new profiles.
                      created_profile->GetPrefs()->SetBoolean(
                          prefs::kHasSeenWelcomePage, true);
                      std::move(initialized_callback).Run(created_profile);
                    },
                    std::move(profile_callback)));
          },
          std::move(profile_ready_callback), name));
}

void BraveImportBulkDataHandler::HandleImportDataBulk(
    const base::Value::List& args) {
  CHECK_GE(args.size(), 2u);
  const auto& list = args[0].GetList();
  // Bulk profiles import assumes new profiles will be created on our side if
  // they do not exist.
  for (const auto& it : list) {
    int browser_index = it.GetInt();
    importing_profiles_.insert(browser_index);
    base::Value::List single_profile_args;
    single_profile_args.Append(browser_index);
    single_profile_args.Append(args[1].Clone());
    BraveImportDataHandler::HandleImportData(single_profile_args);
  }
}

std::optional<int> BraveImportBulkDataHandler::GetProfileIndex(
    const importer::SourceProfile& source_profile) {
  for (auto index : importing_profiles_) {
    const auto& profile = GetSourceProfileAt(index);
    if (profile.source_path == source_profile.source_path) {
      return index;
    }
  }
  return std::nullopt;
}

void BraveImportBulkDataHandler::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  if (!imported_items)
    return;
  // If profile is not from the bulk import request fallback to single profile
  // import.
  if (!GetProfileIndex(source_profile).has_value()) {
    BraveImportDataHandler::StartImport(source_profile, imported_items);
    return;
  }
  auto profile_name = source_profile.profile.empty()
                          ? source_profile.importer_name
                          : source_profile.profile;
  auto import_callback = base::BindOnce(
      &BraveImportBulkDataHandler::StartImportImpl, weak_factory_.GetWeakPtr(),
      source_profile, imported_items);
#if BUILDFLAG(IS_MAC)
  CheckDiskAccess(imported_items, source_profile.source_path,
                  source_profile.importer_type,
                  base::BindOnce(&BraveImportBulkDataHandler::PrepareProfile,
                                 weak_factory_.GetWeakPtr(), profile_name,
                                 std::move(import_callback)));
#else
  PrepareProfile(profile_name, std::move(import_callback));
#endif
}

void BraveImportBulkDataHandler::NotifyImportProgress(
    const importer::SourceProfile& source_profile,
    const base::Value::Dict& info) {
  FireWebUIListener("brave-import-data-status-changed", info);
}

void BraveImportBulkDataHandler::OnImportEnded(
    const importer::SourceProfile& source_profile) {
  auto index = GetProfileIndex(source_profile);
  if (index.has_value()) {
    importing_profiles_.erase(index.value());
    return;
  }
  BraveImportDataHandler::OnImportEnded(source_profile);
}

}  // namespace settings
