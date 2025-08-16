/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/browser/webcompat_reporter/webcompat_reporter_service_factory.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/browser/de_amp_util.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
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
      "getAdBlockOnlyModeEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetAdBlockOnlyModeEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setAdBlockOnlyModeEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetAdBlockOnlyModeEnabled,
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
      "getNoScriptControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetNoScriptControlType,
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
  web_ui()->RegisterMessageCallback(
      "setContactInfoSaveFlag",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetContactInfoSaveFlag,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getContactInfo",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetContactInfo,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getHideBlockAllCookieTogle",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetHideBlockAllCookieFlag,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDeAmpEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetDeAmpEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setDeAmpEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetDeAmpEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDebounceEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetDebounceEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setDebounceEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetDebounceEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getReduceLanguageEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetReduceLanguageEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setReduceLanguageEnabled",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetReduceLanguageEnabled,
                          base::Unretained(this)));

  content_settings_observation_.Observe(
      HostContentSettingsMapFactory::GetForProfile(profile_));
  cookie_settings_observation_.Observe(
      CookieSettingsFactory::GetForProfile(profile_).get());

  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      brave_shields::prefs::kAdblockAdBlockOnlyModeEnabled,
      base::BindRepeating(
          &DefaultBraveShieldsHandler::OnAdBlockOnlyModePrefChanged,
          weak_ptr_factory_.GetWeakPtr()));
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

void DefaultBraveShieldsHandler::OnAdBlockOnlyModePrefChanged() {
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
  const bool is_ad_control_enabled =
      setting == ControlType::BLOCK &&
      !brave_shields::GetBraveShieldsAdBlockOnlyModeEnabled(
          profile_->GetPrefs());

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(is_ad_control_enabled));
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
  ResolveJavascriptCallback(args[0], base::Value(enabled));
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
      CookieSettingsFactory::GetForProfile(profile_).get(), GURL(),
      profile_->GetPrefs());

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(ControlTypeToString(setting)));
}

void DefaultBraveShieldsHandler::GetHideBlockAllCookieFlag(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  const ControlType setting = brave_shields::GetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_),
      CookieSettingsFactory::GetForProfile(profile_).get(), GURL(),
      profile_->GetPrefs());

  const bool block_all_cookies_feature_enabled = base::FeatureList::IsEnabled(
      brave_shields::features::kBlockAllCookiesToggle);

  AllowJavascript();
  ResolveJavascriptCallback(args[0],
                            base::Value(setting != ControlType::BLOCK &&
                                        !block_all_cookies_feature_enabled));
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
  ResolveJavascriptCallback(args[0], base::Value(ControlTypeToString(setting)));
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
  ResolveJavascriptCallback(args[0], base::Value(result));
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

void DefaultBraveShieldsHandler::GetAdBlockOnlyModeEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool enabled = brave_shields::GetBraveShieldsAdBlockOnlyModeEnabled(
      profile_->GetPrefs());
  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(enabled));
}

void DefaultBraveShieldsHandler::SetAdBlockOnlyModeEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool enabled = args[0].GetBool();
  brave_shields::SetBraveShieldsAdBlockOnlyModeEnabled(profile_->GetPrefs(),
                                                       enabled);
}

void DefaultBraveShieldsHandler::GetHttpsUpgradeControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetHttpsUpgradeControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(ControlTypeToString(setting)));
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

void DefaultBraveShieldsHandler::GetNoScriptControlType(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetNoScriptControlType(
      HostContentSettingsMapFactory::GetForProfile(profile_), GURL());

  AllowJavascript();
  ResolveJavascriptCallback(args[0],
                            base::Value(setting == ControlType::BLOCK));
}

void DefaultBraveShieldsHandler::SetContactInfoSaveFlag(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  if (!args[0].is_bool()) {
    return;
  }
  bool value = args[0].GetBool();

  auto* webcompat_reporter_service =
      webcompat_reporter::WebcompatReporterServiceFactory::GetServiceForContext(
          profile_);
  if (webcompat_reporter_service) {
    webcompat_reporter_service->SetContactInfoSaveFlag(value);
  }
}

void DefaultBraveShieldsHandler::GetContactInfo(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);
  AllowJavascript();

  auto* webcompat_reporter_service =
      webcompat_reporter::WebcompatReporterServiceFactory::GetServiceForContext(
          profile_);
  if (!webcompat_reporter_service) {
    base::Value::Dict params_dict;
    params_dict.Set("contactInfo", "");
    params_dict.Set("contactInfoSaveFlag", false);
    ResolveJavascriptCallback(args[0], std::move(params_dict));
    return;
  }

  webcompat_reporter_service->GetBrowserParams(
      base::BindOnce(&DefaultBraveShieldsHandler::OnGetContactInfo,
                     weak_ptr_factory_.GetWeakPtr(), args[0].Clone()));
}
void DefaultBraveShieldsHandler::OnGetContactInfo(
    base::Value javascript_callback,
    const std::optional<std::string>& contact_info,
    const bool contact_info_save_flag,
    const std::vector<std::string>& components) {
  base::Value::Dict params_dict;
  params_dict.Set("contactInfo", contact_info.value_or(""));
  params_dict.Set("contactInfoSaveFlag", contact_info_save_flag);
  ResolveJavascriptCallback(javascript_callback, std::move(params_dict));
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
  ResolveJavascriptCallback(args[0], base::Value(result));
}

void DefaultBraveShieldsHandler::GetDeAmpEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool de_amp_enabled = de_amp::IsDeAmpEnabled(
      profile_->GetPrefs(),
      HostContentSettingsMapFactory::GetForProfile(profile_));

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(de_amp_enabled));
}

void DefaultBraveShieldsHandler::SetDeAmpEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool value = args[0].GetBool();
  profile_->GetPrefs()->SetBoolean(de_amp::kDeAmpPrefEnabled, value);
}

void DefaultBraveShieldsHandler::GetDebounceEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool debounce_enabled =
      profile_->GetPrefs()->GetBoolean(debounce::prefs::kDebounceEnabled) &&
      !brave_shields::GetBraveShieldsAdBlockOnlyModeEnabled(
          profile_->GetPrefs());

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(debounce_enabled));
}

void DefaultBraveShieldsHandler::SetDebounceEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  bool value = args[0].GetBool();
  profile_->GetPrefs()->SetBoolean(debounce::prefs::kDebounceEnabled, value);
}

void DefaultBraveShieldsHandler::GetReduceLanguageEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  const bool reduce_language_enabled =
      brave_shields::IsReduceLanguageEnabledForProfile(
          HostContentSettingsMapFactory::GetForProfile(profile_), GURL(),
          profile_->GetPrefs());

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(reduce_language_enabled));
}

void DefaultBraveShieldsHandler::SetReduceLanguageEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(profile_);

  bool value = args[0].GetBool();
  profile_->GetPrefs()->SetBoolean(brave_shields::prefs::kReduceLanguageEnabled,
                                   value);
}
