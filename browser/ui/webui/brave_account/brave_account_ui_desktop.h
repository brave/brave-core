/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_DESKTOP_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_DESKTOP_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_account/brave_account_service_factory.h"
#include "brave/components/brave_account/brave_account_ui_base.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"

class PrefService;

namespace content {
class WebUI;
}  // namespace content

class BraveAccountUIDesktop
    : public BraveAccountUIBase<content::WebUIDataSource,
                                brave_account::BraveAccountServiceFactory>,
      public ConstrainedWebDialogUI {
 public:
  explicit BraveAccountUIDesktop(content::WebUI* web_ui);

  ~BraveAccountUIDesktop() override;

 private:
  void OnVerificationTokenChanged();

  const raw_ptr<PrefService> pref_service_;
  PrefChangeRegistrar pref_change_registrar_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveAccountUIDesktopConfig
    : public content::DefaultWebUIConfig<BraveAccountUIDesktop> {
 public:
  BraveAccountUIDesktopConfig();
};

void ShowBraveAccountDialog(content::WebUI* web_ui);

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_DESKTOP_H_
