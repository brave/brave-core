/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_PRIVACY_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_PRIVACY_HANDLER_H_

#include <string>

#include "base/scoped_observation.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

namespace content {
class WebUIDataSource;
}

class Profile;

class BravePrivacyHandler : public settings::SettingsPageUIHandler,
                            public BraveOriginStateObserver {
 public:
  BravePrivacyHandler();
  BravePrivacyHandler(const BravePrivacyHandler&) = delete;
  BravePrivacyHandler& operator=(const BravePrivacyHandler&) = delete;
  ~BravePrivacyHandler() override;
  static void AddLoadTimeData(content::WebUIDataSource* data_source,
                              Profile* profile);

  // BraveOriginStateObserver:
  void OnBraveOriginStatusChanged() override;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void SetLocalStateBooleanEnabled(const std::string& path,
                                   const base::Value::List& args);
  void GetLocalStateBooleanEnabled(const std::string& path,
                                   const base::Value::List& args);

  void SetStatsUsagePingEnabled(const base::Value::List& args);
  void GetStatsUsagePingEnabled(const base::Value::List& args);
  void OnStatsUsagePingEnabledChanged();

  void SetP3AEnabled(const base::Value::List& args);
  void GetP3AEnabled(const base::Value::List& args);
  void OnP3AEnabledChanged();

  static bool IsStatsReportingHidden();

#if BUILDFLAG(IS_WIN)
  void OnWindowsRecallDisabledChanged();
#endif

  raw_ptr<Profile> profile_ = nullptr;
  PrefChangeRegistrar local_state_change_registrar_;
  base::ScopedObservation<BraveOriginState, BraveOriginStateObserver>
      brave_origin_state_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_PRIVACY_HANDLER_H_
