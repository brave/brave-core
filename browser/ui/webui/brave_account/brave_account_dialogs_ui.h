/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_

#include "brave/components/brave_account/brave_account_dialogs_ui_base.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

class BraveAccountDialogsUI
    : public BraveAccountDialogsUIBase<content::WebUIDataSource>,
      public ConstrainedWebDialogUI {
 public:
  explicit BraveAccountDialogsUI(content::WebUI* web_ui);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveAccountDialogsUIConfig
    : public content::DefaultWebUIConfig<BraveAccountDialogsUI> {
 public:
  BraveAccountDialogsUIConfig();

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_
