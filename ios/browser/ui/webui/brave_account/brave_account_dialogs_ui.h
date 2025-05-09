// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_

#include "brave/components/brave_account/core/mojom/brave_account.mojom.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/receiver.h"

class GURL;

namespace web {
class WebUIIOS;
}

class BraveAccountDialogsUI : public web::WebUIIOSController,
                              public brave_account::mojom::BraveAccountHandler {
 public:
  explicit BraveAccountDialogsUI(web::WebUIIOS* web_ui, const GURL& url);
  ~BraveAccountDialogsUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_account::mojom::BraveAccountHandler>
          pending_receiver);

  void GetPasswordStrength(
      const std::string& password,
      brave_account::mojom::BraveAccountHandler::GetPasswordStrengthCallback
          callback) override;

  void OpenDialog() override;

 private:
  mojo::Receiver<brave_account::mojom::BraveAccountHandler> receiver_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_UI_H_
