/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREFS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREFS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom-forward.h"

class PrefService;

namespace ai_chat::prefs {

// Customizations and user memories prefs
// Returns the customizations from the customizations dictionary in the pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
mojom::CustomizationsPtr GetCustomizationsFromPrefs(const PrefService& prefs);
// Sets the customizations to the customizations dictionary in the pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void SetCustomizationsToPrefs(const mojom::CustomizationsPtr& customizations,
                              PrefService& prefs);
// Reset the customizations pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void ResetCustomizationsPref(PrefService& prefs);

// Returns the memories from the memories list in the pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
std::vector<std::string> GetMemoriesFromPrefs(const PrefService& prefs);
// Appends the memory to the memories list in the pref. Silently ignore
// duplicate items.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void AddMemoryToPrefs(const std::string& memory, PrefService& prefs);
// Updates a memory in the memories list in the pref. Returns true if the memory
// is updated, false if the memory is not found.
COMPONENT_EXPORT(AI_CHAT_COMMON)
bool UpdateMemoryInPrefs(const std::string& old_memory,
                         const std::string& new_memory,
                         PrefService& prefs);
// Deletes a memory from the memories list in the pref. Silently ignore if the
// memory is not found.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void DeleteMemoryFromPrefs(const std::string& memory, PrefService& prefs);
// Checks if a specific memory exists in the memories list in the pref.
// Returns false if memory feature is disabled.
COMPONENT_EXPORT(AI_CHAT_COMMON)
bool HasMemoryFromPrefs(const std::string& memory, const PrefService& prefs);
// Resets the memories list in the pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void DeleteAllMemoriesFromPrefs(PrefService& prefs);

COMPONENT_EXPORT(AI_CHAT_COMMON)
std::optional<base::Value::Dict> GetUserMemoryDictFromPrefs(PrefService& prefs);

// Smart Modes prefs
// Returns smart modes from the smart modes dictionary in the pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
std::vector<mojom::SmartModePtr> GetSmartModesFromPrefs(
    const PrefService& prefs);
// Returns a specific smart mode by ID, or nullptr if not found.
COMPONENT_EXPORT(AI_CHAT_COMMON)
mojom::SmartModePtr GetSmartModeFromPrefs(const PrefService& prefs,
                                          const std::string& id);

// Adds a new smart mode and saves it to prefs.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void AddSmartModeToPrefs(const std::string& shortcut,
                         const std::string& prompt,
                         const std::optional<std::string>& model,
                         PrefService& prefs);
// Updates an existing smart mode in prefs.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void UpdateSmartModeInPrefs(const std::string& id,
                            const std::string& shortcut,
                            const std::string& prompt,
                            const std::optional<std::string>& model,
                            PrefService& prefs);
// Deletes a smart mode from prefs.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void DeleteSmartModeFromPrefs(const std::string& id, PrefService& prefs);

}  // namespace ai_chat::prefs

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREFS_H_
