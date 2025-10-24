// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_UI_H_

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

class EmailAliasesPanelUI : public TopChromeWebUIController {
 public:
  explicit EmailAliasesPanelUI(content::WebUI* web_ui);
  EmailAliasesPanelUI(const EmailAliasesPanelUI&) = delete;
  EmailAliasesPanelUI& operator=(const EmailAliasesPanelUI&) = delete;
  ~EmailAliasesPanelUI() override;
  // Binds the EmailAliasesService for this WebUI so TS can call
  // EmailAliasesService.getRemote().
  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService>
          receiver);

  static constexpr std::string_view GetWebUIName() {
    return "EmailAliasesPanel";
  }

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class EmailAliasesPanelUIConfig
    : public DefaultTopChromeWebUIConfig<EmailAliasesPanelUI> {
 public:
  EmailAliasesPanelUIConfig()
      : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                    kEmailAliasesPanelHost) {}

  bool ShouldAutoResizeHost() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_UI_H_
