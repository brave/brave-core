// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/prefs.h"

#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::prefs {

class AIChatPrefsTest : public ::testing::Test {
 public:
  void SetUp() override { RegisterProfilePrefs(pref_service_.registry()); }

  TestingPrefServiceSimple pref_service_;
};

TEST_F(AIChatPrefsTest, GetCustomizationsFromPrefs_EmptyPref) {
  auto customizations = GetCustomizationsFromPrefs(pref_service_);

  EXPECT_EQ(customizations->name, "");
  EXPECT_EQ(customizations->job, "");
  EXPECT_EQ(customizations->tone, "");
  EXPECT_EQ(customizations->other, "");
}

TEST_F(AIChatPrefsTest, GetCustomizationsFromPrefs_WithData) {
  // Set up test data
  auto dict = base::Value::Dict()
                  .Set("name", "John Doe")
                  .Set("job", "Software Engineer")
                  .Set("tone", "Professional")
                  .Set("other", "Loves coding");
  pref_service_.SetDict(kBraveAIChatUserCustomizations, std::move(dict));

  auto customizations = GetCustomizationsFromPrefs(pref_service_);

  EXPECT_EQ(customizations->name, "John Doe");
  EXPECT_EQ(customizations->job, "Software Engineer");
  EXPECT_EQ(customizations->tone, "Professional");
  EXPECT_EQ(customizations->other, "Loves coding");
}

TEST_F(AIChatPrefsTest, GetCustomizationsFromPrefs_PartialData) {
  // Set up test data with only some fields
  auto dict =
      base::Value::Dict().Set("name", "Jane Smith").Set("job", "Designer");
  pref_service_.SetDict(kBraveAIChatUserCustomizations, std::move(dict));

  auto customizations = GetCustomizationsFromPrefs(pref_service_);

  EXPECT_EQ(customizations->name, "Jane Smith");
  EXPECT_EQ(customizations->job, "Designer");
  EXPECT_EQ(customizations->tone, "");
  EXPECT_EQ(customizations->other, "");
}

TEST_F(AIChatPrefsTest, SetCustomizationsToPrefs) {
  auto customizations = mojom::Customizations::New(
      "Alice Johnson", "Product Manager", "Friendly", "Enjoys hiking");

  SetCustomizationsToPrefs(customizations, pref_service_);

  const base::Value::Dict& stored_dict =
      pref_service_.GetDict(kBraveAIChatUserCustomizations);
  EXPECT_EQ(*stored_dict.FindString("name"), "Alice Johnson");
  EXPECT_EQ(*stored_dict.FindString("job"), "Product Manager");
  EXPECT_EQ(*stored_dict.FindString("tone"), "Friendly");
  EXPECT_EQ(*stored_dict.FindString("other"), "Enjoys hiking");
}

TEST_F(AIChatPrefsTest, SetCustomizationsToPrefs_EmptyValues) {
  auto customizations = mojom::Customizations::New("", "", "", "");

  SetCustomizationsToPrefs(customizations, pref_service_);

  const base::Value::Dict& stored_dict =
      pref_service_.GetDict(kBraveAIChatUserCustomizations);
  EXPECT_EQ(*stored_dict.FindString("name"), "");
  EXPECT_EQ(*stored_dict.FindString("job"), "");
  EXPECT_EQ(*stored_dict.FindString("tone"), "");
  EXPECT_EQ(*stored_dict.FindString("other"), "");
}

TEST_F(AIChatPrefsTest, GetMemoriesFromPrefs_EmptyPref) {
  auto memories = GetMemoriesFromPrefs(pref_service_);

  EXPECT_TRUE(memories.empty());
}

TEST_F(AIChatPrefsTest, GetMemoriesFromPrefs_WithData) {
  // Set up test data
  auto list = base::Value::List();
  list.Append("I work as a software engineer");
  list.Append("I prefer dark mode");
  list.Append("I use Brave browser");
  pref_service_.SetList(kBraveAIChatUserMemories, std::move(list));

  auto memories = GetMemoriesFromPrefs(pref_service_);

  ASSERT_EQ(memories.size(), 3u);
  EXPECT_EQ(memories[0], "I work as a software engineer");
  EXPECT_EQ(memories[1], "I prefer dark mode");
  EXPECT_EQ(memories[2], "I use Brave browser");
}

TEST_F(AIChatPrefsTest, AddMemoryToPrefs_NewMemory) {
  AddMemoryToPrefs("I love coding", pref_service_);
  AddMemoryToPrefs("I love coding2", pref_service_);

  auto memories = GetMemoriesFromPrefs(pref_service_);
  ASSERT_EQ(memories.size(), 2u);
  EXPECT_EQ(memories[0], "I love coding");
  EXPECT_EQ(memories[1], "I love coding2");
}

