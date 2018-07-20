/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_MD_SETTINGS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_MD_SETTINGS_UI_H_

#include <memory>

#include "chrome/browser/ui/webui/settings/md_settings_ui.h"

class BraveMdSettingsUI : public settings::MdSettingsUI {
 public:
  BraveMdSettingsUI(content::WebUI* web_ui, const std::string& host);
  ~BraveMdSettingsUI() override;

  DISALLOW_COPY_AND_ASSIGN(BraveMdSettingsUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_MD_SETTINGS_UI_H_
