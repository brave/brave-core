// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/browser/accelerator_pref_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {
namespace {

// A dictionary of command_id: [ "shortcut" ]
// for example:
// { 1: [ "Control+KeyC", "Control+KeySpace" ] }
constexpr char kAcceleratorsPrefs[] = "brave.accelerators";
constexpr char kDefaultAcceleratorsPrefs[] = "brave.default_accelerators";

AcceleratorPrefManager::Accelerators GetAcceleratorsFromPref(
    const std::string& pref,
    PrefService* prefs) {
  AcceleratorPrefManager::Accelerators result;

  const auto& accelerators = prefs->GetDict(pref);
  for (const auto [command_id, shortcuts] : accelerators) {
    int id;
    if (!base::StringToInt(command_id, &id)) {
      DCHECK(false) << "Failed to parse " << command_id << " as int";
      continue;
    }

    for (const auto& accelerator : shortcuts.GetList()) {
      result[id].push_back(FromCodesString(accelerator.GetString()));
    }
  }

  return result;
}

}  // namespace

// static
void AcceleratorPrefManager::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kAcceleratorsPrefs);
  registry->RegisterDictionaryPref(kDefaultAcceleratorsPrefs);
}

AcceleratorPrefManager::AcceleratorPrefManager(PrefService* prefs)
    : prefs_(prefs) {
  DCHECK(prefs_);

#if BUILDFLAG(IS_MAC)
  MigrateMetaKeyToCommandKey();
#endif
}

AcceleratorPrefManager::~AcceleratorPrefManager() = default;

void AcceleratorPrefManager::ClearAccelerators(int command_id) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  list->clear();
}

void AcceleratorPrefManager::AddAccelerator(
    int command_id,
    const ui::Accelerator& accelerator) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  auto accelerator_string = ToCodesString(accelerator);
  DCHECK(!accelerator_string.empty());
  list->Append(accelerator_string);
}

void AcceleratorPrefManager::RemoveAccelerator(
    int command_id,
    const ui::Accelerator& accelerator) {
  auto accelerator_string = ToCodesString(accelerator);
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  list->EraseIf([&accelerator_string](const base::Value& value) {
    auto* string_value = value.GetIfString();
    return string_value && *string_value == accelerator_string;
  });
}

AcceleratorPrefManager::Accelerators AcceleratorPrefManager::GetAccelerators() {
  return GetAcceleratorsFromPref(kAcceleratorsPrefs, prefs_);
}

AcceleratorPrefManager::Accelerators
AcceleratorPrefManager::GetDefaultAccelerators() {
  return GetAcceleratorsFromPref(kDefaultAcceleratorsPrefs, prefs_);
}

void AcceleratorPrefManager::SetDefaultAccelerators(
    const Accelerators& default_accelerators) {
  ScopedDictPrefUpdate defaults_update(prefs_, kDefaultAcceleratorsPrefs);
  defaults_update->clear();
  for (const auto& [command, accelerators] : default_accelerators) {
    auto* items = defaults_update->EnsureList(base::NumberToString(command));
    for (const auto& accelerator : accelerators) {
      items->Append(ToCodesString(accelerator));
    }
  }
}

#if BUILDFLAG(IS_MAC)
void AcceleratorPrefManager::MigrateMetaKeyToCommandKey() {
  for (const auto* pref_key : {kAcceleratorsPrefs, kDefaultAcceleratorsPrefs}) {
    if (!prefs_->FindPreference(pref_key)) {
      CHECK_IS_TEST();
      continue;
    }

    ScopedDictPrefUpdate pref_update(prefs_, pref_key);

    base::ranges::for_each(
        *pref_update, [&pref_update](const auto& id_and_list_value) {
          std::string command_id;
          std::tie(command_id, std::ignore) = id_and_list_value;

          auto* accelerators = pref_update->EnsureList(command_id);
          DCHECK(accelerators);
          base::ranges::for_each(*accelerators, [](auto& accelerator_value) {
            const auto& encoded_accelerator = accelerator_value.GetString();
            if (!base::Contains(encoded_accelerator, "Meta")) {
              return;
            }

            DVLOG(2) << "Found 'Meta' key from the pref. As there's no Meta "
                        "key on Mac, we will migrate it to 'Command' key: "
                     << encoded_accelerator;

            // Find 'Meta'
            std::vector<std::string> parts =
                base::SplitString(encoded_accelerator, "+",
                                  base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
            auto iter = base::ranges::find(parts, "Meta");
            DCHECK(iter != parts.end());

            // Replace it with 'Command'
            *iter = "Command";

            // Replace the pref value
            auto new_value = base::Value(base::JoinString(parts, "+"));
            std::swap(accelerator_value, new_value);

#if DCHECK_IS_ON()
            // Post invariant check
            DCHECK(base::Contains(new_value.GetString(), "Meta"));
            DCHECK(!base::Contains(accelerator_value.GetString(), "Meta"));
#endif
          });
        });
  }
}
#endif

}  // namespace commands
