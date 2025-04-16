/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/psst/brave_psst_consent_helper_handler.h"
#include "brave/components/psst/browser/core/psst_consent_dialog.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "ui/web_dialogs/web_dialog_ui.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class Browser;

namespace psst {

class BravePsstDialogUI;

// class RewardsDOMHandler : public content::WebUIMessageHandler,
//                           public psst_consent_dialog::mojom::PsstConsentHelper {
//  public:
//   RewardsDOMHandler();
//   RewardsDOMHandler(const RewardsDOMHandler&) = delete;
//   RewardsDOMHandler& operator=(const RewardsDOMHandler&) = delete;
//   ~RewardsDOMHandler() override;

//   void BindInterface(mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper> pending_receiver);

//   void Init(BravePsstDialogUI* dlg);

//   // WebUIMessageHandler implementation.
// //   void OnJavascriptAllowed() override;
// //   void OnJavascriptDisallowed() override;
//   void RegisterMessages() override;

//  private:
//  void SetClientPage(::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> dialog) override;
//  void CloseDialog() override;

//  raw_ptr<BravePsstDialogUI> dlg_;
//  mojo::ReceiverSet<psst_consent_dialog::mojom::PsstConsentHelper> receivers_;
//  mojo::Remote<psst_consent_dialog::mojom::PsstConsentDialog> client_page_;
//  base::WeakPtrFactory<RewardsDOMHandler> weak_factory_{this};
// };

class BravePsstDialogUIConfig
    : public content::DefaultWebUIConfig<BravePsstDialogUI> {
 public:
 BravePsstDialogUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBravePsstHost) { LOG(INFO) << "[PSST] BravePsstDialogUIConfig";}
};

class BravePsstDialogUI
    : public ui::MojoWebDialogUI,
      public psst_consent_dialog::mojom::PsstConsentFactory {
 public:
  explicit BravePsstDialogUI(content::WebUI* web_ui);
  ~BravePsstDialogUI() override;
  BravePsstDialogUI(const BravePsstDialogUI&) = delete;
  BravePsstDialogUI& operator=(const BravePsstDialogUI&) = delete;

  // Instantiates the implementor of the psst_consent_dialog::mojom::PsstConsentFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
    mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentFactory>
        receiver);


 private:
  void CreatePsstConsentHandler(
      ::mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper> psst_consent_helper,
      ::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> psst_consent_dialog) override;
  raw_ptr<Browser> browser_;

  std::unique_ptr<BravePsstConsentHelperHandler> psst_consent_handler_;
  mojo::Receiver<psst_consent_dialog::mojom::PsstConsentFactory>
      psst_consent_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};
}

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_UI_H_