// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_HANDLER_H_

#include <string>

#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class TopChromeWebUIController;

namespace email_aliases {
class EmailAliasesController;
}

class EmailAliasesPanelHandler
    : public email_aliases::mojom::EmailAliasesPanelHandler {
 public:
  EmailAliasesPanelHandler(
      TopChromeWebUIController* webui_controller,
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPanelHandler>
          receiver);
  ~EmailAliasesPanelHandler() override;

 private:
  email_aliases::EmailAliasesController* GetEmailAliasesController();

  // email_aliases::mojom::EmailAliasesPanelHandler:
  void OnAliasCreationComplete(const std::string& email) override;
  void OnManageAliases() override;
  void OnCancel() override;

  const raw_ptr<TopChromeWebUIController> webui_controller_ = nullptr;
  mojo::Receiver<email_aliases::mojom::EmailAliasesPanelHandler> receiver_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_HANDLER_H_