TEST_F(AIChatPrefsTest, AddMemoryToPrefs_DuplicateMemory) {
  // Add the same memory twice
  AddMemoryToPrefs("I love coding", pref_service_);
  AddMemoryToPrefs("I love coding", pref_service_);

  auto memories = GetMemoriesFromPrefs(pref_service_);
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0], "I love coding");
}

TEST_F(AIChatPrefsTest, UpdateMemoryInPrefs_Success) {
  // Set up initial memories
  AddMemoryToPrefs("Old memory", pref_service_);
  AddMemoryToPrefs("Another memory", pref_service_);

  bool result =
      UpdateMemoryInPrefs("Old memory", "Updated memory", pref_service_);

  EXPECT_TRUE(result);
  auto memories = GetMemoriesFromPrefs(pref_service_);
  ASSERT_EQ(memories.size(), 2u);
  EXPECT_EQ(memories[0], "Updated memory");
  EXPECT_EQ(memories[1], "Another memory");
}

TEST_F(AIChatPrefsTest, UpdateMemoryInPrefs_NotFound) {
  // Set up initial memories
  AddMemoryToPrefs("Existing memory", pref_service_);

  bool result =
      UpdateMemoryInPrefs("Non-existent memory", "New memory", pref_service_);

  EXPECT_FALSE(result);
  auto memories = GetMemoriesFromPrefs(pref_service_);
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0], "Existing memory");
}

TEST_F(AIChatPrefsTest, DeleteMemoryFromPrefs_Success) {
  // Set up initial memories
  AddMemoryToPrefs("Memory to delete", pref_service_);
  AddMemoryToPrefs("Memory to keep", pref_service_);

  DeleteMemoryFromPrefs("Memory to delete", pref_service_);

  auto memories = GetMemoriesFromPrefs(pref_service_);
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0], "Memory to keep");
}

TEST_F(AIChatPrefsTest, DeleteMemoryFromPrefs_NotFound) {
  // Set up initial memories
  AddMemoryToPrefs("Existing memory", pref_service_);

  DeleteMemoryFromPrefs("Non-existent memory", pref_service_);

  auto memories = GetMemoriesFromPrefs(pref_service_);
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0], "Existing memory");
}

