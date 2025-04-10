/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/psst/browser/core/psst_consent_dialog.mojom.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace psst {

class BravePsstDialogUI;

class RewardsDOMHandler : public content::WebUIMessageHandler,
                          public psst_consent_dialog::mojom::PsstConsentHelper {
 public:
  RewardsDOMHandler();
  RewardsDOMHandler(const RewardsDOMHandler&) = delete;
  RewardsDOMHandler& operator=(const RewardsDOMHandler&) = delete;
  ~RewardsDOMHandler() override;

  void BindInterface(mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper> pending_receiver);

  void Init();

  // WebUIMessageHandler implementation.
//   void OnJavascriptAllowed() override;
//   void OnJavascriptDisallowed() override;
  void RegisterMessages() override;

 private:
 void SetClientPage(::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> dialog) override;

 mojo::ReceiverSet<psst_consent_dialog::mojom::PsstConsentHelper> receivers_;
 mojo::Remote<psst_consent_dialog::mojom::PsstConsentDialog> client_page_;
 base::WeakPtrFactory<RewardsDOMHandler> weak_factory_{this};
};

class BravePsstDialogUIConfig
    : public content::DefaultWebUIConfig<BravePsstDialogUI> {
 public:
 BravePsstDialogUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBravePsstHost) { LOG(INFO) << "[PSST] BravePsstDialogUIConfig";}
};

class BravePsstDialogUI : public ConstrainedWebDialogUI {
 public:
 explicit BravePsstDialogUI(content::WebUI* web_ui);
  ~BravePsstDialogUI() override;
  BravePsstDialogUI(const BravePsstDialogUI&) = delete;
  BravePsstDialogUI& operator=(const BravePsstDialogUI&) = delete;

  void BindInterface(mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper>
    pending_receiver);

  private:
  raw_ptr<RewardsDOMHandler> handler_;
  WEB_UI_CONTROLLER_TYPE_DECL();
};

}

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_