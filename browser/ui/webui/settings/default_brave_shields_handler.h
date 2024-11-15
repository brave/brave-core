/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

class Profile;

class DefaultBraveShieldsHandler
    : public settings::SettingsPageUIHandler,
      public content_settings::Observer,
      public content_settings::CookieSettings::Observer {
 public:
  DefaultBraveShieldsHandler();
  DefaultBraveShieldsHandler(const DefaultBraveShieldsHandler&) = delete;
  DefaultBraveShieldsHandler& operator=(const DefaultBraveShieldsHandler&) =
      delete;
  ~DefaultBraveShieldsHandler() override;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  // content_settings::Observer overrides:
  void OnContentSettingChanged(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsTypeSet content_type_set) override;

  // content_settings::CookieSettings::Observer overrides:
  void OnThirdPartyCookieBlockingChanged(
      bool block_third_party_cookies) override;

  void SetAdControlType(const base::Value::List& args);
  void IsAdControlEnabled(const base::Value::List& args);
  void SetCosmeticFilteringControlType(const base::Value::List& args);
  void IsFirstPartyCosmeticFilteringEnabled(const base::Value::List& args);
  void SetCookieControlType(const base::Value::List& args);
  void GetCookieControlType(const base::Value::List& args);
  void GetHideBlockAllCookieFlag(const base::Value::List& args);
  void SetFingerprintingControlType(const base::Value::List& args);
  void GetFingerprintingControlType(const base::Value::List& args);
  void SetFingerprintingBlockEnabled(const base::Value::List& args);
  void GetFingerprintingBlockEnabled(const base::Value::List& args);
  void SetHttpsUpgradeControlType(const base::Value::List& args);
  void GetHttpsUpgradeControlType(const base::Value::List& args);
  void SetNoScriptControlType(const base::Value::List& args);
  void GetNoScriptControlType(const base::Value::List& args);
  void SetForgetFirstPartyStorageEnabled(const base::Value::List& args);
  void GetForgetFirstPartyStorageEnabled(const base::Value::List& args);
  void SetContactInfoSaveFlag(const base::Value::List& args);
  void GetContactInfoSaveFlag(const base::Value::List& args);
  void OnGetContactInfoSaveFlag(base::Value javascript_callback,
                                const bool contact_info_save_flag);

  raw_ptr<Profile> profile_ = nullptr;

  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      content_settings_observation_{this};
  base::ScopedObservation<content_settings::CookieSettings,
                          content_settings::CookieSettings::Observer>
      cookie_settings_observation_{this};
  base::WeakPtrFactory<DefaultBraveShieldsHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
