/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_SNAP_EXECUTOR_SNAP_EXECUTOR_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_SNAP_EXECUTOR_SNAP_EXECUTOR_UI_H_

#include <memory>

#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace snap_executor {

// Untrusted WebUI (`chrome-untrusted://snap-executor`) that evaluates snap
// bundles. Deliberately not an `ui::EnableMojoWebUI` — unlike
// `UntrustedLedgerUI`, no Mojo pipe may exist in a frame that evaluates
// downloaded code. The trusted wallet page owns `mojom::SnapBridge` and
// talks to this frame only via postMessage.
class UntrustedSnapExecutorUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedSnapExecutorUI(content::WebUI* web_ui);
  UntrustedSnapExecutorUI(const UntrustedSnapExecutorUI&) = delete;
  UntrustedSnapExecutorUI& operator=(const UntrustedSnapExecutorUI&) = delete;
  ~UntrustedSnapExecutorUI() override;
};

class UntrustedSnapExecutorUIConfig : public content::WebUIConfig {
 public:
  UntrustedSnapExecutorUIConfig();
  ~UntrustedSnapExecutorUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace snap_executor

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_SNAP_EXECUTOR_SNAP_EXECUTOR_UI_H_
