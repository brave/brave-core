/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SITE_SETTINGS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SITE_SETTINGS_HANDLER_H_

#include <string>
#include <vector>

#include "chrome/browser/ui/webui/settings/site_settings_handler.h"

namespace settings {

class BraveSiteSettingsHandler : public SiteSettingsHandler {
 public:
  explicit BraveSiteSettingsHandler(Profile* profile);

  BraveSiteSettingsHandler(const BraveSiteSettingsHandler&) = delete;
  BraveSiteSettingsHandler& operator=(const BraveSiteSettingsHandler&) = delete;

  ~BraveSiteSettingsHandler() override;

  // SettingsPageUIHandler:
  void RegisterMessages() override;

  // Returns whether the pattern is valid given the type.
  void HandleIsPatternValidForType(const base::Value::List& args);

  bool IsPatternValidForBraveContentType(ContentSettingsType content_type,
                                         const std::string& pattern_string);

  void RemoveNonModelData(const std::vector<url::Origin>& origins) override;

 private:
  friend class TestBraveSiteSettingsHandlerUnittest;
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SITE_SETTINGS_HANDLER_H_
