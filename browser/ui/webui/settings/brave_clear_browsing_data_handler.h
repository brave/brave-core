// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_CLEAR_BROWSING_DATA_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_CLEAR_BROWSING_DATA_HANDLER_H_

#include "base/values.h"
#include "chrome/browser/ui/webui/settings/settings_clear_browsing_data_handler.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

namespace settings {

class BraveClearBrowsingDataHandler : public ClearBrowsingDataHandler {
 public:
  BraveClearBrowsingDataHandler(content::WebUI* webui, Profile* profile);
  BraveClearBrowsingDataHandler(const BraveClearBrowsingDataHandler&) = delete;
  BraveClearBrowsingDataHandler& operator=(
      const BraveClearBrowsingDataHandler&) = delete;
  ~BraveClearBrowsingDataHandler() override;

 private:
  // ClearBrowsingDataHandler:
  void RegisterMessages() override;

  void HandleGetBraveRewardsEnabled(const base::Value::List& args);

  void HandleClearBraveAdsData(const base::Value::List& args);

  void OnRewardsEnabledPreferenceChanged();

  raw_ptr<Profile> profile_ = nullptr;

  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_CLEAR_BROWSING_DATA_HANDLER_H_
