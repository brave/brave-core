/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_site_settings_handler.h"

#include <string>

#include "base/values.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/webui/settings/site_settings_helper.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace settings {

namespace {
constexpr char kIsValidKey[] = "isValid";
constexpr char kReasonKey[] = "reason";
}  // namespace

BraveSiteSettingsHandler::BraveSiteSettingsHandler(Profile* profile)
    : SiteSettingsHandler(profile) {}

BraveSiteSettingsHandler::~BraveSiteSettingsHandler() = default;

void BraveSiteSettingsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "isPatternValidForType",
      base::BindRepeating(
          &BraveSiteSettingsHandler::HandleIsPatternValidForType,
          base::Unretained(this)));

  SiteSettingsHandler::RegisterMessages();
}

bool BraveSiteSettingsHandler::IsPatternValidForBraveContentType(
    ContentSettingsType content_type,
    const std::string& pattern_string) {
  if (content_type != ContentSettingsType::BRAVE_SHIELDS) {
    return true;
  }
  return (pattern_string.find('*') == std::string::npos);
}

void BraveSiteSettingsHandler::HandleIsPatternValidForType(
    const base::Value::List& args) {
  CHECK_EQ(3U, args.size());
  const base::Value& callback_id = args[0];
  const std::string& pattern_string = args[1].GetString();
  const std::string& type = args[2].GetString();

  ContentSettingsType content_type =
      site_settings::ContentSettingsTypeFromGroupName(type);

  if (!IsPatternValidForBraveContentType(content_type, pattern_string)) {
    base::Value::Dict return_value;
    return_value.Set(kIsValidKey, base::Value(false));
    return_value.Set(kReasonKey, base::Value(l10n_util::GetStringUTF8(
                                     IDS_BRAVE_SHIELDS_NOT_VALID_ADDRESS)));
    AllowJavascript();
    ResolveJavascriptCallback(callback_id, return_value);
    return;
  }

  SiteSettingsHandler::HandleIsPatternValidForType(args);
}

void BraveSiteSettingsHandler::RemoveNonModelData(
    const std::vector<url::Origin>& origins) {
  SiteSettingsHandler::RemoveNonModelData(origins);

  auto* settings_map = HostContentSettingsMapFactory::GetForProfile(profile_);
  for (const auto& origin : origins) {
    const auto& url = origin.GetURL();
    // base::Value() is a default value which removes the setting internally.
    settings_map->SetWebsiteSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA, base::Value());
  }
}

}  // namespace settings
