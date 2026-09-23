// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PROMO_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PROMO_UI_H_

#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class EmailAliasesPromoUI
    : public ConstrainedWebDialogUI,
      public email_aliases::mojom::EmailAliasesPromoHandler {
 public:
  explicit EmailAliasesPromoUI(content::WebUI* web_ui);
  EmailAliasesPromoUI(const EmailAliasesPromoUI&) = delete;
  EmailAliasesPromoUI& operator=(const EmailAliasesPromoUI&) = delete;
  ~EmailAliasesPromoUI() override;

  void SetHandlerDelegate(
      email_aliases::mojom::EmailAliasesPromoHandler* delegate);

  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPromoHandler>
          receiver);

 private:
  void OnPromoClosed() override;

  mojo::Receiver<email_aliases::mojom::EmailAliasesPromoHandler> promo_handler_{
      this};
  raw_ptr<email_aliases::mojom::EmailAliasesPromoHandler> delegate_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class EmailAliasesPromoUIConfig
    : public content::DefaultWebUIConfig<EmailAliasesPromoUI> {
 public:
  EmailAliasesPromoUIConfig();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PROMO_UI_H_
