/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_bulk_data_handler.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/webui/settings/import_feature.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace settings {

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
  Profile* profile =
      profile_manager->GetProfileByPath(profile_manager->user_data_dir().Append(
          base::FilePath::FromASCII(base::UTF16ToUTF8(name))));

  if (profile) {
    std::move(profile_ready_callback).Run(profile);
    return;
  }

  auto avatar_index = base::RandInt(profiles::GetModernAvatarIconStartIndex(),
                                    profiles::GetDefaultAvatarIconCount());
  ProfileManager::CreateMultiProfileAsync(
      name, avatar_index, false,
      base::BindOnce(
          [](ProfileReadyCallback initialized_callback, Profile* profile) {
            CHECK(profile);
            // Migrate welcome page flag to new profiles.
            profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);
            std::move(initialized_callback).Run(profile);
          },
          std::move(profile_ready_callback)));
}

void BraveImportBulkDataHandler::ProfileReadyForImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items,
    Profile* profile) {
#if BUILDFLAG(IS_MAC)
  CheckDiskAccess(source_profile, imported_items, profile);
#else
  StartImportImpl(source_profile, imported_items, profile);
#endif
}

void BraveImportBulkDataHandler::HandleImportDataBulk(
    const base::Value::List& args) {
  CHECK_GE(args.size(), 2u);
  const auto& list = args[0].GetList();
  // Bulk profiles import assumes new profiles will be created on our side if
  // they do not exist.
  const base::Value& types = args[1];
  for (const auto& it : list) {
    int browser_index = it.GetInt();
    importing_profiles_.insert(browser_index);
    base::Value::List single_profile_args;
    single_profile_args.Append(base::Value(browser_index));
    single_profile_args.Append(types.Clone());
    BraveImportDataHandler::HandleImportData(single_profile_args);
  }
}

absl::optional<int> BraveImportBulkDataHandler::GetProfileIndex(
    const importer::SourceProfile& source_profile) {
  for (auto index : importing_profiles_) {
    const auto& profile = GetSourceProfileAt(index);
    if (profile.source_path == source_profile.source_path) {
      return index;
    }
  }
  return absl::nullopt;
}

void BraveImportBulkDataHandler::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  // If profile is not from the bulk import request fallback to single profile
  // import.
  if (!GetProfileIndex(source_profile).has_value()) {
    BraveImportDataHandler::StartImport(source_profile, imported_items);
    return;
  }
  if (!imported_items)
    return;
  PrepareProfile(
      source_profile.profile.empty() ? source_profile.importer_name
                                     : source_profile.profile,
      base::BindOnce(&BraveImportBulkDataHandler::ProfileReadyForImport,
                     weak_factory_.GetWeakPtr(), source_profile,
                     imported_items));
}

void BraveImportBulkDataHandler::NotifyImportProgress(
    const importer::SourceProfile& source_profile,
    const base::Value& info) {
  BraveImportDataHandler::NotifyImportProgress(source_profile, info);

  const std::string* event = info.FindStringKey("event");
  if (!event || *event != "ImportEnded")
    return;
  auto index = GetProfileIndex(source_profile);
  if (index.has_value()) {
    importing_profiles_.erase(index.value());
  }
}

}  // namespace settings
