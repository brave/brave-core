/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_UI_H_

#include <memory>

#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ledger {

class UntrustedLedgerUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedLedgerUI(content::WebUI* web_ui);
  UntrustedLedgerUI(const UntrustedLedgerUI&) = delete;
  UntrustedLedgerUI& operator=(const UntrustedLedgerUI&) = delete;
  ~UntrustedLedgerUI() override;
};

class UntrustedLedgerUIConfig : public content::WebUIConfig {
 public:
  UntrustedLedgerUIConfig();
  ~UntrustedLedgerUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace ledger

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_UI_H_
