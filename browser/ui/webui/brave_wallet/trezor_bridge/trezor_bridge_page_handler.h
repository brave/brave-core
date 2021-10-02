/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_TREZOR_BRIDGE_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_TREZOR_BRIDGE_PAGE_HANDLER_H_

#include <string>

#include "brave/components/trezor_bridge/trezor_bridge.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class WebContents;
class WebUI;
}  // namespace content

class GURL;
class TrezorBridgeUI;

class TrezorBridgePageHandler : public trezor_bridge::mojom::PageHandler {
 public:
  TrezorBridgePageHandler(
      mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver,
      mojo::PendingRemote<trezor_bridge::mojom::Page> page,
      TrezorBridgeUI* trezor_bridge_ui,
      content::WebUI* web_ui);
  TrezorBridgePageHandler(const TrezorBridgePageHandler&) = delete;
  TrezorBridgePageHandler& operator=(const TrezorBridgePageHandler&) = delete;
  ~TrezorBridgePageHandler() override;

  // trezor_bridge::mojom::PageHandler:
  void OnAddressesFetched(const std::vector<std::string>& addresses) override;
  void OnUnlocked(bool success) override;

 private:

  mojo::Receiver<trezor_bridge::mojom::PageHandler> receiver_;
  mojo::Remote<trezor_bridge::mojom::Page> page_;
  // TrezorBridgePageHandler is owned by |trezor_bridge_ui_| and so we expect
  // |trezor_bridge_ui_| to remain valid for the lifetime of |this|.
  TrezorBridgeUI* const trezor_bridge_ui_;
  content::WebUI* const web_ui_;
  content::WebContents* web_contents_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_TREZOR_BRIDGE_PAGE_HANDLER_H_
