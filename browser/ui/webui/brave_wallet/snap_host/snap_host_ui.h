/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_SNAP_HOST_SNAP_HOST_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_SNAP_HOST_SNAP_HOST_UI_H_

#include <memory>

#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace snap_host {

// Untrusted WebUI (`chrome-untrusted://snap-host`) that evaluates snap
// bundles. Unlike `UntrustedLedgerUI`, it does not use
// `ui::EnableMojoWebUI`. The trusted wallet page owns
// `mojom::SnapHostBridge` and talks to this frame via postMessage.
class UntrustedSnapHostUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedSnapHostUI(content::WebUI* web_ui);
  UntrustedSnapHostUI(const UntrustedSnapHostUI&) = delete;
  UntrustedSnapHostUI& operator=(const UntrustedSnapHostUI&) = delete;
  ~UntrustedSnapHostUI() override;
};

class UntrustedSnapHostUIConfig : public content::WebUIConfig {
 public:
  UntrustedSnapHostUIConfig();
  ~UntrustedSnapHostUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace snap_host

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_SNAP_HOST_SNAP_HOST_UI_H_
