/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SYNC_I_SYNC_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SYNC_I_SYNC_UI_H_

#include <string>

#include "brave/browser/ui/webui/basic_ui.h"

class SyncUI : public BasicUI {
public:
 SyncUI(content::WebUI* web_ui, const std::string& host);
 ~SyncUI() override;

private:
  DISALLOW_COPY_AND_ASSIGN(SyncUI);
};

#endif //BRAVE_BROWSER_UI_WEBUI_SYNC_I_SYNC_UI_H_
