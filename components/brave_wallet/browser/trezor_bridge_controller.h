/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TREZOR_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TREZOR_BRIDGE_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "brave/components/trezor_bridge/mojo_trezor_web_ui_controller.h"

namespace content {
  class WebContents;
}

class TrezorBridgeUI;

namespace brave_wallet {

class TrezorBridgeController : public KeyedService,
                               public mojom::TrezorBridgeController,
                               public MojoTrezorWebUIController::Embedder {
 public:
  explicit TrezorBridgeController(content::BrowserContext* browser_context);
  ~TrezorBridgeController() override;

  TrezorBridgeController(const TrezorBridgeController&) = delete;
  TrezorBridgeController& operator=(const TrezorBridgeController&) = delete;

  mojo::PendingRemote<mojom::TrezorBridgeController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::TrezorBridgeController> receiver);

  // mojom::TrezorBridgeController:
  void Unlock(UnlockCallback callback) override;
  void GetAddress(const std::string& path,
                  GetAddressCallback callback) override;
  
  void Unlock() override;
  void GetAccounts(std::vector<std::string> accounts) override;
  // May return null.
  TrezorBridgeUI* GetWebUIController();

 private:
  bool IsUnlocked();
  content::WebContents* web_contents() { return web_contents_.get(); }

  bool unlocked_ = false;
  
  mojo::ReceiverSet<mojom::TrezorBridgeController> receivers_;
  std::unique_ptr<content::WebContents> web_contents_;
  base::WeakPtrFactory<TrezorBridgeController> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TREZOR_BRIDGE_CONTROLLER_H_
