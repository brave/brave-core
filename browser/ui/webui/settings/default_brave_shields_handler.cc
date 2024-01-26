/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"

#include <string>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/web_ui.h"
#include "url/gurl.h"

using brave_shields::ControlType;
using brave_shields::ControlTypeFromString;
using brave_shields::ControlTypeToString;

DefaultBraveShieldsHandler::DefaultBraveShieldsHandler() = default;
DefaultBraveShieldsHandler::~DefaultBraveShieldsHandler() = default;

void DefaultBraveShieldsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "isAdControlEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::IsAdControlEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setAdControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetAdControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isFirstPartyCosmeticFilteringEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::IsFirstPartyCosmeticFilteringEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setCosmeticFilteringControlType",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetCosmeticFilteringControlType,
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
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetFingerprintingControlType,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setFingerprintingControlType",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetFingerprintingControlType,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getFingerprintingBlockEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetFingerprintingBlockEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setFingerprintingBlockEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetFingerprintingBlockEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getHttpsUpgradeControlType",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetHttpsUpgradeControlType,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setHttpsUpgradeControlType",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetHttpsUpgradeControlType,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setNoScriptControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetNoScriptControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getForgetFirstPartyStorageEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetForgetFirstPartyStorageEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setForgetFirstPartyStorageEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetForgetFirstPartyStorageEnabled,
          base::Unretained(this)));

  content_settings_observation_.Observe(
      HostContentSettingsMapFactory::GetForProfile(profile_));
  cookie_settings_observation_.Observe(
      CookieSettingsFactory::GetForProfile(profile_).get());
}

void DefaultBraveShieldsHandler::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsTypeSet content_type_set) {
  if (!content_type_set.Contains(ContentSettingsType::COOKIES) &&
      !content_type_set.Contains(
          ContentSettingsType::BRAVE_COSMETIC_FILTERING) &&
      !content_type_set.Contains(ContentSettingsType::BRAVE_TRACKERS) &&
      !content_type_set.Contains(
          ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES) &&
      !content_type_set.Contains(
          ContentSettingsType::BRAVE_FINGERPRINTING_V2) &&
      !content_type_set.Contains(ContentSettingsType::BRAVE_SHIELDS) &&
      !content_type_set.Contains(ContentSettingsType::BRAVE_HTTPS_UPGRADE) &&
      !content_type_set.Contains(
          ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE)) {
    return;
  }

  if (primary_pattern != ContentSettingsPattern::Wildcard() &&
      secondary_pattern != ContentSettingsPattern::Wildcard()) {
    return;
  }

  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener("brave-shields-settings-changed");
}

void DefaultBraveShieldsHandler::OnThirdPartyCookieBlockingChanged(
    bool block_third_party_cookies) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener("brave-shields-settings-changed");
}

void DefaultBraveShieldsHandler::IsAdControlEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetAdControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(),
                            base::Value(setting == ControlType::BLOCK));
}

void DefaultBraveShieldsHandler::SetAdControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  bool value = args[0].GetBool();

  brave_shields::SetAdControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      value ? ControlType::BLOCK : ControlType::ALLOW, GURL(),
      g_browser_process->local_state());
}

void DefaultBraveShieldsHandler::IsFirstPartyCosmeticFilteringEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  bool enabled = brave_shields::IsFirstPartyCosmeticFilteringEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(), base::Value(enabled));
}

void DefaultBraveShieldsHandler::SetCosmeticFilteringControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  std::string value = args[0].GetString();

  brave_shields::SetCosmeticFilteringControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      ControlTypeFromString(value), GURL(), g_browser_process->local_state(),
      profile_->GetPrefs());
}

void DefaultBraveShieldsHandler::GetCookieControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const ControlType setting = brave_shields::GetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      CookieSettingsFactory::GetForProfile(profile_).get(), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(),
                            base::Value(ControlTypeToString(setting)));
}

void DefaultBraveShieldsHandler::SetCookieControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  std::string value = args[0].GetString();

  brave_shields::SetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      profile_->GetPrefs(), ControlTypeFromString(value), GURL(),
      g_browser_process->local_state());
}

void DefaultBraveShieldsHandler::GetFingerprintingControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(),
                            base::Value(ControlTypeToString(setting)));
}

void DefaultBraveShieldsHandler::SetFingerprintingControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  std::string value = args[0].GetString();

  brave_shields::SetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      ControlTypeFromString(value), GURL(), g_browser_process->local_state(),
      profile_->GetPrefs());
}

void DefaultBraveShieldsHandler::GetFingerprintingBlockEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());
  bool result = setting != ControlType::ALLOW;
  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(), base::Value(result));
}

void DefaultBraveShieldsHandler::SetFingerprintingBlockEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  bool value = args[0].GetBool();

  brave_shields::SetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      value ? ControlType::DEFAULT : ControlType::ALLOW, GURL(),
      g_browser_process->local_state());
}

void DefaultBraveShieldsHandler::GetHttpsUpgradeControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetHttpsUpgradeControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(),
                            base::Value(ControlTypeToString(setting)));
}

void DefaultBraveShieldsHandler::SetHttpsUpgradeControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  std::string value = args[0].GetString();

  brave_shields::SetHttpsUpgradeControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      ControlTypeFromString(value), GURL(), g_browser_process->local_state());
}

void DefaultBraveShieldsHandler::SetNoScriptControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  bool value = args[0].GetBool();

  brave_shields::SetNoScriptControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      value ? ControlType::BLOCK : ControlType::ALLOW, GURL(),
      g_browser_process->local_state());
}

void DefaultBraveShieldsHandler::SetForgetFirstPartyStorageEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  bool value = args[0].GetBool();

  brave_shields::SetForgetFirstPartyStorageEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile_), value, GURL(),
      g_browser_process->local_state());
}

void DefaultBraveShieldsHandler::GetForgetFirstPartyStorageEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool result = brave_shields::GetForgetFirstPartyStorageEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0].Clone(), base::Value(result));
}
