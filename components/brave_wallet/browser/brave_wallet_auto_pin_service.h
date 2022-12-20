// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_AUTO_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_AUTO_PIN_SERVICE_H_

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_pin_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_wallet::mojom::BlockchainTokenPtr;

namespace brave_wallet {

enum Operation { kAdd = 0, kDelete = 1, kValidate = 2 };

struct IntentData {
  BlockchainTokenPtr token;
  Operation operation;
  absl::optional<std::string> service;
  size_t attempt = 0;
  IntentData(const BlockchainTokenPtr& token,
             Operation operation,
             absl::optional<std::string> service);
  ~IntentData();
};

class BraveWalletAutoPinService
    : public KeyedService,
      public brave_wallet::mojom::WalletAutoPinService,
      public brave_wallet::mojom::BraveWalletServiceTokenObserver {
 public:
  BraveWalletAutoPinService(PrefService* prefs,
                            BraveWalletService* brave_wallet_service,
                            BraveWalletPinService* brave_wallet_pin_service);
  ~BraveWalletAutoPinService() override;

  mojo::PendingRemote<mojom::WalletAutoPinService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::WalletAutoPinService> receiver);

  void SetAutoPinEnabled(bool enabled) override;
  void IsAutoPinEnabled(IsAutoPinEnabledCallback callback) override;

  void PostPinToken(BlockchainTokenPtr token,
                    PostPinTokenCallback callback) override;
  void PostUnpinToken(BlockchainTokenPtr token,
                      PostUnpinTokenCallback callback) override;

  // BraveWalletServiceTokenObserver
  void OnTokenAdded(mojom::BlockchainTokenPtr token) override;
  void OnTokenRemoved(mojom::BlockchainTokenPtr token) override;

 private:
  void Restore();
  void OnTokenListResolved(std::vector<BlockchainTokenPtr>);

  void CheckQueue();
  void AddOrExecute(std::unique_ptr<IntentData> data);
  void PostRetry(std::unique_ptr<IntentData> data);

  bool IsAutoPinEnabled();

  std::vector<absl::optional<std::string>> GetServicesToPin();
  std::vector<absl::optional<std::string>> GetKnownServices();

  void ValidateToken(const std::unique_ptr<IntentData>& data);
  void PinToken(const std::unique_ptr<IntentData>& data);
  void UnpinToken(const std::unique_ptr<IntentData>& data);

  void OnTaskFinished(bool result, mojom::PinErrorPtr error);
  void OnValidateTaskFinished(bool result, mojom::PinErrorPtr error);

  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceTokenObserver>
      token_observer_{this};
  mojo::ReceiverSet<brave_wallet::mojom::WalletAutoPinService> receivers_;

  raw_ptr<PrefService> pref_service_;
  raw_ptr<BraveWalletService> brave_wallet_service_;
  raw_ptr<BraveWalletPinService> brave_wallet_pin_service_;

  std::unique_ptr<IntentData> current_;
  std::deque<std::unique_ptr<IntentData>> queue_;

  base::WeakPtrFactory<BraveWalletAutoPinService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_AUTO_PIN_SERVICE_H_
