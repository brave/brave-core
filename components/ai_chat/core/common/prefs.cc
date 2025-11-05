/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/prefs.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ai_chat::prefs {

namespace {

// Helper function to convert a dictionary to Skill
mojom::SkillPtr SkillDictToStruct(const std::string& id,
                                  const base::Value::Dict& skill_dict) {
  const std::string* shortcut = skill_dict.FindString("shortcut");
  const std::string* prompt = skill_dict.FindString("prompt");
  const std::string* model = skill_dict.FindString("model");

  if (!shortcut || !prompt) {
    return nullptr;
  }

  // Parse timestamps using base::ValueToTime
  base::Time created_time, last_used;
  if (const base::Value* created_time_value = skill_dict.Find("created_time")) {
    auto parsed_time = base::ValueToTime(*created_time_value);
    if (parsed_time) {
      created_time = *parsed_time;
    }
  }
  if (const base::Value* last_used_value = skill_dict.Find("last_used")) {
    auto parsed_time = base::ValueToTime(*last_used_value);
    if (parsed_time) {
      last_used = *parsed_time;
    }
  }

  return mojom::Skill::New(id, *shortcut, *prompt,
                           model ? std::make_optional(*model) : std::nullopt,
                           created_time, last_used);
}

// Helper function to convert Skill to dictionary
base::Value::Dict SkillStructToDict(const mojom::SkillPtr& skill) {
  base::Value::Dict skill_dict;
  skill_dict.Set("shortcut", skill->shortcut);
  skill_dict.Set("prompt", skill->prompt);
  if (skill->model) {
    skill_dict.Set("model", *skill->model);
  }
  skill_dict.Set("created_time", base::TimeToValue(skill->created_time));
  skill_dict.Set("last_used", base::TimeToValue(skill->last_used));
  return skill_dict;
}

bool IsValidAndUniqueShortcut(const PrefService& prefs,
                              const std::string& shortcut,
                              const std::string& exclude_id) {
  // Check format validation
  if (shortcut.empty()) {
    return false;
  }

  if (!std::all_of(shortcut.begin(), shortcut.end(), [](char c) -> bool {
        return base::IsAsciiAlphaNumeric(c) || c == '_' || c == '-';
      })) {
    return false;
  }

  // Check for duplicates
  const base::Value::Dict& skills_dict =
      prefs.GetDict(prefs::kBraveAIChatSkills);

  for (const auto [id, skill_value] : skills_dict) {
    if (!skill_value.is_dict()) {
      continue;
    }

    // Skip if this is the same ID we're updating
    if (!exclude_id.empty() && id == exclude_id) {
      continue;
    }

    const std::string* shortcut_str =
        skill_value.GetDict().FindString("shortcut");
    if (shortcut_str && shortcut == *shortcut_str) {
      return false;  // Duplicate found
    }
  }

  return true;  // Valid and unique
}

}  // namespace

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

bool HasMemoryFromPrefs(const std::string& memory, const PrefService& prefs) {
  const base::Value::List& memories_list =
      prefs.GetList(prefs::kBraveAIChatUserMemories);
  for (const auto& item : memories_list) {
    if (item.is_string() && item.GetString() == memory) {
      return true;
    }
  }
  return false;
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

std::vector<mojom::SkillPtr> GetSkillsFromPrefs(const PrefService& prefs) {
  const base::Value::Dict& skills_dict =
      prefs.GetDict(prefs::kBraveAIChatSkills);

  std::vector<mojom::SkillPtr> skills;

  for (const auto [id, skill_value] : skills_dict) {
    if (!skill_value.is_dict()) {
      continue;
    }

    auto skill = SkillDictToStruct(id, skill_value.GetDict());
    if (skill) {
      skills.push_back(std::move(skill));
    }
  }

  return skills;
}

mojom::SkillPtr GetSkillFromPrefs(const PrefService& prefs,
                                  const std::string& id) {
  const base::Value::Dict& skills_dict =
      prefs.GetDict(prefs::kBraveAIChatSkills);

  const base::Value* skill_value = skills_dict.Find(id);
  if (!skill_value || !skill_value->is_dict()) {
    return nullptr;
  }

  return SkillDictToStruct(id, skill_value->GetDict());
}

void AddSkillToPrefs(const std::string& shortcut,
                     const std::string& prompt,
                     const std::optional<std::string>& model,
                     PrefService& prefs) {
  if (prompt.empty() || !IsValidAndUniqueShortcut(prefs, shortcut, "")) {
    return;
  }

  std::string id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  base::Time now = base::Time::Now();

  auto skill = mojom::Skill::New(id, shortcut, prompt, model, now, now);

  ScopedDictPrefUpdate update(&prefs, prefs::kBraveAIChatSkills);
  base::Value::Dict& skills_dict = update.Get();

  skills_dict.Set(id, SkillStructToDict(skill));
}

void UpdateSkillInPrefs(const std::string& id,
                        const std::string& shortcut,
                        const std::string& prompt,
                        const std::optional<std::string>& model,
                        PrefService& prefs) {
  if (prompt.empty() || !IsValidAndUniqueShortcut(prefs, shortcut, id)) {
    return;
  }

  ScopedDictPrefUpdate update(&prefs, prefs::kBraveAIChatSkills);
  base::Value::Dict& skills_dict = update.Get();

  base::Value::Dict* skill_dict = skills_dict.FindDict(id);
  if (!skill_dict) {
    return;
  }

  skill_dict->Set("shortcut", shortcut);
  skill_dict->Set("prompt", prompt);
  if (model.has_value()) {
    skill_dict->Set("model", *model);
  } else {
    skill_dict->Remove("model");
  }
}

void DeleteSkillFromPrefs(const std::string& id, PrefService& prefs) {
  ScopedDictPrefUpdate update(&prefs, prefs::kBraveAIChatSkills);
  base::Value::Dict& skills_dict = update.Get();
  skills_dict.Remove(id);
}

void UpdateSkillLastUsedInPrefs(const std::string& id, PrefService& prefs) {
  ScopedDictPrefUpdate update(&prefs, prefs::kBraveAIChatSkills);
  base::Value::Dict* skill_dict = update->FindDict(id);
  if (!skill_dict) {
    return;
  }

  skill_dict->Set("last_used", base::TimeToValue(base::Time::Now()));
}

}  // namespace ai_chat::prefs