TEST_F(AIChatPrefsTest, DeleteAllMemoriesFromPrefs) {
  // Set up initial memories
  AddMemoryToPrefs("First memory", pref_service_);
  AddMemoryToPrefs("Second memory", pref_service_);
  AddMemoryToPrefs("Third memory", pref_service_);

  DeleteAllMemoriesFromPrefs(pref_service_);

  auto memories = GetMemoriesFromPrefs(pref_service_);
  EXPECT_TRUE(memories.empty());
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_BothDisabled) {
  // Both customization and memory features are disabled
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, false);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, false);

  // Set up customization data
  auto customizations_dict = base::Value::Dict()
                                 .Set("name", "John Doe")
                                 .Set("job", "Software Engineer")
                                 .Set("tone", "Professional")
                                 .Set("other", "Loves coding");
  pref_service_.SetDict(kBraveAIChatUserCustomizations,
                        std::move(customizations_dict));

  // Set up memory data
  AddMemoryToPrefs("I work as a software engineer", pref_service_);

  auto result = GetUserMemoryDictFromPrefs(pref_service_);

  EXPECT_FALSE(result.has_value());
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_CustomizationOnly) {
  // Only customization is enabled
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, true);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, false);

  // Set up customization data
  auto customizations_dict = base::Value::Dict()
                                 .Set("name", "John Doe")
                                 .Set("job", "Software Engineer")
                                 .Set("tone", "Professional")
                                 .Set("other", "Loves coding");
  pref_service_.SetDict(kBraveAIChatUserCustomizations,
                        std::move(customizations_dict));

  // Set up memory data
  AddMemoryToPrefs("I work as a software engineer", pref_service_);

  auto result = GetUserMemoryDictFromPrefs(pref_service_);

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result->FindString("name"), "John Doe");
  EXPECT_EQ(*result->FindString("job"), "Software Engineer");
  EXPECT_EQ(*result->FindString("tone"), "Professional");
  EXPECT_EQ(*result->FindString("other"), "Loves coding");
  EXPECT_FALSE(result->Find("memories"));
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_MemoryOnly) {
  // Only memory is enabled
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, false);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, true);

  // Set up customization data
  auto customizations_dict = base::Value::Dict()
                                 .Set("name", "John Doe")
                                 .Set("job", "Software Engineer")
                                 .Set("tone", "Professional")
                                 .Set("other", "Loves coding");
  pref_service_.SetDict(kBraveAIChatUserCustomizations,
                        std::move(customizations_dict));

  // Set up memory data
  auto memories_list = base::Value::List();
  memories_list.Append("I work as a software engineer");
  memories_list.Append("I prefer dark mode");
  pref_service_.SetList(kBraveAIChatUserMemories, std::move(memories_list));

  auto result = GetUserMemoryDictFromPrefs(pref_service_);

  EXPECT_TRUE(result.has_value());
  EXPECT_FALSE(result->Find("name"));
  EXPECT_FALSE(result->Find("job"));
  EXPECT_FALSE(result->Find("tone"));
  EXPECT_FALSE(result->Find("other"));

  const base::Value::List* memories = result->FindList("memories");
  EXPECT_TRUE(memories);
  EXPECT_EQ(memories->size(), 2u);
  EXPECT_EQ((*memories)[0].GetString(), "I work as a software engineer");
  EXPECT_EQ((*memories)[1].GetString(), "I prefer dark mode");
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_BothEnabled) {
  // Both customization and memory are enabled
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, true);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, true);

  // Empty prefs should not have any value
  auto result = GetUserMemoryDictFromPrefs(pref_service_);
  EXPECT_FALSE(result.has_value());

  // Set up customization data
  auto customizations_dict = base::Value::Dict()
                                 .Set("name", "Jane Smith")
                                 .Set("job", "Designer")
                                 .Set("tone", "Friendly")
                                 .Set("other", "Enjoys art");
  pref_service_.SetDict(kBraveAIChatUserCustomizations,
                        std::move(customizations_dict));

  // Set up memory data
  AddMemoryToPrefs("I love creating beautiful designs", pref_service_);
  AddMemoryToPrefs("I use Brave browser daily", pref_service_);

  result = GetUserMemoryDictFromPrefs(pref_service_);

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result->FindString("name"), "Jane Smith");
  EXPECT_EQ(*result->FindString("job"), "Designer");
  EXPECT_EQ(*result->FindString("tone"), "Friendly");
  EXPECT_EQ(*result->FindString("other"), "Enjoys art");

  const base::Value::List* memories = result->FindList("memories");
  ASSERT_TRUE(memories);
  ASSERT_EQ(memories->size(), 2u);
  EXPECT_EQ((*memories)[0].GetString(), "I love creating beautiful designs");
  EXPECT_EQ((*memories)[1].GetString(), "I use Brave browser daily");
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_EmptyCustomizations) {
  // Customization enabled but with empty values
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, true);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, false);

  // Set up empty customization data
  auto customizations_dict = base::Value::Dict()
                                 .Set("name", "")
                                 .Set("job", "")
                                 .Set("tone", "")
                                 .Set("other", "");
  pref_service_.SetDict(kBraveAIChatUserCustomizations,
                        std::move(customizations_dict));

  auto result = GetUserMemoryDictFromPrefs(pref_service_);

  EXPECT_FALSE(result.has_value());
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_PartialCustomizations) {
  // Customization enabled but with only some fields filled
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, true);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, false);

  // Set up partial customization data
  auto customizations_dict = base::Value::Dict()
                                 .Set("name", "Alice")
                                 .Set("job", "")
                                 .Set("tone", "Casual")
                                 .Set("other", "");
  pref_service_.SetDict(kBraveAIChatUserCustomizations,
                        std::move(customizations_dict));

  auto result = GetUserMemoryDictFromPrefs(pref_service_);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result->FindString("name"), "Alice");
  EXPECT_EQ(*result->FindString("tone"), "Casual");
  EXPECT_FALSE(result->Find("job"));
  EXPECT_FALSE(result->Find("other"));
}

TEST_F(AIChatPrefsTest, GetUserMemoryDictFromPrefs_EmptyMemories) {
  // Memory enabled but with empty list
  pref_service_.SetBoolean(kBraveAIChatUserCustomizationEnabled, false);
  pref_service_.SetBoolean(kBraveAIChatUserMemoryEnabled, true);

  // Set up empty memory data
  auto memories_list = base::Value::List();
  pref_service_.SetList(kBraveAIChatUserMemories, std::move(memories_list));

  auto result = GetUserMemoryDictFromPrefs(pref_service_);

  EXPECT_FALSE(result.has_value());
}

TEST_F(AIChatPrefsTest, HasMemoryFromPrefs) {
  // Test with empty prefs
  EXPECT_FALSE(HasMemoryFromPrefs("Any memory", pref_service_));

  // Set up test data
  auto list = base::Value::List();
  list.Append("I work as a software engineer");
  list.Append("I live in San Francisco");
  list.Append("I use Brave browser");
  pref_service_.SetList(kBraveAIChatUserMemories, std::move(list));

  // Test existing memory
  EXPECT_TRUE(HasMemoryFromPrefs("I live in San Francisco", pref_service_));

  // Test non-existing memory
  EXPECT_FALSE(HasMemoryFromPrefs("I work in New York", pref_service_));
}

}  // namespace ai_chat::prefs
