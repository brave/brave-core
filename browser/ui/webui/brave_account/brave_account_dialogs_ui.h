/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_

#include <string>
#include <vector>

#include "brave/components/brave_account/brave_account_dialogs_ui_base.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

class BraveAccountDialogsUI;

namespace content {
class WebUI;
}

class BraveAccountDialogsUIConfig
    : public content::DefaultWebUIConfig<BraveAccountDialogsUI> {
 public:
  BraveAccountDialogsUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountDialogsHost) {
  }
};

class BraveAccountDialogsUI
    : public ConstrainedWebDialogUI,
      public BraveAccountDialogsUIBase<content::WebUIDataSource> {
 public:
  explicit BraveAccountDialogsUI(content::WebUI* web_ui);
  ~BraveAccountDialogsUI() override;

  void OpenDialog() override;

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveAccountDialogsDialog : public ui::WebDialogDelegate {
 public:
  static void Show(content::WebUI* web_ui);
  ~BraveAccountDialogsDialog() override;
  BraveAccountDialogsDialog(const BraveAccountDialogsDialog&) = delete;
  BraveAccountDialogsDialog& operator=(const BraveAccountDialogsDialog&) =
      delete;

 private:
  BraveAccountDialogsDialog();
  // ui::WebDialogDelegate:
  ui::mojom::ModalType GetDialogModalType() const override;
  std::u16string GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogShown(content::WebUI* webui) override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;

  raw_ptr<content::WebUI> webui_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_
