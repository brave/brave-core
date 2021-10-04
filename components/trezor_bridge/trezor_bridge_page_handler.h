/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_TREZOR_BRIDGE_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_TREZOR_BRIDGE_PAGE_HANDLER_H_

#include <string>

#include "brave/components/trezor_bridge/mojo_trezor_web_ui_controller.h"
#include "brave/components/trezor_bridge/trezor_bridge.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class TrezorBridgeUI;

class TrezorBridgePageHandler
    : public trezor_bridge::mojom::PageHandler,
      public MojoTrezorWebUIController::LibraryController {
 public:
  TrezorBridgePageHandler(
      mojo::PendingReceiver<trezor_bridge::mojom::PageHandler> receiver,
      mojo::PendingRemote<trezor_bridge::mojom::Page> page);
  TrezorBridgePageHandler(const TrezorBridgePageHandler&) = delete;
  TrezorBridgePageHandler& operator=(const TrezorBridgePageHandler&) = delete;
  ~TrezorBridgePageHandler() override;

  // trezor_bridge::mojom::PageHandler:
  void OnAddressesReceived(
      bool success,
      std::vector<trezor_bridge::mojom::HardwareWalletAccountPtr> accounts,
      const std::string& error) override;
  void OnUnlocked(bool success, const std::string& error) override;

  // MojoTrezorWebUIController:LibraryController
  void RequestAddresses(const std::vector<std::string>& addresses) override;
  void Unlock() override;

  void SetSubscriber(
      base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber);

  base::WeakPtr<TrezorBridgePageHandler> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  mojo::Receiver<trezor_bridge::mojom::PageHandler> receiver_;
  mojo::Remote<trezor_bridge::mojom::Page> page_;

  base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber_;
  base::WeakPtrFactory<TrezorBridgePageHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_TREZOR_BRIDGE_PAGE_HANDLER_H_
