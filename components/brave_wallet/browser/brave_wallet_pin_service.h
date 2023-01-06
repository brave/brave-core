// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_

#include <set>
#include <string>
#include <vector>

#include "base/containers/cxx20_erase_deque.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_wallet::mojom::BlockchainTokenPtr;

namespace brave_wallet {

std::string StatusToString(const mojom::TokenPinStatusCode& status);
std::string ErrorCodeToString(
    const mojom::WalletPinServiceErrorCode& error_code);

class BraveWalletPinService : public KeyedService,
                              public brave_wallet::mojom::WalletPinService,
                              public ipfs::IpfsServiceObserver {
 public:
  BraveWalletPinService(PrefService* prefs,
                        JsonRpcService* service,
                        ipfs::IpfsLocalPinService* local_pin_service,
                        IpfsService* ipfs_service);
  ~BraveWalletPinService() override;

  mojo::PendingRemote<mojom::WalletPinService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::WalletPinService> receiver);

  static absl::optional<std::string> GetPath(
      const absl::optional<std::string>& service,
      const BlockchainTokenPtr& token);
  static BlockchainTokenPtr TokenFromPath(const std::string& path);
  static absl::optional<std::string> ServiceFromPath(const std::string& path);

  // WalletPinService
  void AddObserver(::mojo::PendingRemote<mojom::BraveWalletPinServiceObserver>
                       observer) override;
  void AddPin(BlockchainTokenPtr token,
              const absl::optional<std::string>& service,
              AddPinCallback callback) override;
  void RemovePin(BlockchainTokenPtr token,
                 const absl::optional<std::string>& service,
                 RemovePinCallback callback) override;
  void GetTokenStatus(BlockchainTokenPtr token,
                      GetTokenStatusCallback callback) override;
  void Validate(BlockchainTokenPtr token,
                const absl::optional<std::string>& service,
                ValidateCallback callback) override;
  void IsLocalNodeRunning(IsLocalNodeRunningCallback callback) override;

  virtual void MarkAsPendingForPinning(
      const mojom::BlockchainTokenPtr& token,
      const absl::optional<std::string>& service);
  virtual void MarkAsPendingForUnpinning(
      const mojom::BlockchainTokenPtr& token,
      const absl::optional<std::string>& service);

  virtual mojom::TokenPinStatusPtr GetTokenStatus(
      const absl::optional<std::string>& service,
      const mojom::BlockchainTokenPtr& token);
  virtual absl::optional<base::Time> GetLastValidateTime(
      const absl::optional<std::string>& service,
      const mojom::BlockchainTokenPtr& token);
  virtual std::set<std::string> GetTokens(
      const absl::optional<std::string>& service);

 protected:
  // For testing
  BraveWalletPinService();

 private:
  bool CreateToken(const absl::optional<std::string>& service,
                   const mojom::BlockchainTokenPtr& token,
                   const std::vector<std::string>& cids);
  bool RemoveToken(const absl::optional<std::string>& service,
                   const mojom::BlockchainTokenPtr& token);
  bool SetTokenStatus(const absl::optional<std::string>& service,
                      const mojom::BlockchainTokenPtr& token,
                      mojom::TokenPinStatusCode,
                      const mojom::PinErrorPtr& error);

  void FinishAddingWithResult(const absl::optional<std::string>& service,
                              const mojom::BlockchainTokenPtr& token,
                              bool result,
                              mojom::PinErrorPtr error,
                              AddPinCallback callback);

  absl::optional<std::vector<std::string>> ResolvePinItems(
      const absl::optional<std::string>& service,
      const BlockchainTokenPtr& token);

  void OnPinsRemoved(absl::optional<std::string> service,
                     RemovePinCallback callback,
                     mojom::BlockchainTokenPtr token,
                     bool result);
  void OnTokenPinned(absl::optional<std::string> service,
                     AddPinCallback callback,
                     mojom::BlockchainTokenPtr,
                     bool result);
  void OnTokenValidated(absl::optional<std::string> service,
                        ValidateCallback callback,
                        mojom::BlockchainTokenPtr,
                        absl::optional<bool> result);

  void OnTokenMetaDataReceived(absl::optional<std::string> service,
                               AddPinCallback callback,
                               mojom::BlockchainTokenPtr token,
                               const std::string& token_url,
                               const std::string& result,
                               mojom::ProviderError error,
                               const std::string& error_message);

  // ipfs::IpfsServiceObserver
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnIpfsShutdown() override;

  mojo::ReceiverSet<brave_wallet::mojom::WalletPinService> receivers_;
  mojo::RemoteSet<mojom::BraveWalletPinServiceObserver> observers_;

  // Prefs service is used to store list of pinned items
  raw_ptr<PrefService> prefs_ = nullptr;

  // JsonRpcService is used to fetch token metadata
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<ipfs::IpfsLocalPinService> local_pin_service_ = nullptr;
  raw_ptr<IpfsService> ipfs_service_ = nullptr;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_
