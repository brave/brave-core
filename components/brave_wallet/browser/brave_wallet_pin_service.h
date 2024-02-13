// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_

#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
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
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_wallet {

class ContentTypeChecker {
 public:
  ContentTypeChecker(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~ContentTypeChecker();

  virtual void CheckContentTypeSupported(
      const std::string& cid,
      base::OnceCallback<void(std::optional<bool>)> callback);

 protected:
  // For tests
  ContentTypeChecker();

 private:
  using UrlLoaderList = std::list<std::unique_ptr<network::SimpleURLLoader>>;

  void OnHeadersFetched(UrlLoaderList::iterator iterator,
                        base::OnceCallback<void(std::optional<bool>)> callback,
                        scoped_refptr<net::HttpResponseHeaders> headers);

  raw_ptr<PrefService> pref_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  // List of requests are actively being sent.
  UrlLoaderList loaders_in_progress_;

  base::WeakPtrFactory<ContentTypeChecker> weak_ptr_factory_{this};
};

/**
 * At the moment only local pinning is supported so use std::nullopt
 * for optional service argument.
 */
class BraveWalletPinService : public KeyedService,
                              public brave_wallet::mojom::WalletPinService,
                              public ipfs::IpfsServiceObserver {
 public:
  BraveWalletPinService(
      PrefService* prefs,
      JsonRpcService* service,
      ipfs::IpfsLocalPinService* local_pin_service,
      IpfsService* ipfs_service,
      std::unique_ptr<ContentTypeChecker> content_type_checker);
  ~BraveWalletPinService() override;

  virtual void Restore();
  virtual void Reset(base::OnceCallback<void(bool)> callback);

  mojo::PendingRemote<mojom::WalletPinService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::WalletPinService> receiver);

  static std::optional<std::string> GetTokenPrefPath(
      const std::optional<std::string>& service,
      const mojom::BlockchainTokenPtr& token);
  static mojom::BlockchainTokenPtr TokenFromPrefPath(const std::string& path);
  static std::optional<std::string> ServiceFromPrefPath(
      const std::string& path);
  static std::string StatusToString(const mojom::TokenPinStatusCode& status);
  static std::string ErrorCodeToString(
      const mojom::WalletPinServiceErrorCode& error_code);
  static bool IsTokenSupportedForPinning(
      const mojom::BlockchainTokenPtr& token);

  // WalletPinService
  void AddObserver(::mojo::PendingRemote<mojom::BraveWalletPinServiceObserver>
                       observer) override;
  void AddPin(mojom::BlockchainTokenPtr token,
              const std::optional<std::string>& service,
              AddPinCallback callback) override;
  void RemovePin(mojom::BlockchainTokenPtr token,
                 const std::optional<std::string>& service,
                 RemovePinCallback callback) override;
  void GetTokenStatus(mojom::BlockchainTokenPtr token,
                      GetTokenStatusCallback callback) override;
  void Validate(mojom::BlockchainTokenPtr token,
                const std::optional<std::string>& service,
                ValidateCallback callback) override;
  void IsLocalNodeRunning(IsLocalNodeRunningCallback callback) override;
  void IsTokenSupported(mojom::BlockchainTokenPtr token,
                        IsTokenSupportedCallback callback) override;

  virtual void MarkAsPendingForPinning(
      const mojom::BlockchainTokenPtr& token,
      const std::optional<std::string>& service);
  virtual void MarkAsPendingForUnpinning(
      const mojom::BlockchainTokenPtr& token,
      const std::optional<std::string>& service);

  virtual mojom::TokenPinStatusPtr GetTokenStatus(
      const std::optional<std::string>& service,
      const mojom::BlockchainTokenPtr& token);
  mojom::TokenPinStatusPtr GetTokenStatus(const std::string& path);
  virtual std::optional<base::Time> GetLastValidateTime(
      const std::optional<std::string>& service,
      const mojom::BlockchainTokenPtr& token);
  // Returns list of known tokens for the provided pinning service.
  // Tokens are returned in the format of string path.
  // See BraveWalletPinService::GetTokenPrefPath.
  virtual std::set<std::string> GetTokens(
      const std::optional<std::string>& service);

  size_t GetPinnedTokensCount();

 protected:
  // For testing
  BraveWalletPinService();

 private:
  bool AddToken(const std::optional<std::string>& service,
                const mojom::BlockchainTokenPtr& token,
                const std::vector<std::string>& cids);
  bool RemoveToken(const std::optional<std::string>& service,
                   const mojom::BlockchainTokenPtr& token);
  bool SetTokenStatus(const std::optional<std::string>& service,
                      const mojom::BlockchainTokenPtr& token,
                      mojom::TokenPinStatusCode,
                      const mojom::PinErrorPtr& error);

  std::optional<std::vector<std::string>> ResolvePinItems(
      const std::optional<std::string>& service,
      const mojom::BlockchainTokenPtr& token);

  void OnPinsRemoved(std::optional<std::string> service,
                     RemovePinCallback callback,
                     mojom::BlockchainTokenPtr token,
                     bool result);
  void OnTokenPinned(std::optional<std::string> service,
                     AddPinCallback callback,
                     mojom::BlockchainTokenPtr,
                     bool result);
  void OnTokenValidated(std::optional<std::string> service,
                        ValidateCallback callback,
                        mojom::BlockchainTokenPtr,
                        std::optional<bool> result);

  void ProcessTokenMetadata(const std::optional<std::string>& service,
                            const mojom::BlockchainTokenPtr& token,
                            const std::string& token_url,
                            const std::string& result,
                            AddPinCallback callback);

  void OnTokenMetaDataReceived(std::optional<std::string> service,
                               AddPinCallback callback,
                               mojom::BlockchainTokenPtr token,
                               const std::string& token_url,
                               const std::string& result,
                               mojom::ProviderError error,
                               const std::string& error_message);
  void OnContentTypeChecked(std::optional<std::string> service,
                            mojom::BlockchainTokenPtr token,
                            std::vector<std::string> cids,
                            AddPinCallback callback,
                            std::optional<bool> result);

  void OnSolTokenMetaDataReceived(std::optional<std::string> service,
                                  AddPinCallback callback,
                                  mojom::BlockchainTokenPtr token,
                                  const std::string& token_url,
                                  const std::string& result,
                                  mojom::SolanaProviderError error,
                                  const std::string& error_message);

  // ipfs::IpfsServiceObserver
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnIpfsShutdown() override;

  void OnResetLocalPinService(base::OnceCallback<void(bool)> callback,
                              bool result);

  mojo::ReceiverSet<brave_wallet::mojom::WalletPinService> receivers_;
  mojo::RemoteSet<mojom::BraveWalletPinServiceObserver> observers_;

  // Prefs service is used to store list of pinned items
  raw_ptr<PrefService> prefs_ = nullptr;

  // JsonRpcService is used to fetch token metadata
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<ipfs::IpfsLocalPinService> local_pin_service_ = nullptr;
  raw_ptr<IpfsService> ipfs_service_ = nullptr;
  std::unique_ptr<ContentTypeChecker> content_type_checker_;

  base::WeakPtrFactory<BraveWalletPinService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PIN_SERVICE_H_
