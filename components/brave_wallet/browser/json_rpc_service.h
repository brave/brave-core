/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/ens_resolver_task.h"
#include "brave/components/brave_wallet/browser/sns_resolver_task.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/unstoppable_domains_multichain_calls.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class PrefService;

namespace brave_wallet {

class EnsResolverTask;
class NftMetadataFetcher;
struct PendingAddChainRequest;
struct PendingSwitchChainRequest;

class JsonRpcService : public KeyedService, public mojom::JsonRpcService {
 public:
  JsonRpcService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      PrefService* local_state_prefs);
  // For testing:
  JsonRpcService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  // For testing:
  JsonRpcService();
  ~JsonRpcService() override;

  static void MigrateMultichainNetworks(PrefService* prefs);
  static void MigrateDeprecatedEthereumTestnets(PrefService* prefs);
  static void MigrateShowTestNetworksToggle(PrefService* prefs);

  mojo::PendingRemote<mojom::JsonRpcService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::JsonRpcService> receiver);

  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using StringResultCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using EnsGetContentHashCallback =
      base::OnceCallback<void(const std::vector<uint8_t>& content_hash,
                              bool require_offchain_consent,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using GetBlockNumberCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;
  using GetFeeHistoryCallback = base::OnceCallback<void(
      const std::vector<std::string>& base_fee_per_gas,
      const std::vector<double>& gas_used_ratio,
      const std::string& oldest_block,
      const std::vector<std::vector<std::string>>& reward,
      mojom::ProviderError error,
      const std::string& error_message)>;
  using EthGetLogsCallback =
      base::OnceCallback<void(const std::vector<Log>& logs,
                              base::Value rawlogs,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetBlockNumber(const std::string& chain_id,
                      GetBlockNumberCallback callback);
  void GetFeeHistory(const std::string& chain_id,
                     GetFeeHistoryCallback callback);

  void Request(const std::string& chain_id,
               const std::string& json_payload,
               bool auto_retry_on_network_change,
               base::Value id,
               mojom::CoinType coin,
               RequestCallback callback) override;

  void OnRequestResult(RequestCallback callback,
                       base::Value id,
                       APIRequestResult api_request_result);

  void GetBalance(const std::string& address,
                  mojom::CoinType coin,
                  const std::string& chaind_id,
                  GetBalanceCallback callback) override;
  void GetCode(const std::string& address,
               mojom::CoinType coin,
               const std::string& chain_id,
               GetCodeCallback callback) override;
  using GetFilBlockHeightCallback =
      base::OnceCallback<void(uint64_t height,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilBlockHeight(const std::string& chain_id,
                         GetFilBlockHeightCallback callback);
  using GetFilStateSearchMsgLimitedCallback =
      base::OnceCallback<void(int64_t code,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilStateSearchMsgLimited(
      const std::string& chain_id,
      const std::string& cid,
      uint64_t period,
      GetFilStateSearchMsgLimitedCallback callback);

  using GetTxCountCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using GetFilTxCountCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetEthTransactionCount(const std::string& chain_id,
                              const std::string& address,
                              GetTxCountCallback callback);
  void GetFilTransactionCount(const std::string& chain_id,
                              const std::string& address,
                              GetFilTxCountCallback callback);
  using SendFilecoinTransactionCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void SendFilecoinTransaction(const std::string& chain_id,
                               const std::string& signed_tx,
                               SendFilecoinTransactionCallback callback);

  using GetTxReceiptCallback =
      base::OnceCallback<void(TransactionReceipt result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetTransactionReceipt(const std::string& chain_id,
                             const std::string& tx_hash,
                             GetTxReceiptCallback callback);

  using SendRawTxCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void SendRawTransaction(const std::string& chain_id,
                          const std::string& signed_tx,
                          SendRawTxCallback callback);

  void GetERC20TokenBalance(const std::string& conract_address,
                            const std::string& address,
                            const std::string& chain_id,
                            GetERC20TokenBalanceCallback callback) override;
  void GetERC20TokenAllowance(const std::string& contract_address,
                              const std::string& owner_address,
                              const std::string& spender_address,
                              const std::string& chain_id,
                              GetERC20TokenAllowanceCallback callback) override;

  void GetERC20TokenBalances(
      const std::vector<std::string>& token_contract_addresses,
      const std::string& user_address,
      const std::string& chain_id,
      GetERC20TokenBalancesCallback callback) override;

  void UnstoppableDomainsResolveDns(
      const std::string& domain,
      UnstoppableDomainsResolveDnsCallback callback) override;

  void UnstoppableDomainsGetWalletAddr(
      const std::string& domain,
      mojom::BlockchainTokenPtr token,
      UnstoppableDomainsGetWalletAddrCallback callback) override;

  void EnsGetContentHash(const std::string& domain,
                         EnsGetContentHashCallback callback) override;

  void GetUnstoppableDomainsResolveMethod(
      GetUnstoppableDomainsResolveMethodCallback callback) override;
  void GetEnsResolveMethod(GetEnsResolveMethodCallback callback) override;
  void GetEnsOffchainLookupResolveMethod(
      GetEnsOffchainLookupResolveMethodCallback callback) override;
  void GetSnsResolveMethod(GetSnsResolveMethodCallback callback) override;

  void SetUnstoppableDomainsResolveMethod(mojom::ResolveMethod method) override;
  void SetEnsResolveMethod(mojom::ResolveMethod method) override;
  void SetEnsOffchainLookupResolveMethod(mojom::ResolveMethod method) override;
  void SetSnsResolveMethod(mojom::ResolveMethod method) override;

  void EnsGetEthAddr(const std::string& domain,
                     EnsGetEthAddrCallback callback) override;
  void SnsGetSolAddr(const std::string& domain,
                     SnsGetSolAddrCallback callback) override;

  void SnsResolveHost(const std::string& domain,
                      SnsResolveHostCallback callback) override;

  bool SetNetwork(const std::string& chain_id,
                  mojom::CoinType coin,
                  const absl::optional<::url::Origin>& origin);
  void SetNetwork(const std::string& chain_id,
                  mojom::CoinType coin,
                  const absl::optional<::url::Origin>& origin,
                  SetNetworkCallback callback) override;
  void GetNetwork(mojom::CoinType coin,
                  const absl::optional<::url::Origin>& origin,
                  GetNetworkCallback callback) override;
  mojom::NetworkInfoPtr GetNetworkSync(
      mojom::CoinType coin,
      const absl::optional<::url::Origin>& origin);

  void AddChain(mojom::NetworkInfoPtr chain,
                AddChainCallback callback) override;
  std::string AddEthereumChainForOrigin(mojom::NetworkInfoPtr chain,
                                        const url::Origin& origin);
  void AddEthereumChainRequestCompleted(const std::string& chain_id,
                                        bool approved) override;
  void RemoveChain(const std::string& chain_id,
                   mojom::CoinType coin,
                   RemoveChainCallback callback) override;

  std::string GetChainIdSync(mojom::CoinType coin,
                             const absl::optional<::url::Origin>& origin) const;
  void GetDefaultChainId(
      mojom::CoinType coin,
      mojom::JsonRpcService::GetDefaultChainIdCallback callback) override;
  void GetChainIdForOrigin(
      mojom::CoinType coin,
      const ::url::Origin& origin,
      mojom::JsonRpcService::GetChainIdForOriginCallback callback) override;
  void GetPendingAddChainRequests(
      GetPendingAddChainRequestsCallback callback) override;
  void GetPendingSwitchChainRequests(
      GetPendingSwitchChainRequestsCallback callback) override;
  std::vector<mojom::SwitchChainRequestPtr> GetPendingSwitchChainRequestsSync();
  void NotifySwitchChainRequestProcessed(const std::string& request_id,
                                         bool approved) override;
  void GetAllNetworks(mojom::CoinType coin,
                      GetAllNetworksCallback callback) override;
  void GetCustomNetworks(mojom::CoinType coin,
                         GetCustomNetworksCallback callback) override;
  void GetKnownNetworks(mojom::CoinType coin,
                        GetKnownNetworksCallback callback) override;
  void GetHiddenNetworks(mojom::CoinType coin,
                         GetHiddenNetworksCallback callback) override;
  void AddHiddenNetwork(mojom::CoinType coin,
                        const std::string& chain_id,
                        AddHiddenNetworkCallback callback) override;
  void RemoveHiddenNetwork(mojom::CoinType coin,
                           const std::string& chain_id,
                           RemoveHiddenNetworkCallback callback) override;
  std::string GetNetworkUrl(mojom::CoinType coin,
                            const absl::optional<::url::Origin>& origin) const;
  void GetNetworkUrl(
      mojom::CoinType coin,
      const absl::optional<::url::Origin>& origin,
      mojom::JsonRpcService::GetNetworkUrlCallback callback) override;

  void AddObserver(
      ::mojo::PendingRemote<mojom::JsonRpcServiceObserver> observer) override;

  using GetFilEstimateGasCallback =
      base::OnceCallback<void(const std::string& gas_premium,
                              const std::string& gas_fee_cap,
                              int64_t gas_limit,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilEstimateGas(const std::string& chain_id,
                         const std::string& from_address,
                         const std::string& to_address,
                         const std::string& gas_premium,
                         const std::string& gas_fee_cap,
                         int64_t gas_limit,
                         uint64_t nonce,
                         const std::string& max_fee,
                         const std::string& value,
                         GetFilEstimateGasCallback callback);

  using GetEstimateGasCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetEstimateGas(const std::string& chain_id,
                      const std::string& from_address,
                      const std::string& to_address,
                      const std::string& gas,
                      const std::string& gas_price,
                      const std::string& value,
                      const std::string& data,
                      GetEstimateGasCallback callback);

  using GetGasPriceCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetGasPrice(const std::string& chain_id, GetGasPriceCallback callback);

  using GetBaseFeePerGasCallback =
      base::OnceCallback<void(const std::string& base_fee_per_gas,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetBaseFeePerGas(const std::string& chain_id,
                        GetBaseFeePerGasCallback callback);

  using GetBlockByNumberCallback =
      base::OnceCallback<void(base::Value result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  // block_number can be kEthereumBlockTagLatest
  void GetBlockByNumber(const std::string& chain_id,
                        const std::string& block_number,
                        GetBlockByNumberCallback callback);

  void GetERC721OwnerOf(const std::string& contract,
                        const std::string& token_id,
                        const std::string& chain_id,
                        GetERC721OwnerOfCallback callback) override;

  void GetERC1155TokenBalance(const std::string& contract_address,
                              const std::string& owner_address,
                              const std::string& token_id,
                              const std::string& chain_id,
                              GetERC1155TokenBalanceCallback callback) override;

  void GetERC721TokenBalance(const std::string& contract_address,
                             const std::string& token_id,
                             const std::string& account_address,
                             const std::string& chain_id,
                             GetERC721TokenBalanceCallback callback) override;

  using GetEthTokenUriCallback =
      base::OnceCallback<void(const GURL& uri,
                              mojom::ProviderError error,
                              const std::string& error_message)>;

  void GetERC721Metadata(const std::string& contract_address,
                         const std::string& token_id,
                         const std::string& chain_id,
                         GetERC721MetadataCallback callback) override;

  void GetERC1155Metadata(const std::string& contract_address,
                          const std::string& token_id,
                          const std::string& chain_id,
                          GetERC1155MetadataCallback callback) override;
  // GetEthTokenUri should only be called after check whether the contract
  // supports the ERC721 or ERC1155 interface
  void GetEthTokenUri(const std::string& chain_id,
                      const std::string& contract_address,
                      const std::string& token_id,
                      const std::string& interface_id,
                      GetEthTokenUriCallback callback);

  void EthGetLogs(const std::string& chain_id,
                  base::Value::Dict filter_options,
                  EthGetLogsCallback callback);

  void OnEthGetLogs(EthGetLogsCallback callback,
                    APIRequestResult api_request_result);

  // Resets things back to the original state of BraveWalletService.
  // To be used when the Wallet is reset / erased
  void Reset();

  using GetSupportsInterfaceCallback =
      base::OnceCallback<void(bool is_supported,
                              mojom::ProviderError error,
                              const std::string& error_message)>;

  void GetSupportsInterface(const std::string& contract_address,
                            const std::string& interface_id,
                            const std::string& chain_id,
                            GetSupportsInterfaceCallback callback);

  using GetEthNftStandardCallback =
      base::OnceCallback<void(const absl::optional<std::string>& standard,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetEthNftStandard(const std::string& contract_address,
                         const std::string& chain_id,
                         const std::vector<std::string>& interfaces,
                         GetEthNftStandardCallback callback,
                         size_t index = 0);

  void OnGetEthNftStandard(const std::string& contract_address,
                           const std::string& chain_id,
                           const std::vector<std::string>& interfaces,
                           size_t index,
                           GetEthNftStandardCallback callback,
                           bool is_supported,
                           mojom::ProviderError error,
                           const std::string& error_message);

  void GetEthTokenSymbol(const std::string& contract_address,
                         const std::string& chain_id,
                         GetEthTokenSymbolCallback callback) override;

  void GetEthTokenDecimals(const std::string& contract_address,
                           const std::string& chain_id,
                           GetEthTokenDecimalsCallback callback) override;

  using SwitchEthereumChainRequestCallback =
      base::OnceCallback<void(mojom::ProviderError error,
                              const std::string& error_message)>;
  // return false when there is an error before processing request
  bool AddSwitchEthereumChainRequest(const std::string& chain_id,
                                     const url::Origin& origin,
                                     RequestCallback callback,
                                     base::Value id);

  void SetAPIRequestHelperForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  void SetSkipEthChainIdValidationForTesting(bool skipped) {
    skip_eth_chain_id_validation_for_testing_ = skipped;
  }

  // Solana JSON RPCs
  void GetSolanaBalance(const std::string& pubkey,
                        const std::string& chain_id,
                        GetSolanaBalanceCallback callback) override;
  void GetSPLTokenAccountBalance(
      const std::string& wallet_address,
      const std::string& token_mint_address,
      const std::string& chain_id,
      GetSPLTokenAccountBalanceCallback callback) override;
  void GetSolTokenMetadata(const std::string& chain_id,
                           const std::string& token_mint_address,
                           GetSolTokenMetadataCallback callback) override;
  void IsSolanaBlockhashValid(const std::string& chain_id,
                              const std::string& blockhash,
                              const absl::optional<std::string>& commitment,
                              IsSolanaBlockhashValidCallback callback) override;
  using SendSolanaTransactionCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void SendSolanaTransaction(
      const std::string& chain_id,
      const std::string& signed_tx,
      absl::optional<SolanaTransaction::SendOptions> send_options,
      SendSolanaTransactionCallback callback);
  using GetSolanaLatestBlockhashCallback =
      base::OnceCallback<void(const std::string& latest_blockhash,
                              uint64_t last_valid_block_height,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaLatestBlockhash(const std::string& chain_id,
                                GetSolanaLatestBlockhashCallback callback);
  using GetSolanaSignatureStatusesCallback = base::OnceCallback<void(
      const std::vector<absl::optional<SolanaSignatureStatus>>&
          signature_statuses,
      mojom::SolanaProviderError error,
      const std::string& error_message)>;
  void GetSolanaSignatureStatuses(const std::string& chain_id,
                                  const std::vector<std::string>& tx_signatures,
                                  GetSolanaSignatureStatusesCallback callback);
  using GetSolanaAccountInfoCallback =
      base::OnceCallback<void(absl::optional<SolanaAccountInfo> account_info,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaAccountInfo(const std::string& chain_id,
                            const std::string& pubkey,
                            GetSolanaAccountInfoCallback callback);
  using GetSolanaFeeForMessageCallback =
      base::OnceCallback<void(uint64_t fee,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaFeeForMessage(const std::string& chain_id,
                              const std::string& message,  // base64 encoded
                              GetSolanaFeeForMessageCallback callback);
  using GetSolanaBlockHeightCallback =
      base::OnceCallback<void(uint64_t block_height,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaBlockHeight(const std::string& chain_id,
                            GetSolanaBlockHeightCallback callback);

  using GetSolanaTokenAccountsByOwnerCallback = base::OnceCallback<void(
      const std::vector<SolanaAccountInfo>& token_accounts,
      mojom::SolanaProviderError error,
      const std::string& error_message)>;
  void GetSolanaTokenAccountsByOwner(
      const SolanaAddress& pubkey,
      const std::string& chain_id,
      GetSolanaTokenAccountsByOwnerCallback callback);

  void GetSPLTokenBalances(const std::string& pubkey,
                           const std::string& chain_id,
                           GetSPLTokenBalancesCallback callback) override;

  void AnkrGetAccountBalance(const std::string& account,
                             const std::vector<std::string>& chain_ids,
                             AnkrGetAccountBalanceCallback callback) override;

 private:
  void FireNetworkChanged(mojom::CoinType coin,
                          const std::string& chain_id,
                          const absl::optional<url::Origin>& origin);
  void FirePendingRequestCompleted(const std::string& chain_id,
                                   const std::string& error);
  bool HasAddChainRequestFromOrigin(const url::Origin& origin) const;
  bool HasSwitchChainRequestFromOrigin(const url::Origin& origin) const;
  void RemoveChainIdRequest(const std::string& chain_id);
  void OnGetFilStateSearchMsgLimited(
      GetFilStateSearchMsgLimitedCallback callback,
      const std::string& cid,
      APIRequestResult api_request_result);
  void OnGetFilBlockHeight(GetFilBlockHeightCallback callback,
                           APIRequestResult api_request_result);
  void OnGetBlockNumber(GetBlockNumberCallback callback,
                        APIRequestResult api_request_result);
  void OnGetFeeHistory(GetFeeHistoryCallback callback,
                       APIRequestResult api_request_result);
  void OnEthGetBalance(GetBalanceCallback callback,
                       APIRequestResult api_request_result);
  void OnFilGetBalance(GetBalanceCallback callback,
                       APIRequestResult api_request_result);
  void OnGetCode(GetCodeCallback callback, APIRequestResult api_request_result);
  void OnEthGetTransactionCount(GetTxCountCallback callback,
                                APIRequestResult api_request_result);
  void OnFilGetTransactionCount(GetFilTxCountCallback callback,
                                APIRequestResult api_request_result);
  void OnSendFilecoinTransaction(SendFilecoinTransactionCallback callback,
                                 APIRequestResult api_request_result);
  void OnGetTransactionReceipt(GetTxReceiptCallback callback,
                               APIRequestResult api_request_result);
  void OnSendRawTransaction(SendRawTxCallback callback,
                            APIRequestResult api_request_result);
  void OnGetERC20TokenBalance(GetERC20TokenBalanceCallback callback,
                              APIRequestResult api_request_result);
  void OnGetERC20TokenAllowance(GetERC20TokenAllowanceCallback callback,
                                APIRequestResult api_request_result);
  void OnGetERC20TokenBalances(
      const std::vector<std::string>& token_contract_addresses,
      GetERC20TokenBalancesCallback callback,
      APIRequestResult api_request_result);
  void OnUnstoppableDomainsResolveDns(const std::string& domain,
                                      const std::string& chain_id,
                                      APIRequestResult api_request_result);
  void OnUnstoppableDomainsGetWalletAddr(
      const unstoppable_domains::WalletAddressKey& key,
      const std::string& chain_id,
      APIRequestResult api_request_result);
  void EnsRegistryGetResolver(const std::string& domain,
                              StringResultCallback callback);
  void OnEnsRegistryGetResolver(StringResultCallback callback,
                                APIRequestResult api_request_result);
  void ContinueEnsGetContentHash(const std::string& domain,
                                 EnsGetContentHashCallback callback,
                                 const std::string& resolver_address,
                                 mojom::ProviderError error,
                                 const std::string& error_message);
  void OnEnsGetContentHash(EnsGetContentHashCallback callback,
                           APIRequestResult api_request_result);
  void ContinueEnsGetEthAddr(const std::string& domain,
                             EnsGetEthAddrCallback callback,
                             const std::string& resolver_address,
                             mojom::ProviderError error,
                             const std::string& error_message);
  void OnEnsGetEthAddrTaskDone(
      EnsResolverTask* task,
      absl::optional<EnsResolverTaskResult> task_result,
      absl::optional<EnsResolverTaskError> error);
  void OnEnsGetContentHashTaskDone(
      EnsResolverTask* task,
      absl::optional<EnsResolverTaskResult> task_result,
      absl::optional<EnsResolverTaskError> error);
  void OnSnsGetSolAddrTaskDone(
      SnsResolverTask* task,
      absl::optional<SnsResolverTaskResult> task_result,
      absl::optional<SnsResolverTaskError> error);
  void OnSnsResolveHostTaskDone(
      SnsResolverTask* task,
      absl::optional<SnsResolverTaskResult> task_result,
      absl::optional<SnsResolverTaskError> error);
  void OnEnsGetEthAddr(EnsGetEthAddrCallback callback,
                       APIRequestResult api_request_result);
  void OnGetFilEstimateGas(GetFilEstimateGasCallback callback,
                           APIRequestResult api_request_result);
  void OnGetEstimateGas(GetEstimateGasCallback callback,
                        APIRequestResult api_request_result);
  void OnGetGasPrice(GetGasPriceCallback callback,
                     APIRequestResult api_request_result);
  void OnGetBaseFeePerGas(GetBaseFeePerGasCallback callback,
                          APIRequestResult api_request_result);
  void OnGetBlockByNumber(GetBlockByNumberCallback callback,
                          APIRequestResult api_request_result);

  void MaybeUpdateIsEip1559(const std::string& chain_id);
  void UpdateIsEip1559(const std::string& chain_id,
                       const std::string& base_fee_per_gas,
                       mojom::ProviderError error,
                       const std::string& error_message);

  void RequestInternal(
      const std::string& json_payload,
      bool auto_retry_on_network_change,
      const GURL& network_url,
      RequestIntermediateCallback callback,
      APIRequestHelper::ResponseConversionCallback conversion_callback);
  void OnEthChainIdValidatedForOrigin(const std::string& chain_id,
                                      const GURL& rpc_url,
                                      APIRequestResult api_request_result);

  void OnEthChainIdValidated(mojom::NetworkInfoPtr chain,
                             const GURL& rpc_url,
                             AddChainCallback callback,
                             APIRequestResult api_request_result);

  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, IsValidDomain);
  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, IsValidUnstoppableDomain);
  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, Reset);
  friend class JsonRpcServiceUnitTest;

  static bool IsValidDomain(const std::string& domain);
  static bool IsValidUnstoppableDomain(const std::string& domain);

  void OnGetERC721OwnerOf(GetERC721OwnerOfCallback callback,
                          APIRequestResult api_request_result);
  void ContinueGetERC721TokenBalance(const std::string& account_address,
                                     GetERC721TokenBalanceCallback callback,
                                     const std::string& owner_address,
                                     mojom::ProviderError error,
                                     const std::string& error_message);

  void OnGetEthTokenUri(GetEthTokenUriCallback callback,
                        const APIRequestResult api_request_result);

  void OnGetSupportsInterface(GetSupportsInterfaceCallback callback,
                              APIRequestResult api_request_result);

  void OnGetEthTokenSymbol(GetEthTokenSymbolCallback callback,
                           APIRequestResult api_request_result);

  void OnGetEthTokenDecimals(GetEthTokenDecimalsCallback callback,
                             APIRequestResult api_request_result);
  // Solana
  void OnGetSolanaBalance(GetSolanaBalanceCallback callback,
                          APIRequestResult api_request_result);
  void OnGetSPLTokenAccountBalance(GetSPLTokenAccountBalanceCallback callback,

                                   APIRequestResult api_request_result);
  void OnSendSolanaTransaction(SendSolanaTransactionCallback callback,
                               APIRequestResult api_request_result);
  void OnGetSolanaLatestBlockhash(GetSolanaLatestBlockhashCallback callback,
                                  APIRequestResult api_request_result);
  void OnGetSolanaSignatureStatuses(GetSolanaSignatureStatusesCallback callback,
                                    APIRequestResult api_request_result);
  void OnGetSolanaAccountInfo(GetSolanaAccountInfoCallback callback,
                              APIRequestResult api_request_result);
  void OnGetSolanaFeeForMessage(GetSolanaFeeForMessageCallback callback,
                                APIRequestResult api_request_result);
  void OnGetSolanaBlockHeight(GetSolanaBlockHeightCallback callback,
                              APIRequestResult api_request_result);
  void OnGetSolanaTokenAccountsByOwner(
      GetSolanaTokenAccountsByOwnerCallback callback,
      APIRequestResult api_request_result);
  void OnIsSolanaBlockhashValid(IsSolanaBlockhashValidCallback callback,
                                APIRequestResult api_request_result);

  void OnGetSPLTokenBalances(GetSPLTokenBalancesCallback callback,
                             APIRequestResult api_request_result);

  void OnAnkrGetAccountBalance(AnkrGetAccountBalanceCallback callback,
                               APIRequestResult api_request_result);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  std::unique_ptr<APIRequestHelper> api_request_helper_ens_offchain_;
  base::flat_map<std::string, PendingAddChainRequest>
      add_chain_pending_requests_;
  base::flat_map<std::string, PendingSwitchChainRequest>
      pending_switch_chain_requests_;

  unstoppable_domains::MultichainCalls<unstoppable_domains::WalletAddressKey,
                                       std::string>
      ud_get_eth_addr_calls_;
  unstoppable_domains::MultichainCalls<std::string, absl::optional<GURL>>
      ud_resolve_dns_calls_;

  mojo::RemoteSet<mojom::JsonRpcServiceObserver> observers_;

  EnsResolverTaskContainer<EnsGetEthAddrCallback> ens_get_eth_addr_tasks_;
  EnsResolverTaskContainer<EnsGetContentHashCallback>
      ens_get_content_hash_tasks_;

  SnsResolverTaskContainer<SnsGetSolAddrCallback> sns_get_sol_addr_tasks_;
  SnsResolverTaskContainer<SnsResolveHostCallback> sns_resolve_host_tasks_;

  bool skip_eth_chain_id_validation_for_testing_ = false;

  mojo::ReceiverSet<mojom::JsonRpcService> receivers_;
  const raw_ptr<PrefService> prefs_ = nullptr;
  const raw_ptr<PrefService> local_state_prefs_ = nullptr;
  std::unique_ptr<NftMetadataFetcher> nft_metadata_fetcher_;
  base::WeakPtrFactory<JsonRpcService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_
