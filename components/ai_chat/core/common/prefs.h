/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREFS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREFS_H_

#include <string>
#include <vector>

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
// Resets the memories list in the pref.
COMPONENT_EXPORT(AI_CHAT_COMMON)
void DeleteAllMemoriesFromPrefs(PrefService& prefs);

}  // namespace ai_chat::prefs

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREFS_H_
