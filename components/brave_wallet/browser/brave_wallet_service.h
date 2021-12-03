/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace brave_wallet {

constexpr char kBraveWalletDailyHistogramName[] = "Brave.Wallet.UsageDaily";
constexpr char kBraveWalletWeeklyHistogramName[] = "Brave.Wallet.UsageWeekly";
constexpr char kBraveWalletMonthlyHistogramName[] = "Brave.Wallet.UsageMonthly";

class KeyringService;
class JsonRpcService;
class EthTxService;

class BraveWalletService : public KeyedService,
                           public mojom::BraveWalletService,
                           public BraveWalletServiceDelegate::Observer {
 public:
  using SignMessageRequestCallback =
      base::OnceCallback<void(bool, const std::string&, const std::string&)>;
  using AddSuggestTokenCallback =
      base::OnceCallback<void(bool, mojom::ProviderError, const std::string&)>;

  explicit BraveWalletService(
      std::unique_ptr<BraveWalletServiceDelegate> delegate,
      KeyringService* keyring_service,
      JsonRpcService* json_rpc_service,
      EthTxService* eth_tx_service,
      PrefService* prefs);
  ~BraveWalletService() override;

  BraveWalletService(const BraveWalletService&) = delete;
  BraveWalletService& operator=(const BraveWalletService&) = delete;

  mojo::PendingRemote<mojom::BraveWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveWalletService> receiver);

  static void MigrateUserAssetEthContractAddress(PrefService* prefs);

  // mojom::BraveWalletService:
  void AddObserver(::mojo::PendingRemote<mojom::BraveWalletServiceObserver>
                       observer) override;

  void GetUserAssets(const std::string& chain_id,
                     GetUserAssetsCallback callback) override;
  void AddUserAsset(mojom::BlockchainTokenPtr token,
                    const std::string& chain_id,
                    AddUserAssetCallback callback) override;
  void RemoveUserAsset(mojom::BlockchainTokenPtr token,
                       const std::string& chain_id,
                       RemoveUserAssetCallback callback) override;
  void SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                           const std::string& chain_id,
                           bool visible,
                           SetUserAssetVisibleCallback callback) override;
  void IsExternalWalletInstalled(mojom::ExternalWalletType,
                                 IsExternalWalletInstalledCallback) override;
  void IsExternalWalletInitialized(
      mojom::ExternalWalletType,
      IsExternalWalletInitializedCallback) override;
  void ImportFromExternalWallet(
      mojom::ExternalWalletType type,
      const std::string& password,
      const std::string& new_password,
      ImportFromExternalWalletCallback callback) override;

  void GetDefaultWallet(GetDefaultWalletCallback callback) override;
  void SetDefaultWallet(mojom::DefaultWallet default_wallet) override;
  void GetDefaultBaseCurrency(GetDefaultBaseCurrencyCallback callback) override;
  void SetDefaultBaseCurrency(const std::string& currency) override;
  void GetDefaultBaseCryptocurrency(
      GetDefaultBaseCryptocurrencyCallback callback) override;
  void SetDefaultBaseCryptocurrency(const std::string& cryptocurrency) override;
  void AddEthereumPermission(const std::string& origin,
                             const std::string& account,
                             AddEthereumPermissionCallback callback) override;
  void HasEthereumPermission(const std::string& origin,
                             const std::string& account,
                             HasEthereumPermissionCallback callback) override;
  void ResetEthereumPermission(
      const std::string& origin,
      const std::string& account,
      ResetEthereumPermissionCallback callback) override;
  void GetActiveOrigin(GetActiveOriginCallback callback) override;
  void GetPendingSignMessageRequests(
      GetPendingSignMessageRequestsCallback callback) override;
  void NotifySignMessageRequestProcessed(bool approved, int id) override;
  void NotifySignMessageHardwareRequestProcessed(
      bool approved,
      int id,
      const std::string& signature,
      const std::string& error) override;
  void GetPendingAddSuggestTokenRequests(
      GetPendingAddSuggestTokenRequestsCallback callback) override;
  void NotifyAddSuggestTokenRequestsProcessed(
      bool approved,
      const std::vector<std::string>& contract_addresses) override;

  // BraveWalletServiceDelegate::Observer:
  void OnActiveOriginChanged(const std::string& origin) override;

  // Resets things back to the original state of BraveWalletService.
  // To be used when the Wallet is reset / erased
  void Reset() override;

  void RecordWalletUsage(base::Time wallet_last_used);

  void AddSignMessageRequest(mojom::SignMessageRequestPtr request,
                             SignMessageRequestCallback callback);
  void AddSuggestTokenRequest(mojom::AddSuggestTokenRequestPtr request,
                              AddSuggestTokenCallback callback);

 private:
  friend class BraveWalletProviderImplUnitTest;
  friend class BraveWalletServiceUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, GetChecksumAddress);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, AddSuggestToken);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, GetUserAsset);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, ImportFromMetaMask);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, Reset);

  void OnDefaultWalletChanged();
  void OnDefaultBaseCurrencyChanged();
  void OnDefaultBaseCryptocurrencyChanged();
  void OnNetworkListChanged();

  absl::optional<std::string> GetChecksumAddress(
      const std::string& contract_address,
      const std::string& chain_id);
  void OnWalletUnlockPreferenceChanged(const std::string& pref_name);
  void OnP3ATimerFired();

  void OnGetImportInfo(
      const std::string& new_password,
      base::OnceCallback<void(bool, const absl::optional<std::string>&)>
          callback,
      bool result,
      ImportInfo info,
      ImportError error);

  bool AddUserAsset(mojom::BlockchainTokenPtr token,
                    const std::string& chain_id);
  bool RemoveUserAsset(mojom::BlockchainTokenPtr token,
                       const std::string& chain_id);
  bool SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                           const std::string& chain_id,
                           bool visible);
  mojom::BlockchainTokenPtr GetUserAsset(const std::string& contract_address,
                                         const std::string& token_id,
                                         bool is_erc721,
                                         const std::string& chain_id);
  void OnNetworkChanged();
  void CancelAllSuggestedTokenCallbacks();
  void CancelAllSignMessageCallbacks();

  base::circular_deque<mojom::SignMessageRequestPtr> sign_message_requests_;
  base::circular_deque<SignMessageRequestCallback> sign_message_callbacks_;
  base::flat_map<std::string, AddSuggestTokenCallback>
      add_suggest_token_callbacks_;
  base::flat_map<std::string, mojom::AddSuggestTokenRequestPtr>
      add_suggest_token_requests_;
  mojo::RemoteSet<mojom::BraveWalletServiceObserver> observers_;
  std::unique_ptr<BraveWalletServiceDelegate> delegate_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<EthTxService> eth_tx_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  mojo::ReceiverSet<mojom::BraveWalletService> receivers_;
  PrefChangeRegistrar pref_change_registrar_;
  base::RepeatingTimer p3a_periodic_timer_;
  base::WeakPtrFactory<BraveWalletService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
