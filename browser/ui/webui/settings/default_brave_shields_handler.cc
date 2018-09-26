/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"

void DefaultBraveShieldsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "getAdControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetAdControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setAdControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetAdControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getCookieControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetCookieControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setCookieControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetCookieControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getFingerprintingControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetFingerprintingControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setFingerprintingControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetFingerprintingControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setHTTPSEverywhereControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetHTTPSEverywhereControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setNoScriptControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetNoScriptControlType,
                          base::Unretained(this)));
}

void DefaultBraveShieldsHandler::GetAdControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile_)->GetContentSetting(
          GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kAds);

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(setting == CONTENT_SETTING_ALLOW ? "allow" : "block"));
}

void DefaultBraveShieldsHandler::SetAdControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  std::string value;
  args->GetString(0, &value);

  HostContentSettingsMapFactory::GetForProfile(profile_)->
      SetContentSettingCustomScope(
        ContentSettingsPattern::Wildcard(),
        ContentSettingsPattern::Wildcard(),
        CONTENT_SETTINGS_TYPE_PLUGINS,
        brave_shields::kAds,
        value == "allow" ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
}

void DefaultBraveShieldsHandler::GetCookieControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile_)->GetContentSetting(
          GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  ContentSetting fp_setting =
      HostContentSettingsMapFactory::GetForProfile(profile_)->GetContentSetting(
          GURL(), GURL("https://firstParty/"), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);

  std::string value = "block";
  if (setting == CONTENT_SETTING_ALLOW) {
    value = "allow";
  } else if (fp_setting != CONTENT_SETTING_BLOCK) {
    value = "3p";
  }

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(value));
}

void DefaultBraveShieldsHandler::SetCookieControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  std::string value;
  args->GetString(0, &value);

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kReferrers,
      value == "allow" ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);

  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies,
      value == "allow" ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);

  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies,
      value == "block" ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);
}

void DefaultBraveShieldsHandler::GetFingerprintingControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile_)->GetContentSetting(
          GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  ContentSetting fp_setting =
      HostContentSettingsMapFactory::GetForProfile(profile_)->GetContentSetting(
          GURL(), GURL("https://firstParty/"), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);

  std::string value;
  if (setting != fp_setting) {
    value = "3p";
  } else {
    value = setting == CONTENT_SETTING_ALLOW ? "allow" : "block";
  }

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(value));
}

void DefaultBraveShieldsHandler::SetFingerprintingControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  std::string value;
  args->GetString(0, &value);

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kFingerprinting,
      value == "allow" ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);

  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kFingerprinting,
      value == "block" ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);
}

void DefaultBraveShieldsHandler::SetHTTPSEverywhereControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  HostContentSettingsMapFactory::GetForProfile(profile_)->
      SetContentSettingCustomScope(
        ContentSettingsPattern::Wildcard(),
        ContentSettingsPattern::Wildcard(),
        CONTENT_SETTINGS_TYPE_PLUGINS,
        brave_shields::kHTTPUpgradableResources,
        value ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);
}

void DefaultBraveShieldsHandler::SetNoScriptControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  HostContentSettingsMapFactory::GetForProfile(profile_)->
      SetContentSettingCustomScope(
        ContentSettingsPattern::Wildcard(),
        ContentSettingsPattern::Wildcard(),
        CONTENT_SETTINGS_TYPE_JAVASCRIPT,
        "",
        value ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);
}
