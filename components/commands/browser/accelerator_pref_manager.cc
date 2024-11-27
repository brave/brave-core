// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/browser/accelerator_pref_manager.h"

#include <string>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
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
    PrefService* prefs,
    const base::flat_set<int>& command_ids) {
  AcceleratorPrefManager::Accelerators result;

  const auto& accelerators = prefs->GetDict(pref);
  for (const auto [command_id, shortcuts] : accelerators) {
    int id;
    CHECK(base::StringToInt(command_id, &id))
        << "Failed to parse " << command_id << " as int";

    if (!base::Contains(command_ids, id)) {
      DVLOG(1) << "Found non-existent command_id " << id
               << ". Maybe it has been removed?";
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

AcceleratorPrefManager::AcceleratorPrefManager(
    PrefService* prefs,
    base::span<const int> available_command_ids)
    : prefs_(prefs),
      available_command_ids_(base::MakeFlatSet<int>(available_command_ids)) {
  DCHECK(prefs_);
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

  // If the value is already in the list, erase it.
  list->EraseIf([&accelerator_string](const base::Value& value) {
    auto* string_value = value.GetIfString();
    return string_value && *string_value == accelerator_string;
  });

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
  return GetAcceleratorsFromPref(kAcceleratorsPrefs, prefs_,
                                 available_command_ids_);
}

AcceleratorPrefManager::Accelerators
AcceleratorPrefManager::GetDefaultAccelerators() {
  return GetAcceleratorsFromPref(kDefaultAcceleratorsPrefs, prefs_,
                                 available_command_ids_);
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

}  // namespace commands
