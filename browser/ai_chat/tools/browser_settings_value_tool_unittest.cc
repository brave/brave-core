// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/browser_settings_value_tool.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/tools/browser_settings_registry.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

// Returns the entry for `setting_id` from the "settings" list, or nullptr.
const base::DictValue* FindSetting(const base::DictValue& result,
                                   std::string_view setting_id) {
  const base::ListValue* settings = result.FindList("settings");
  if (!settings) {
    return nullptr;
  }
  for (const auto& value : *settings) {
    if (value.is_dict() && value.GetDict().FindString("id") &&
        *value.GetDict().FindString("id") == setting_id) {
      return &value.GetDict();
    }
  }
  return nullptr;
}

mojom::ToolUseEventPtr MakeToolUseEvent(const std::string& arguments_json) {
  auto event = mojom::ToolUseEvent::New();
  event->tool_name = mojom::kBrowserSettingValueToolName;
  event->id = "tool-use-id";
  event->arguments_json = arguments_json;
  return event;
}

}  // namespace

class BrowserSettingsValueToolTest : public testing::Test {
 protected:
  void SetUp() override {
    tool_ = std::make_unique<BrowserSettingsValueTool>(profile_.GetPrefs());
  }

  base::DictValue RunTool(const std::string& json) {
    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                           std::vector<mojom::ToolArtifactPtr>>
        future;
    tool_->UseTool(json, future.GetCallback());
    text_output_ =
        ExtractText(future.Get<std::vector<mojom::ContentBlockPtr>>());
    auto dict = base::JSONReader::ReadDict(text_output_, base::JSON_PARSE_RFC);
    return dict ? std::move(*dict) : base::DictValue();
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  std::unique_ptr<BrowserSettingsValueTool> tool_;
  std::string text_output_;
};

TEST_F(BrowserSettingsValueToolTest, ReturnsCurrentValue) {
  profile_.GetPrefs()->SetBoolean("brave.de_amp.enabled", false);

  auto result = RunTool(R"({"setting_ids": ["brave.de_amp.enabled"]})");
  const base::DictValue* entry = FindSetting(result, "brave.de_amp.enabled");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->FindBool("value"), false);
}

// The assistant needs to distinguish "the user turned this off" from "this
// just ships off", otherwise it will describe defaults as deliberate choices.
TEST_F(BrowserSettingsValueToolTest, ReportsWhetherValueIsStillTheDefault) {
  auto result = RunTool(R"({"setting_ids": ["brave.de_amp.enabled"]})");
  const base::DictValue* entry = FindSetting(result, "brave.de_amp.enabled");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->FindBool("is_default"), true);
  EXPECT_EQ(entry->FindBool("is_managed"), false);

  profile_.GetPrefs()->SetBoolean("brave.de_amp.enabled", false);

  result = RunTool(R"({"setting_ids": ["brave.de_amp.enabled"]})");
  entry = FindSetting(result, "brave.de_amp.enabled");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->FindBool("is_default"), false);
}

TEST_F(BrowserSettingsValueToolTest, ReportsPolicyManagedValues) {
  profile_.GetTestingPrefService()->SetManagedPref("brave.de_amp.enabled",
                                                   base::Value(false));

  auto result = RunTool(R"({"setting_ids": ["brave.de_amp.enabled"]})");
  const base::DictValue* entry = FindSetting(result, "brave.de_amp.enabled");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->FindBool("value"), false);
  EXPECT_EQ(entry->FindBool("is_managed"), true);
}

// Integer preferences are internal enums and the registry carries no labels
// for them, so the raw number is reported as-is. The tool description tells
// the model not to invent a meaning for it.
TEST_F(BrowserSettingsValueToolTest, ReturnsRawValueForIntegerEnums) {
  profile_.GetPrefs()->SetInteger("brave.tabs.hover_mode", 2);

  auto result = RunTool(R"({"setting_ids": ["brave.tabs.hover_mode"]})");
  const base::DictValue* entry = FindSetting(result, "brave.tabs.hover_mode");
  ASSERT_TRUE(entry);
  EXPECT_EQ(entry->FindInt("value"), 2);
}

// Defence in depth against an allowlist mistake: structured values are refused
// at read time even if something dictionary- or list-valued gets allowlisted.
TEST_F(BrowserSettingsValueToolTest, RefusesNonScalarValues) {
  for (std::string_view pref_name : browser_settings::GetAllowedPrefs()) {
    const PrefService::Preference* pref =
        profile_.GetPrefs()->FindPreference(pref_name);
    ASSERT_TRUE(pref) << pref_name;
    ASSERT_FALSE(pref->GetValue()->is_dict()) << pref_name;
    ASSERT_FALSE(pref->GetValue()->is_list()) << pref_name;
  }
}

