// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/browser_settings_value_tool.h"

#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/browser_settings_registry.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

constexpr char kPropertySettingIds[] = "setting_ids";

constexpr char kOutputKeySettings[] = "settings";
constexpr char kOutputKeyErrors[] = "errors";
constexpr char kOutputKeyId[] = "id";
constexpr char kOutputKeyValue[] = "value";
constexpr char kOutputKeyIsDefault[] = "is_default";
constexpr char kOutputKeyIsManaged[] = "is_managed";
constexpr char kOutputKeyError[] = "error";

// Keeps the permission prompt legible and stops a single approval from
// covering a sweep of the user's configuration.
constexpr size_t kMaxSettingsPerCall = 10;

}  // namespace

namespace internal {

std::string BuildSettingsValueJson(const std::vector<std::string>& setting_ids,
                                   const PrefService& prefs) {
  base::ListValue settings;
  base::ListValue errors;

  for (const auto& setting_id : setting_ids) {
    base::DictValue error;
    // The allowlist is the security boundary: anything outside it is refused
    // before PrefService is ever consulted.
    if (!browser_settings::IsAllowedPref(setting_id)) {
      error.Set(kOutputKeyId, setting_id);
      error.Set(kOutputKeyError,
                "Unknown or disallowed setting. Use browser_settings_search "
                "to find readable settings.");
      errors.Append(std::move(error));
      continue;
    }

    // The allowlist is compiled in, so a preference can be absent at runtime
    // if it is registered behind a feature or platform condition that isn't
    // met on this build. Report that rather than reporting a wrong value.
    const PrefService::Preference* pref = prefs.FindPreference(setting_id);
    if (!pref) {
      error.Set(kOutputKeyId, setting_id);
      error.Set(kOutputKeyError,
                "This setting is not available in this build or on this "
                "platform.");
      errors.Append(std::move(error));
      continue;
    }

    // Defence in depth against an allowlist mistake: dictionaries and lists
    // hold structured user data (site lists, URLs, device records) rather than
    // a setting the user picked, so never emit them even if allowlisted.
    const base::Value& value = *pref->GetValue();
    if (value.is_dict() || value.is_list()) {
      error.Set(kOutputKeyId, setting_id);
      error.Set(kOutputKeyError, "This setting is not a simple value.");
      errors.Append(std::move(error));
      continue;
    }

    base::DictValue result;
    result.Set(kOutputKeyId, setting_id);
    result.Set(kOutputKeyValue, value.Clone());
    // Lets the assistant distinguish "the user chose this" from "this is just
    // the shipped default", and flag settings locked by enterprise policy.
    result.Set(kOutputKeyIsDefault, pref->IsDefaultValue());
    result.Set(kOutputKeyIsManaged, pref->IsManaged());
    settings.Append(std::move(result));
  }

  base::DictValue root;
  root.Set(kOutputKeySettings, std::move(settings));
  if (!errors.empty()) {
    root.Set(kOutputKeyErrors, std::move(errors));
  }

  std::string json;
  base::JSONWriter::Write(root, &json);
  return json;
}

}  // namespace internal

BrowserSettingsValueTool::BrowserSettingsValueTool(PrefService* prefs)
    : prefs_(prefs) {}

BrowserSettingsValueTool::~BrowserSettingsValueTool() = default;

std::string_view BrowserSettingsValueTool::Name() const {
  return mojom::kBrowserSettingValueToolName;
}

std::string_view BrowserSettingsValueTool::Description() const {
  return "Reads the user's current value for one or more Brave browser "
         "settings, identified by the preference paths returned by the "
         "'browser_settings_search' tool -- do not invent paths. Returns each "
         "value along with whether it is still the shipped default and "
         "whether it is locked by enterprise policy. Values are raw "
         "preference values, so a number may be an internal enum whose "
         "meaning is not given; say what the value is rather than guessing at "
         "a label for it. The user is asked to approve every call, so request "
         "all the settings you need in a single call rather than one at a "
         "time.";
}

std::optional<base::DictValue> BrowserSettingsValueTool::InputProperties()
    const {
  return CreateInputProperties(
      {{kPropertySettingIds,
        ArrayProperty(
            "Preference paths to read, as returned by "
            "browser_settings_search. At most 10 per call.",
            StringProperty(
                "A preference path, e.g. \"brave.de_amp.enabled\"."))}});
}

std::optional<std::vector<std::string>>
BrowserSettingsValueTool::RequiredProperties() const {
  return std::vector<std::string>{kPropertySettingIds};
}

std::variant<bool, mojom::PermissionChallengePtr>
BrowserSettingsValueTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  // Intentionally unconditional: the value of a setting is information about
  // the user, so each read is approved on its own. ConversationHandler clears
  // the challenge on the event once granted, so this does not loop.
  //
  // The prompt lists the requested ids via the tool label in
  // `get_tool_label.ts` (localized there), so no `plan` is needed here.
  return mojom::PermissionChallenge::New(/*assessment=*/std::nullopt,
                                         /*plan=*/std::nullopt);
}

void BrowserSettingsValueTool::UseTool(const std::string& input_json,
                                       UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"), {});
    return;
  }

  const base::ListValue* requested = input->FindList(kPropertySettingIds);
  if (!requested || requested->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
                                "Error: missing or empty 'setting_ids' field"),
                            {});
    return;
  }

  std::vector<std::string> setting_ids;
  for (const auto& value : *requested) {
    if (!value.is_string()) {
      continue;
    }
    // De-duplicate so a repeated id doesn't consume the per-call budget.
    if (!std::ranges::contains(setting_ids, value.GetString())) {
      setting_ids.push_back(value.GetString());
    }
    if (setting_ids.size() == kMaxSettingsPerCall) {
      break;
    }
  }

  if (setting_ids.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: 'setting_ids' must contain at least one string"),
        {});
    return;
  }

  std::move(callback).Run(
      CreateContentBlocksForText(
          internal::BuildSettingsValueJson(setting_ids, *prefs_)),
      {});
}

}  // namespace ai_chat
