/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_

#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"
#include "brave/components/ipfs/pin/ipfs_remote_pin_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_wallet::mojom::BlockchainTokenPtr;

namespace brave_wallet {

class BraveWalletPinService : public KeyedService,
                              public brave_wallet::mojom::WalletPinService {
 public:
  BraveWalletPinService(PrefService* prefs,
                        JsonRpcService* service,
                        ipfs::IPFSRemotePinService* remote_pin_service,
                        ipfs::IpfsLocalPinService* local_pin_service);
  ~BraveWalletPinService() override;

  mojo::PendingRemote<mojom::WalletPinService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::WalletPinService> receiver);

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
  void AddRemotePinService(mojom::RemotePinServicePtr service,
                           AddRemotePinServiceCallback callback) override;
  void RemoveRemotePinService(const std::string& name,
                              RemoveRemotePinServiceCallback callback) override;
  void GetRemotePinServices(GetRemotePinServicesCallback callback) override;
  void Validate(BlockchainTokenPtr token,
                const absl::optional<std::string>& service,
                ValidateCallback callback) override;

  void MarkAsPendingForPinning(const mojom::BlockchainTokenPtr& token,
                               const absl::optional<std::string>& service);
  void MarkAsPendingForUnpinning(const mojom::BlockchainTokenPtr& token,
                                 const absl::optional<std::string>& service);

  mojom::TokenPinStatusCode GetTokenStatus(
      absl::optional<std::string> service,
      const mojom::BlockchainTokenPtr& token);
  absl::optional<base::Time> GetLastValidateTime(
      absl::optional<std::string> service,
      const mojom::BlockchainTokenPtr& token);

 private:
  void CreateToken(absl::optional<std::string> service,
                   const mojom::BlockchainTokenPtr& token,
                   const std::vector<std::string>& cids);
  void SetTokenStatus(absl::optional<std::string> service,
                      const mojom::BlockchainTokenPtr& token,
                      mojom::TokenPinStatusCode);

  void AddPinInternal(BlockchainTokenPtr token,
                      const absl::optional<std::string>& service,
                      AddPinCallback callback);
  void RemovePinInternal(BlockchainTokenPtr token,
                         const absl::optional<std::string>& service,
                         RemovePinCallback callback);
  void ValidateTokenInternal(BlockchainTokenPtr token,
                             const absl::optional<std::string>& service,
                             ValidateCallback callback);

  void OnAddRemotePinServiceResult(AddRemotePinServiceCallback callback,
                                   bool result);
  void OnRemoveRemotePinServiceResult(RemoveRemotePinServiceCallback callback,
                                      bool result);
  void OnGetRemotePinServicesResult(
      GetRemotePinServicesCallback callback,
      absl::optional<ipfs::GetRemotePinServicesResult> result);

  // Precreate nft.storage
  using EnsureNFTStorageServiceAddedCallback = base::OnceCallback<void(bool)>;
  void EnsureNFTStorageServiceAdded(
      EnsureNFTStorageServiceAddedCallback callback);
  void OnAvailableServicesReady(
      EnsureNFTStorageServiceAddedCallback callback,
      absl::optional<ipfs::GetRemotePinServicesResult> result);

  void FinishAddingWithResult(
      absl::optional<std::string> service,
      const mojom::BlockchainTokenPtr& token,
      bool result,
      absl::optional<mojom::WalletPinServiceErrorCode> response,
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
                        bool result);

  void OnTokenMetaDataReceived(absl::optional<std::string> service,
                               AddPinCallback callback,
                               mojom::BlockchainTokenPtr token,
                               const std::string& token_url,
                               const std::string& result,
                               mojom::ProviderError error,
                               const std::string& error_message);

  mojo::ReceiverSet<brave_wallet::mojom::WalletPinService> receivers_;
  mojo::RemoteSet<mojom::BraveWalletPinServiceObserver> observers_;

  // Prefs service is used to store list of pinned items
  PrefService* prefs_;

  // JsonRpcService is used to fetch token metadata
  JsonRpcService* json_rpc_service_;

  // ipfs::IpfsPinService* pin_service_;
  ipfs::IPFSRemotePinService* remote_pin_service_;
  ipfs::IpfsLocalPinService* local_pin_service_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_
