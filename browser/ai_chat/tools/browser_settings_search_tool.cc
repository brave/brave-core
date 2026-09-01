// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/browser_settings_search_tool.h"

#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/browser_settings_registry.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

namespace {

constexpr char kPropertyQuery[] = "query";
constexpr char kPropertyCount[] = "count";

constexpr char kOutputKeyQuery[] = "query";
constexpr char kOutputKeyResults[] = "results";

constexpr int kDefaultResultCount = 10;
constexpr int kMaxResultCount = 40;

// Description() must return a view onto storage that outlives the call, so
// the companion tool's name is spelled out in the literal below rather than
// concatenated at runtime. Keep the two in sync.
static_assert(std::string_view(mojom::kBrowserSettingValueToolName) ==
              "browser_setting_value");

}  // namespace

namespace internal {

std::string BuildSettingsSearchResultJson(
    const std::string& query,
    const std::vector<browser_settings::SearchMatch>& matches) {
  // A flat list of preference paths: the path is the only thing there is to
  // report, and it is what the value tool expects back.
  base::ListValue results;
  for (const auto& match : matches) {
    results.Append(match.pref_name);
  }

  base::DictValue root;
  root.Set(kOutputKeyQuery, query);
  root.Set(kOutputKeyResults, std::move(results));

  std::string json;
  base::JSONWriter::Write(root, &json);
  return json;
}

}  // namespace internal

BrowserSettingsSearchTool::BrowserSettingsSearchTool() = default;

BrowserSettingsSearchTool::~BrowserSettingsSearchTool() = default;

std::string_view BrowserSettingsSearchTool::Name() const {
  return mojom::kBrowserSettingsSearchToolName;
}

std::string_view BrowserSettingsSearchTool::Description() const {
  return "Searches the Brave browser settings the assistant is allowed to "
         "read and returns their preference paths, e.g. "
         "\"browser.clear_data.cookies_on_exit\". Use when the user asks how "
         "their browser is configured, or asks you to explain or troubleshoot "
         "behaviour that a setting could account for. Returns paths only -- "
         "it does NOT return the user's current values; pass the paths to the "
         "'browser_setting_value' tool for that. Interpret each path from its "
         "own wording, and treat it as authoritative over any assumption "
         "about what Brave settings exist. Not every setting is covered; if "
         "nothing relevant comes back, say so rather than guessing.";
}

std::optional<base::DictValue> BrowserSettingsSearchTool::InputProperties()
    const {
  return CreateInputProperties(
      {{kPropertyQuery,
        StringProperty("Natural language description of the setting to look "
                       "for, e.g. \"clear cookies when I close the "
                       "browser\". Matching is on the words in the "
                       "preference path, so prefer the terms Brave itself "
                       "uses over marketing names.")},
       {kPropertyCount,
        IntegerProperty("Maximum number of settings to return. Defaults to "
                        "10; capped at 40.")}});
}

std::optional<std::vector<std::string>>
BrowserSettingsSearchTool::RequiredProperties() const {
  return std::vector<std::string>{kPropertyQuery};
}

std::variant<bool, mojom::PermissionChallengePtr>
BrowserSettingsSearchTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  // The results are the same static catalogue for every user and reveal
  // nothing about them, so one grant covers the whole conversation. The
  // user-facing wording lives in `get_tool_permission_implications.tsx` so
  // that it goes through i18n; this side only has to return a non-null
  // challenge.
  return mojom::PermissionChallenge::New(/*assessment=*/std::nullopt,
                                         /*plan=*/std::nullopt);
}

void BrowserSettingsSearchTool::UserPermissionGranted(
    const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void BrowserSettingsSearchTool::UseTool(const std::string& input_json,
                                        UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"), {});
    return;
  }

  const std::string* query = input->FindString(kPropertyQuery);
  if (!query || query->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing or empty 'query' field"),
        {});
    return;
  }

  int count = kDefaultResultCount;
  if (auto requested = input->FindInt(kPropertyCount)) {
    count = std::clamp(*requested, 1, kMaxResultCount);
  }

  std::move(callback).Run(
      CreateContentBlocksForText(internal::BuildSettingsSearchResultJson(
          *query,
          browser_settings::SearchPrefs(*query, static_cast<size_t>(count)))),
      {});
}

}  // namespace ai_chat
