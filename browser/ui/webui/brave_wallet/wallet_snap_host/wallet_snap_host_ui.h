/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_SNAP_HOST_WALLET_SNAP_HOST_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_SNAP_HOST_WALLET_SNAP_HOST_UI_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class WebUI;
}  // namespace content

namespace brave_wallet {

// Trusted host page for the hidden snap execution environment. Loaded in a
// never-composited WebContents (or, under host-page-debug, a real tab); its
// TS binds mojom::SnapService and embeds chrome-untrusted://snap-host/.
class WalletSnapHostUI : public ui::MojoWebUIController {
 public:
  explicit WalletSnapHostUI(content::WebUI* web_ui);
  WalletSnapHostUI(const WalletSnapHostUI&) = delete;
  WalletSnapHostUI& operator=(const WalletSnapHostUI&) = delete;
  ~WalletSnapHostUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::SnapService> receiver);

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class WalletSnapHostUIConfig
    : public content::DefaultWebUIConfig<WalletSnapHostUI> {
 public:
  WalletSnapHostUIConfig();

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_SNAP_HOST_WALLET_SNAP_HOST_UI_H_