TEST_F(BrowserSettingsValueToolTest, ReadsMultipleSettingsInOneCall) {
  auto result = RunTool(
      R"({"setting_ids": ["brave.de_amp.enabled", "brave.ai_chat.user_memory_enabled"]})");
  EXPECT_TRUE(FindSetting(result, "brave.de_amp.enabled"));
  EXPECT_TRUE(FindSetting(result, "brave.ai_chat.user_memory_enabled"));
  EXPECT_FALSE(result.FindList("errors"));
}

// Ids come straight from an LLM, so unknown ones are expected rather than
// exceptional. They must not prevent the valid ids from being answered.
TEST_F(BrowserSettingsValueToolTest, ReportsUnknownIdsWithoutFailingTheCall) {
  auto result = RunTool(
      R"({"setting_ids": ["brave.de_amp.enabled", "made.up.setting"]})");
  EXPECT_TRUE(FindSetting(result, "brave.de_amp.enabled"));

  const base::ListValue* errors = result.FindList("errors");
  ASSERT_TRUE(errors);
  ASSERT_EQ(errors->size(), 1u);
  EXPECT_EQ(*(*errors)[0].GetDict().FindString("id"), "made.up.setting");
  EXPECT_TRUE((*errors)[0].GetDict().FindString("error"));
}

// The allowlist is the security boundary, so a real but non-allowlisted
// preference must be refused rather than read.
TEST_F(BrowserSettingsValueToolTest, RefusesRegisteredButDisallowedPrefs) {
  ASSERT_TRUE(
      profile_.GetPrefs()->FindPreference("download.default_directory"));

  auto result = RunTool(R"({"setting_ids": ["download.default_directory"]})");
  EXPECT_TRUE(result.FindList("settings")->empty());
  const base::ListValue* errors = result.FindList("errors");
  ASSERT_TRUE(errors);
  ASSERT_EQ(errors->size(), 1u);
  EXPECT_EQ(*(*errors)[0].GetDict().FindString("id"),
            "download.default_directory");
}

TEST_F(BrowserSettingsValueToolTest, DeduplicatesRequestedIds) {
  auto result = RunTool(
      R"({"setting_ids": ["brave.de_amp.enabled", "brave.de_amp.enabled"]})");
  EXPECT_EQ(result.FindList("settings")->size(), 1u);
}

TEST_F(BrowserSettingsValueToolTest, CapsTheNumberOfSettingsPerCall) {
  base::ListValue ids;
  for (std::string_view pref_name : browser_settings::GetAllowedPrefs()) {
    ids.Append(pref_name);
  }
  ASSERT_GT(ids.size(), 10u);
  base::DictValue input;
  input.Set("setting_ids", std::move(ids));
  std::string input_json;
  ASSERT_TRUE(base::JSONWriter::Write(input, &input_json));

  auto result = RunTool(input_json);
  EXPECT_EQ(result.FindList("settings")->size(), 10u);
}

TEST_F(BrowserSettingsValueToolTest, IgnoresNonStringIds) {
  auto result =
      RunTool(R"({"setting_ids": [42, "brave.de_amp.enabled", null]})");
  ASSERT_EQ(result.FindList("settings")->size(), 1u);
  EXPECT_TRUE(FindSetting(result, "brave.de_amp.enabled"));
}

TEST_F(BrowserSettingsValueToolTest, RejectsMalformedInput) {
  RunTool("not json");
  EXPECT_THAT(text_output_, testing::StartsWith("Error:"));
  RunTool("{}");
  EXPECT_THAT(text_output_, testing::StartsWith("Error:"));
  RunTool(R"({"setting_ids": []})");
  EXPECT_THAT(text_output_, testing::StartsWith("Error:"));
  RunTool(R"({"setting_ids": "brave.de_amp.enabled"})");
  EXPECT_THAT(text_output_, testing::StartsWith("Error:"));
  RunTool(R"({"setting_ids": [42]})");
  EXPECT_THAT(text_output_, testing::StartsWith("Error:"));
}

// Unlike the search tool, approval must not persist: every read of the user's
// configuration is approved on its own.
TEST_F(BrowserSettingsValueToolTest, AsksForPermissionOnEveryCall) {
  auto event = MakeToolUseEvent(R"({"setting_ids": ["brave.de_amp.enabled"]})");
  for (int i = 0; i < 3; ++i) {
    auto interaction = tool_->RequiresUserInteractionBeforeHandling(*event);
    ASSERT_TRUE(
        std::holds_alternative<mojom::PermissionChallengePtr>(interaction));
    EXPECT_TRUE(std::get<mojom::PermissionChallengePtr>(interaction));
    // Granting permission for one use must not carry over to the next.
    tool_->UserPermissionGranted("tool-use-id");
  }
}

}  // namespace ai_chat
