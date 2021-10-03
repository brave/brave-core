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
#include "brave/components/trezor_bridge/mojo_trezor_web_ui_controller.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {
class BrowserContext;
}

class TrezorBridgeUI;

namespace brave_wallet {

class TrezorBridgeContentObserver {
 public:
  virtual void BridgeReady() = 0;
  virtual void BridgeFail() = 0;
};

class TrezorBridgeContentProxy {
 public:
  explicit TrezorBridgeContentProxy(content::BrowserContext* browser_context) {}
  virtual ~TrezorBridgeContentProxy() = default;

  virtual void SetObserver(TrezorBridgeContentObserver* observer) = 0;
  virtual void InitWebContents() = 0;
  virtual base::WeakPtr<MojoTrezorWebUIController::LibraryController>
  ConnectWithWebUIBridge(
      base::WeakPtr<MojoTrezorWebUIController::Subscriber> subscriber) = 0;
  virtual bool IsReady() const = 0;
};

class TrezorBridgeController : public KeyedService,
                               public mojom::TrezorBridgeController,
                               public MojoTrezorWebUIController::Subscriber,
                               public TrezorBridgeContentObserver {
 public:
  explicit TrezorBridgeController(
      content::BrowserContext* browser_context,
      std::unique_ptr<TrezorBridgeContentProxy> content_proxy);
  ~TrezorBridgeController() override;

  TrezorBridgeController(const TrezorBridgeController&) = delete;
  TrezorBridgeController& operator=(const TrezorBridgeController&) = delete;

  mojo::PendingRemote<mojom::TrezorBridgeController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::TrezorBridgeController> receiver);

  // mojom::TrezorBridgeController:
  void Unlock(UnlockCallback callback) override;
  void GetTrezorAccounts(const std::vector<std::string>& paths,
                         GetTrezorAccountsCallback callback) override;

  void OnAddressesReceived(
      bool success, std::vector<trezor_bridge::mojom::HardwareWalletAccountPtr> accounts)
      override;
  void OnUnlocked(bool success) override;

 private:
  void InitWebContents(content::BrowserContext* browser_context);
  bool IsUnlocked() const;

  void BridgeReady() override;
  void BridgeFail() override;

  bool unlocked_ = false;
  UnlockCallback unlock_callback_;
  GetTrezorAccountsCallback get_trezor_accounts_callback_;
  base::WeakPtr<MojoTrezorWebUIController::LibraryController>
      library_controller_;
  std::unique_ptr<TrezorBridgeContentProxy> content_proxy_;

  mojo::ReceiverSet<mojom::TrezorBridgeController> receivers_;
  base::WeakPtrFactory<TrezorBridgeController> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TREZOR_BRIDGE_CONTROLLER_H_
