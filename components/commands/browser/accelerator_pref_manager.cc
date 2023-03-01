// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/browser/accelerator_pref_manager.h"

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace commands {
namespace {
// A dictionary of command_id: [ "shortcut" ]
// for example:
// { 1: [ "Control+KeyC", "Control+KeySpace" ] }
constexpr char kAcceleratorsPrefs[] = "brave.accelerators";
}  // namespace

// static
void AcceleratorPrefManager::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kAcceleratorsPrefs);
}

AcceleratorPrefManager::AcceleratorPrefManager(PrefService* prefs)
    : prefs_(prefs) {}

AcceleratorPrefManager::~AcceleratorPrefManager() = default;

void AcceleratorPrefManager::ClearAccelerators(int command_id) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  list->clear();
}

void AcceleratorPrefManager::AddAccelerator(int command_id,
                                            const std::string& accelerator) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  list->Append(accelerator);
}

void AcceleratorPrefManager::RemoveAccelerator(int command_id,
                                               const std::string& accelerator) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  list->EraseIf([accelerator](const base::Value& value) {
    auto* string_value = value.GetIfString();
    return string_value && *string_value == accelerator;
  });
}

base::flat_map<int, std::vector<std::string>>
AcceleratorPrefManager::GetAccelerators() {
  base::flat_map<int, std::vector<std::string>> result;

  const auto& accelerators = prefs_->GetDict(kAcceleratorsPrefs);
  for (const auto [command_id, shortcuts] : accelerators) {
    int id;
    if (!base::StringToInt(command_id, &id)) {
      DCHECK(false) << "Failed to parse " << command_id << " as int";
      continue;
    }

    for (const auto& accelerator : shortcuts.GetList()) {
      result[id].push_back(accelerator.GetString());
    }
  }

  return result;
}

}  // namespace commands
