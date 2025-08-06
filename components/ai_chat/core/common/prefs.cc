/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/prefs.h"

#include <utility>

#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ai_chat::prefs {

mojom::CustomizationsPtr GetCustomizationsFromPrefs(const PrefService& prefs) {
  const base::Value::Dict& customizations_dict =
      prefs.GetDict(prefs::kBraveAIChatUserCustomizations);

  auto get_string_or_empty =
      [&customizations_dict](const std::string& key) -> std::string {
    auto* string_value = customizations_dict.FindString(key);
    return string_value ? *string_value : "";
  };

  return mojom::Customizations::New(
      get_string_or_empty("name"), get_string_or_empty("job"),
      get_string_or_empty("tone"), get_string_or_empty("other"));
}

void SetCustomizationsToPrefs(const mojom::CustomizationsPtr& customizations,
                              PrefService& prefs) {
  auto dict = base::Value::Dict()
                  .Set("name", customizations->name)
                  .Set("job", customizations->job)
                  .Set("tone", customizations->tone)
                  .Set("other", customizations->other);
  prefs.SetDict(prefs::kBraveAIChatUserCustomizations, std::move(dict));
}

void ResetCustomizationsPref(PrefService& prefs) {
  prefs.ClearPref(prefs::kBraveAIChatUserCustomizations);
}

std::vector<std::string> GetMemoriesFromPrefs(const PrefService& prefs) {
  const base::Value::List& memories_list =
      prefs.GetList(prefs::kBraveAIChatUserMemories);
  std::vector<std::string> memories;
  for (const auto& memory : memories_list) {
    memories.push_back(memory.GetString());
  }
  return memories;
}

void AddMemoryToPrefs(const std::string& memory, PrefService& prefs) {
  ScopedListPrefUpdate update(&prefs, prefs::kBraveAIChatUserMemories);
  for (auto& item : *update) {
    if (item.is_string() && item.GetString() == memory) {
      // Silently ignore duplicate items.
      return;
    }
  }
  update->Append(memory);
}

bool UpdateMemoryInPrefs(const std::string& old_memory,
                         const std::string& new_memory,
                         PrefService& prefs) {
  ScopedListPrefUpdate update(&prefs, prefs::kBraveAIChatUserMemories);
  for (auto& item : *update) {
    if (item.is_string() && item.GetString() == old_memory) {
      item = base::Value(new_memory);
      return true;
    }
  }
  return false;
}

void DeleteMemoryFromPrefs(const std::string& memory, PrefService& prefs) {
  ScopedListPrefUpdate update(&prefs, prefs::kBraveAIChatUserMemories);
  update->EraseValue(base::Value(memory));
}

void DeleteAllMemoriesFromPrefs(PrefService& prefs) {
  prefs.ClearPref(prefs::kBraveAIChatUserMemories);
}

std::optional<base::Value::Dict> GetUserMemoryDictFromPrefs(
    PrefService& prefs) {
  bool customization_enabled =
      prefs.GetBoolean(prefs::kBraveAIChatUserCustomizationEnabled);
  bool memory_enabled = prefs.GetBoolean(prefs::kBraveAIChatUserMemoryEnabled);
  if (!customization_enabled && !memory_enabled) {
    return std::nullopt;
  }

  base::Value::Dict user_memory;
  if (customization_enabled) {
    const base::Value::Dict& customizations_dict =
        prefs.GetDict(prefs::kBraveAIChatUserCustomizations);

    // Only set values when they have actual content
    if (const std::string* name = customizations_dict.FindString("name")) {
      if (!name->empty()) {
        user_memory.Set("name", *name);
      }
    }
    if (const std::string* job = customizations_dict.FindString("job")) {
      if (!job->empty()) {
        user_memory.Set("job", *job);
      }
    }
    if (const std::string* tone = customizations_dict.FindString("tone")) {
      if (!tone->empty()) {
        user_memory.Set("tone", *tone);
      }
    }
    if (const std::string* other = customizations_dict.FindString("other")) {
      if (!other->empty()) {
        user_memory.Set("other", *other);
      }
    }
  }

  if (memory_enabled) {
    const base::Value::List& memories =
        prefs.GetList(prefs::kBraveAIChatUserMemories);
    if (!memories.empty()) {
      user_memory.Set("memories", memories.Clone());
    }
  }

  if (user_memory.empty()) {
    return std::nullopt;
  }

  return user_memory;
}

}  // namespace ai_chat::prefs
