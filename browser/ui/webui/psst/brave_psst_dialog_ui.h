/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/psst/common/constants.h"
#include "brave/components/psst/common/psst_ui_common.mojom-shared.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "ui/web_dialogs/web_dialog_ui.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace psst {

class BravePsstDialogUI;
class BravePsstDialogUIConfig
    : public content::DefaultWebUIConfig<BravePsstDialogUI> {
 public:
  BravePsstDialogUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBravePsstHost) {}
};

class BravePsstDialogUI : public ui::MojoWebDialogUI,
                          public psst::mojom::PsstConsentFactory {
 public:
  explicit BravePsstDialogUI(content::WebUI* web_ui);
  ~BravePsstDialogUI() override;
  BravePsstDialogUI(const BravePsstDialogUI&) = delete;
  BravePsstDialogUI& operator=(const BravePsstDialogUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<psst::mojom::PsstConsentFactory> receiver);

 private:
  friend class PsstTabWebContentsObserverBrowserTest;
  void CreatePsstConsentHandler(
      ::mojo::PendingReceiver<psst::mojom::PsstConsentHelper>
          psst_consent_helper,
      ::mojo::PendingRemote<psst::mojom::PsstConsentDialog> psst_consent_dialog)
      override;

  std::unique_ptr<BravePsstDialogHandler> psst_consent_handler_;
  mojo::Receiver<psst::mojom::PsstConsentFactory>
      psst_consent_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};
}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_
