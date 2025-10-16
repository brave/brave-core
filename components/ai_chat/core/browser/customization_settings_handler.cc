// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/customization_settings_handler.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace ai_chat {

// Validates if a string is not empty and within the maximum length to be stored
// as customization or memory.
bool IsValidMemoryLength(const std::string& value) {
  return !value.empty() && value.size() <= mojom::kMaxMemoryRecordLength;
}

CustomizationSettingsHandler::CustomizationSettingsHandler(PrefService* prefs)
    : prefs_(prefs) {
  DCHECK(prefs_);
  pref_change_registrar_.Init(prefs_);

  // Watch for changes to customization and memory preferences
  pref_change_registrar_.Add(
      prefs::kBraveAIChatUserCustomizations,
      base::BindRepeating(
          &CustomizationSettingsHandler::OnCustomizationsChanged,
          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveAIChatUserMemories,
      base::BindRepeating(&CustomizationSettingsHandler::OnMemoriesChanged,
                          base::Unretained(this)));
}

CustomizationSettingsHandler::~CustomizationSettingsHandler() = default;

void CustomizationSettingsHandler::BindUI(
    mojo::PendingRemote<mojom::CustomizationSettingsUI> ui) {
  DCHECK(!ui_);
  ui_.Bind(std::move(ui));
}

void CustomizationSettingsHandler::GetCustomizations(
    GetCustomizationsCallback callback) {
  std::move(callback).Run(prefs::GetCustomizationsFromPrefs(*prefs_));
}

void CustomizationSettingsHandler::SetCustomizations(
    mojom::CustomizationsPtr customizations,
    SetCustomizationsCallback callback) {
  // Check each field has less than max record length.
  if (customizations->name.length() > mojom::kMaxMemoryRecordLength ||
      customizations->job.length() > mojom::kMaxMemoryRecordLength ||
      customizations->tone.length() > mojom::kMaxMemoryRecordLength ||
      customizations->other.length() > mojom::kMaxMemoryRecordLength) {
    std::move(callback).Run(mojom::CustomizationOperationError::kInvalidLength);
    return;
  }

  prefs::SetCustomizationsToPrefs(customizations, *prefs_);
  std::move(callback).Run(std::nullopt);
}

void CustomizationSettingsHandler::AddMemory(const std::string& memory,
                                             AddMemoryCallback callback) {
  if (!IsValidMemoryLength(memory)) {
    std::move(callback).Run(mojom::CustomizationOperationError::kInvalidLength);
    return;
  }

  prefs::AddMemoryToPrefs(memory, *prefs_);
  std::move(callback).Run(std::nullopt);
}

void CustomizationSettingsHandler::EditMemory(const std::string& old_memory,
                                              const std::string& new_memory,
                                              EditMemoryCallback callback) {
  if (!IsValidMemoryLength(new_memory)) {
    std::move(callback).Run(mojom::CustomizationOperationError::kInvalidLength);
    return;
  }

  if (prefs::UpdateMemoryInPrefs(old_memory, new_memory, *prefs_)) {
    std::move(callback).Run(std::nullopt);
  } else {
    std::move(callback).Run(mojom::CustomizationOperationError::kNotFound);
  }
}

void CustomizationSettingsHandler::GetMemories(GetMemoriesCallback callback) {
  std::move(callback).Run(prefs::GetMemoriesFromPrefs(*prefs_));
}

void CustomizationSettingsHandler::DeleteMemory(const std::string& memory) {
  prefs::DeleteMemoryFromPrefs(memory, *prefs_);
}

void CustomizationSettingsHandler::DeleteAllMemories() {
  prefs::DeleteAllMemoriesFromPrefs(*prefs_);
}

void CustomizationSettingsHandler::OnCustomizationsChanged() {
  if (!ui_) {
    return;
  }

  ui_->OnCustomizationsChanged(prefs::GetCustomizationsFromPrefs(*prefs_));
}

void CustomizationSettingsHandler::OnMemoriesChanged() {
  if (!ui_) {
    return;
  }

  ui_->OnMemoriesChanged(prefs::GetMemoriesFromPrefs(*prefs_));
}

}  // namespace ai_chat
