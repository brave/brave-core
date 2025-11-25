// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_UI_H_

#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/resources/cr_components/color_change_listener/color_change_listener.mojom.h"

class EmailAliasesPanelUI
    : public ConstrainedWebDialogUI,
      public email_aliases::mojom::EmailAliasesPanelHandler {
 public:
  explicit EmailAliasesPanelUI(content::WebUI* web_ui);
  EmailAliasesPanelUI(const EmailAliasesPanelUI&) = delete;
  EmailAliasesPanelUI& operator=(const EmailAliasesPanelUI&) = delete;
  ~EmailAliasesPanelUI() override;

  void SetHandlerDelegate(
      email_aliases::mojom::EmailAliasesPanelHandler* delegate);

  // Binds the EmailAliasesService for this WebUI so TS can call
  // EmailAliasesService.getRemote().
  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService>
          receiver);

  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPanelHandler>
          receiver);

  void BindInterface(
      mojo::PendingReceiver<color_change_listener::mojom::PageHandler>
          pending_receiver);

 private:
  void OnAliasCreated(const std::string& email) override;
  void OnManageAliases() override;
  void OnCancelAliasCreation() override;

  mojo::Receiver<email_aliases::mojom::EmailAliasesPanelHandler> panel_handler_{
      this};
  raw_ptr<email_aliases::mojom::EmailAliasesPanelHandler> delegate_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class EmailAliasesPanelUIConfig
    : public content::DefaultWebUIConfig<EmailAliasesPanelUI> {
 public:
  EmailAliasesPanelUIConfig();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_UI_H_
