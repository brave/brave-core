/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UI_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UI_H_

#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace brave_wallet {

class UntrustedPolkadotUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedPolkadotUI(content::WebUI* web_ui);
  UntrustedPolkadotUI(const UntrustedPolkadotUI&) = delete;
  UntrustedPolkadotUI& operator=(const UntrustedPolkadotUI&) = delete;
  ~UntrustedPolkadotUI() override;
};

class UntrustedPolkadotUIConfig : public content::WebUIConfig {
 public:
  UntrustedPolkadotUIConfig();
  ~UntrustedPolkadotUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UI_H_
