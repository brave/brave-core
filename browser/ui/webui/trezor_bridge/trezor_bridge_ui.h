/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_UI_H_

#include <string>

#include "base/macros.h"

#include "brave/components/trezor_bridge/mojo_trezor_web_ui_controller.h"
#include "content/public/browser/web_ui_controller.h"

class TrezorBridgePageHandler;

class TrezorBridgeUI : public MojoTrezorWebUIController {
 public:
  TrezorBridgeUI(content::WebUI* web_ui, const std::string& host);
  ~TrezorBridgeUI() override;

  TrezorBridgeUI(const TrezorBridgeUI&) = delete;
  TrezorBridgeUI& operator=(const TrezorBridgeUI&) = delete;

  void CreatePageHandler(
      mojo::PendingRemote<trezor_bridge::mojom::Page> page,
      mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver)
      override;
  void SetSubscriber(base::WeakPtr<Subscriber> subscriber);

 private:
  std::unique_ptr<TrezorBridgePageHandler> page_handler_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_UI_H_
