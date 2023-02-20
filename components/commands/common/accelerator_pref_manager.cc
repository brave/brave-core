// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/common/accelerator_pref_manager.h"

#include <vector>

#include "base/containers/flat_map.h"
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

void AcceleratorPrefManager::AddAccelerator(
    int command_id,
    const ui::Accelerator& accelerator) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  auto serialized = ToCodesString(accelerator);
  if (serialized.empty()) {
    DCHECK(false) << "Failed to serialize shortcut Modifier: "
                  << accelerator.modifiers()
                  << ", Keycode: " << accelerator.key_code();
    return;
  }
  list->Append(serialized);
}

void AcceleratorPrefManager::RemoveAccelerator(
    int command_id,
    const ui::Accelerator& accelerator) {
  ScopedDictPrefUpdate update(prefs_, kAcceleratorsPrefs);
  auto accelerator_as_string = ToCodesString(accelerator);
  auto* list = update->EnsureList(base::NumberToString(command_id));
  list->EraseIf([accelerator_as_string](const base::Value& value) {
    auto* string_value = value.GetIfString();
    return string_value && *string_value == accelerator_as_string;
  });
}

base::flat_map<int, std::vector<ui::Accelerator>>
AcceleratorPrefManager::GetAccelerators() {
  base::flat_map<int, std::vector<ui::Accelerator>> result;

  const auto& accelerators = prefs_->GetDict(kAcceleratorsPrefs);
  for (const auto [command_id, shortcuts] : accelerators) {
    int id;
    DCHECK(base::StringToInt(command_id, &id));
    for (const auto& shortcut : shortcuts.GetList()) {
      auto accelerator = FromCodesString(shortcut.GetString());
      result[id].push_back(accelerator);
    }
  }

  return result;
}

}  // namespace commands
