/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_provider_impl.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/to_vector.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/siwe_message_parser.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "crypto/random.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

constexpr char kEthAccounts[] = "eth_accounts";
constexpr char kEthCoinbase[] = "eth_coinbase";
constexpr char kEthRequestAccounts[] = "eth_requestAccounts";
constexpr char kEthSendTransaction[] = "eth_sendTransaction";
constexpr char kEthSignTransaction[] = "eth_signTransaction";
constexpr char kEthSendRawTransaction[] = "eth_sendRawTransaction";
constexpr char kEthSign[] = "eth_sign";
constexpr char kPersonalSign[] = "personal_sign";
constexpr char kPersonalEcRecover[] = "personal_ecRecover";
constexpr char kEthGetEncryptionPublicKey[] = "eth_getEncryptionPublicKey";
constexpr char kEthDecrypt[] = "eth_decrypt";
constexpr char kWalletWatchAsset[] = "wallet_watchAsset";
constexpr char kMetamaskWatchAsset[] = "metamask_watchAsset";
constexpr char kWeb3ClientVersion[] = "web3_clientVersion";
constexpr char kEthSubscribe[] = "eth_subscribe";
constexpr char kEthSubscribeNewHeads[] = "newHeads";
constexpr char kEthSubscribeLogs[] = "logs";
constexpr char kEthUnsubscribe[] = "eth_unsubscribe";

constexpr char kEthSignTypedDataV3[] = "eth_signTypedData_v3";
constexpr char kEthSignTypedDataV4[] = "eth_signTypedData_v4";
constexpr char kAddEthereumChainMethod[] = "wallet_addEthereumChain";
constexpr char kSwitchEthereumChainMethod[] = "wallet_switchEthereumChain";
constexpr char kRequestPermissionsMethod[] = "wallet_requestPermissions";
constexpr char kGetPermissionsMethod[] = "wallet_getPermissions";

constexpr auto kMethodsRequireInteraction =
    base::MakeFixedFlatSet<std::string_view>(
        {kEthRequestAccounts, kAddEthereumChainMethod,
         kSwitchEthereumChainMethod, kEthSendTransaction, kEthSignTransaction,
         kEthSign, kPersonalSign, kPersonalEcRecover, kEthSignTypedDataV3,
         kEthSignTypedDataV4, kEthGetEncryptionPublicKey, kEthDecrypt,
         kWalletWatchAsset, kRequestPermissionsMethod});

void RejectUnsupportedMethod(
    base::Value id,
    mojom::EthereumProvider::RequestCallback callback) {
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUnsupportedMethod,
      l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), true, "", false));
}

void RejectInvalidParams(base::Value id,
                         mojom::EthereumProvider::RequestCallback callback) {
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kInvalidParams,
      l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), true, "", false));
}

void RejectAccountNotAuthed(base::Value id,
                            mojom::EthereumProvider::RequestCallback callback) {
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUnauthorized,
      l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), true, "", false));
}

void RejectMismatchError(base::Value id,
                         const std::string& err_msg,
                         mojom::EthereumProvider::RequestCallback callback) {
  base::Value formed_response =
      GetProviderErrorDictionary(mojom::ProviderError::kInternalError, err_msg);
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), true, "", false));
}

bool IsTypedDataStructure(const base::ListValue& params_list) {
  return (ParseEthSignTypedDataParams(params_list,
                                      EthSignTypedDataHelper::Version::kV4) ||
          ParseEthSignTypedDataParams(params_list,
                                      EthSignTypedDataHelper::Version::kV3));
}

base::DictValue GetJsonRpcRequest(const std::string& method,
                                  base::ListValue params) {
  base::DictValue dictionary;
  dictionary.Set("jsonrpc", "2.0");
  dictionary.Set("method", method);
  dictionary.Set("params", std::move(params));
  dictionary.Set("id", "1");
  return dictionary;
}

// Common logic for filtering the list of accounts based on the selected account
std::vector<std::string> FilterAccountsByPermissionIdentifier(
    const std::vector<std::string>& identifiers,
    const mojom::AccountInfoPtr& selected_account) {
  // If one of the accounts matches the selected account, then only
  // return that account.  This is for webcompat reasons.
  // Some Dapps select the first account in the list, and some the
  // last. So having only 1 item returned here makes it work for
  // all Dapps.
  std::vector<std::string> filtered_identifiers;
  for (const auto& identifier : identifiers) {
    if (selected_account &&
        base::EqualsCaseInsensitiveASCII(
            identifier,
            GetAccountPermissionIdentifier(selected_account->account_id))) {
      filtered_identifiers.clear();
      filtered_identifiers.push_back(identifier);
      break;
    } else {
      filtered_identifiers.push_back(identifier);
    }
  }
  return filtered_identifiers;
}

}  // namespace

EthereumProviderImpl::EthereumProviderImpl(
    HostContentSettingsMap* host_content_settings_map,
    BraveWalletService* brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map),
      delegate_(std::move(delegate)),
      brave_wallet_service_(brave_wallet_service),
      json_rpc_service_(brave_wallet_service->json_rpc_service()),
      tx_service_(brave_wallet_service->tx_service()),
      keyring_service_(brave_wallet_service->keyring_service()),
      eth_block_tracker_(json_rpc_service_),
      eth_logs_tracker_(json_rpc_service_),
      prefs_(prefs) {
  SetupMethodHandlers();

  DCHECK(json_rpc_service_);
  json_rpc_service_->AddObserver(
      rpc_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(tx_service_);
  tx_service_->AddObserver(tx_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(keyring_service_);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
  host_content_settings_map_->AddObserver(this);

  // Get the current so we can compare for changed events
  if (delegate_) {
    UpdateKnownAccounts();
  }

  eth_block_tracker_.AddObserver(this);
  eth_logs_tracker_.AddObserver(this);
}

EthereumProviderImpl::~EthereumProviderImpl() {
  host_content_settings_map_->RemoveObserver(this);
  eth_block_tracker_.RemoveObserver(this);
  eth_logs_tracker_.RemoveObserver(this);
}

void EthereumProviderImpl::AddEthereumChain(base::ListValue params,
                                            RequestCallback callback,
                                            base::Value id) {
  bool reject = false;

  if (params.empty()) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  auto chain = ParseEip3085Payload(params.front());
  if (!chain) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }
  std::string chain_id_lower = base::ToLowerASCII(chain->chain_id);

  // Check if we already have the chain
  if (brave_wallet_service_->network_manager()
          ->GetNetworkURL(chain_id_lower, mojom::CoinType::ETH)
          .is_valid()) {
    if (base::CompareCaseInsensitiveASCII(
            json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH,
                                              delegate_->GetOrigin()),
            chain_id_lower) != 0) {
      SwitchEthereumChain(chain_id_lower, std::move(callback), std::move(id));
      return;
    }

    reject = false;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), base::Value(), reject, "", true));
    return;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  // https://github.com/brave/brave-browser/issues/17637
  if (chain_id_lower.empty() || chain->rpc_endpoints.empty() ||
      chain->chain_name.empty()) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }
  if (chain_callbacks_.contains(chain_id_lower)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", true));
    return;
  }
  if (!delegate_) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", true));
    return;
  }
  auto error_message = json_rpc_service_->AddEthereumChainForOrigin(
      std::move(chain), delegate_->GetOrigin());

  if (!error_message.empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest, error_message);
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", true));
    return;
  }

  chain_callbacks_[chain_id_lower] = std::move(callback);
  chain_ids_[chain_id_lower] = std::move(id);
  delegate_->ShowPanel();
}

void EthereumProviderImpl::SwitchEthereumChain(const std::string& chain_id,
                                               RequestCallback callback,
                                               base::Value id) {
  // Only show bubble when there is no immediate error
  if (json_rpc_service_->AddSwitchEthereumChainRequest(
          chain_id, delegate_->GetOrigin(), std::move(callback),
          std::move(id))) {
    delegate_->ShowPanel();
  }
}

void EthereumProviderImpl::SendOrSignTransactionInternal(
    RequestCallback callback,
    base::Value id,
    const base::ListValue& params,
    bool sign_only) {
  url::Origin origin = delegate_->GetOrigin();
  mojom::NetworkInfoPtr chain =
      json_rpc_service_->GetNetworkSync(mojom::CoinType::ETH, origin);

  bool reject = false;
  if (!chain) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    base::Value formed_response = GetProviderErrorDictionary(code, message);
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", false));
    return;
  }

  std::string from;
  mojom::TxData1559Ptr tx_data_1559 =
      ParseEthTransaction1559Params(params, from);
  if (!tx_data_1559) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    base::Value formed_response = GetProviderErrorDictionary(code, message);
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", false));
    return;
  }
  tx_data_1559->base_data->sign_only = sign_only;

  const auto account_id = FindAuthenticatedAccountByAddress(from, id, callback);
  if (!account_id) {
    return;
  }
  const bool is_eip_1559_network =
      brave_wallet_service_->network_manager()->IsEip1559Chain(chain->chain_id);
  if (is_eip_1559_network && ShouldCreate1559Tx(*tx_data_1559)) {
    // Set chain_id to current chain_id.
    tx_data_1559->chain_id = chain->chain_id;
    tx_service_->AddUnapprovedTransactionWithOrigin(
        mojom::TxDataUnion::NewEthTxData1559(std::move(tx_data_1559)),
        chain->chain_id, account_id.Clone(), nullptr, origin,
        base::BindOnce(&EthereumProviderImpl::OnAddUnapprovedTransactionAdapter,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id)));
  } else {
    tx_service_->AddUnapprovedTransactionWithOrigin(
        mojom::TxDataUnion::NewEthTxData(std::move(tx_data_1559->base_data)),
        chain->chain_id, account_id.Clone(), nullptr, origin,
        base::BindOnce(&EthereumProviderImpl::OnAddUnapprovedTransactionAdapter,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id)));
  }
}

void EthereumProviderImpl::IsLocked(IsLockedCallback callback) {
  keyring_service_->IsLocked(std::move(callback));
  delegate_->WalletInteractionDetected();
}

// AddUnapprovedTransaction is a different return type from
// AddAndApproveTransaction so we need to use an adapter callback that passes
// through.
void EthereumProviderImpl::OnAddUnapprovedTransactionAdapter(
    RequestCallback callback,
    base::Value id,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  OnAddUnapprovedTransaction(std::move(callback), std::move(id), tx_meta_id,
                             success ? mojom::ProviderError::kSuccess
                                     : mojom::ProviderError::kInternalError,
                             success ? "" : error_message);
}

void EthereumProviderImpl::OnAddUnapprovedTransaction(
    RequestCallback callback,
    base::Value id,
    const std::string& tx_meta_id,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error == mojom::ProviderError::kSuccess) {
    add_tx_callbacks_[tx_meta_id] = std::move(callback);
    add_tx_ids_[tx_meta_id] = std::move(id);
    delegate_->ShowPanel();
  } else {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    bool reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", false));
  }
}

void EthereumProviderImpl::SignMessage(const std::string& address,
                                       const std::string& message,
                                       RequestCallback callback,
                                       base::Value id) {
  if (!IsValidHexString(message)) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  const auto account_id =
      FindAuthenticatedAccountByAddress(address, id, callback);
  if (!account_id) {
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }
  std::string message_str(message_bytes.begin(), message_bytes.end());
  mojom::SignDataUnionPtr sign_data = nullptr;

  SIWEMessageParser parser;
  auto siwe_message = parser.Parse(message_str);

  // Non SIWE compliant message will fallback to eip-191(Signed Data Standard)
  if (siwe_message) {
    const std::string chain_id_hex = json_rpc_service_->GetChainIdSync(
        mojom::CoinType::ETH, delegate_->GetOrigin());
    uint64_t chain_id;
    if (!base::HexStringToUInt64(chain_id_hex, &chain_id) ||
        chain_id != siwe_message->chain_id) {
      const std::string& incorrect_chain_id =
          base::NumberToString(siwe_message->chain_id);
      brave_wallet_service_->AddSignMessageError(mojom::SignMessageError::New(
          GenerateRandomHexString(), MakeOriginInfo(delegate_->GetOrigin()),
          mojom::SignMessageErrorType::kChainIdMismatched,
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_MISMATCH_ERR,
              l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_NETWORK),
              base::StrCat(
                  {l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_CHAIN_ID), u": ",
                   base::ASCIIToUTF16(incorrect_chain_id)})),
          incorrect_chain_id));
      delegate_->ShowPanel();
      return RejectMismatchError(
          std::move(id),
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_CHAIN_ID_MISMATCH,
              base::ASCIIToUTF16(incorrect_chain_id)),
          std::move(callback));
    }
    if (EthAddress::FromHex(address) !=
        EthAddress::FromHex(siwe_message->address)) {
      brave_wallet_service_->AddSignMessageError(mojom::SignMessageError::New(
          GenerateRandomHexString(), MakeOriginInfo(delegate_->GetOrigin()),
          mojom::SignMessageErrorType::kAccountMismatched,
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_MISMATCH_ERR,
              l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_ACCOUNT),
              base::ASCIIToUTF16(siwe_message->address)),
          std::nullopt));
      delegate_->ShowPanel();
      return RejectMismatchError(
          std::move(id),
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_ACCOUNT_MISMATCH,
              base::ASCIIToUTF16(siwe_message->address)),
          std::move(callback));
    }
    if (delegate_->GetOrigin() != siwe_message->origin) {
      const std::string& err_domain = siwe_message->origin.Serialize();
      brave_wallet_service_->AddSignMessageError(mojom::SignMessageError::New(
          GenerateRandomHexString(), MakeOriginInfo(delegate_->GetOrigin()),
          mojom::SignMessageErrorType::kDomainMismatched,
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_MISMATCH_ERR,
              l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_DOMAIN),
              base::ASCIIToUTF16(err_domain)),
          std::nullopt));
      delegate_->ShowPanel();
      return RejectMismatchError(
          std::move(id),
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_DOMAIN_MISMATCH,
              base::ASCIIToUTF16(err_domain)),
          std::move(callback));
    }

    sign_data = mojom::SignDataUnion::NewEthSiweData(std::move(siwe_message));
  } else {
    if (!base::IsStringUTF8(message_str)) {
      message_str = ToHex(message_str);
    }

    sign_data = mojom::SignDataUnion::NewEthStandardSignData(
        mojom::EthStandardSignData::New(message_str));
  }

  SignMessageInternal(account_id, std::move(sign_data),
                      std::move(message_bytes), std::move(callback),
                      std::move(id));
}

void EthereumProviderImpl::RecoverAddress(const std::string& message,
                                          const std::string& signature,
                                          RequestCallback callback,
                                          base::Value id) {
  bool reject = false;
  // TODO(apaymyshev): MM supports larger v. Not only one byte as we do.
  // https://github.com/ethereumjs/ethereumjs-util/blob/f51bfcab9e5505dfed4819ef1336f9fc00a12c3d/src/signature.ts#L110
  // 65 * 2 hex chars per byte + 2 chars for  0x
  if (signature.length() != 132) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  auto message_bytes = PrefixedHexStringToBytes(message);
  if (!message_bytes) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  auto signature_bytes = PrefixedHexStringToBytes(signature);
  if (!signature_bytes) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  auto address = keyring_service_->RecoverAddressByDefaultKeyring(
      *message_bytes, *signature_bytes);
  if (!address) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", false));
    return;
  }

  reject = false;
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), base::Value(std::move(address).value()), reject, "",
      false));
}

void EthereumProviderImpl::EthSubscribe(const std::string& event_type,
                                        std::optional<base::DictValue> filter,
                                        RequestCallback callback,
                                        base::Value id) {
  const std::string& chain_id = json_rpc_service_->GetChainIdSync(
      mojom::CoinType::ETH, delegate_->GetOrigin());
  if (event_type == kEthSubscribeNewHeads) {
    auto& new_subscription =
        eth_subscriptions_.emplace_back(ToHex(crypto::RandBytesAsVector(16)));
    if (eth_subscriptions_.size() == 1) {
      eth_block_tracker_.Start(
          chain_id, base::Seconds(kBlockTrackerDefaultTimeInSeconds));
    }
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), base::Value(new_subscription), false, "", false));
    return;
  }

  if (event_type == kEthSubscribeLogs && filter) {
    auto& new_subscription = eth_log_subscriptions_.emplace_back(
        ToHex(crypto::RandBytesAsVector(16)));
    if (eth_log_subscriptions_.size() == 1) {
      eth_logs_tracker_.Start(chain_id,
                              base::Seconds(kLogTrackerDefaultTimeInSeconds));
    }

    eth_logs_tracker_.AddSubscriber(new_subscription, std::move(*filter));

    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), base::Value(new_subscription), false, "", false));
    return;
  }

  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_UNSUPPORTED_SUBSCRIPTION_TYPE));
  bool reject = true;
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), reject, "", false));
}

void EthereumProviderImpl::EthUnsubscribe(const std::string& subscription_id,
                                          RequestCallback callback,
                                          base::Value id) {
  bool found = UnsubscribeBlockObserver(subscription_id) ||
               UnsubscribeLogObserver(subscription_id);

  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), base::Value(found), false, "", false));
}

bool EthereumProviderImpl::UnsubscribeBlockObserver(
    const std::string& subscription_id) {
  auto it = std::find(eth_subscriptions_.begin(), eth_subscriptions_.end(),
                      subscription_id);
  bool found = it != eth_subscriptions_.end();
  if (found) {
    if (eth_subscriptions_.size() == 1) {
      eth_block_tracker_.Stop(json_rpc_service_->GetChainIdSync(
          mojom::CoinType::ETH, delegate_->GetOrigin()));
    }
    eth_subscriptions_.erase(it);
  }
  return found;
}

bool EthereumProviderImpl::UnsubscribeLogObserver(
    const std::string& subscription_id) {
  if (std::erase(eth_log_subscriptions_, subscription_id)) {
    eth_logs_tracker_.RemoveSubscriber(subscription_id);
    if (eth_log_subscriptions_.empty()) {
      eth_logs_tracker_.Stop();
    }
    return true;
  }
  return false;
}

void EthereumProviderImpl::GetEncryptionPublicKey(const std::string& address,
                                                  RequestCallback callback,
                                                  base::Value id) {
  const auto account_id =
      FindAuthenticatedAccountByAddress(address, id, callback);
  if (!account_id) {
    return;
  }

  // Only show bubble when there is no immediate error
  brave_wallet_service_->AddGetPublicKeyRequest(
      account_id, delegate_->GetOrigin(), std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::Decrypt(
    const std::string& untrusted_encrypted_data_json,
    const std::string& address,
    const url::Origin& origin,
    RequestCallback callback,
    base::Value id) {
  const auto account_id =
      FindAuthenticatedAccountByAddress(address, id, callback);
  if (!account_id) {
    return;
  }

  api_request_helper::ParseJsonNonBlocking(
      untrusted_encrypted_data_json,
      base::BindOnce(&EthereumProviderImpl::ContinueDecryptWithSanitizedJson,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), account_id->Clone(), origin));
}

void EthereumProviderImpl::ContinueDecryptWithSanitizedJson(
    RequestCallback callback,
    base::Value id,
    const mojom::AccountIdPtr& account_id,
    const url::Origin& origin,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    SendErrorOnRequest(mojom::ProviderError::kInvalidParams,
                       l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                       std::move(callback), std::move(id));
    return;
  }

  auto eth_decrypt_data = ParseEthDecryptData(result->GetDict());
  if (!eth_decrypt_data) {
    SendErrorOnRequest(mojom::ProviderError::kInvalidParams,
                       l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                       std::move(callback), std::move(id));
    return;
  }

  std::optional<std::vector<uint8_t>> unsafe_message_bytes =
      keyring_service_
          ->DecryptCipherFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
              account_id, eth_decrypt_data->version, eth_decrypt_data->nonce,
              eth_decrypt_data->ephemeral_public_key,
              eth_decrypt_data->ciphertext);
  if (!unsafe_message_bytes.has_value()) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  std::string unsafe_message(unsafe_message_bytes->begin(),
                             unsafe_message_bytes->end());
  // If the string was not UTF8 then it should have already failed on the
  // JSON sanitization, but we add this check for extra safety.
  if (!base::IsStringUTF8(unsafe_message)) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  brave_wallet_service_->AddDecryptRequest(account_id, origin,
                                           std::move(unsafe_message),
                                           std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::SignTypedMessage(
    mojom::EthSignTypedDataPtr eth_sign_typed_data,
    RequestCallback callback,
    base::Value id) {
  if (eth_sign_typed_data->chain_id) {
    auto active_chain_id = json_rpc_service_->GetChainIdSync(
        mojom::CoinType::ETH, delegate_->GetOrigin());
    if (!base::EqualsCaseInsensitiveASCII(*eth_sign_typed_data->chain_id,
                                          active_chain_id)) {
      return RejectMismatchError(
          std::move(id),
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_MESSAGE_CHAIN_ID_MISMATCH,
              base::ASCIIToUTF16(*eth_sign_typed_data->chain_id)),
          std::move(callback));
    }
  }

  const auto account_id = FindAuthenticatedAccountByAddress(
      eth_sign_typed_data->address_param, id, callback);
  if (!account_id) {
    return;
  }

  auto message_to_sign = EthSignTypedDataHelper::GetTypedDataMessageToSign(
      eth_sign_typed_data->domain_hash, eth_sign_typed_data->primary_hash);

  mojom::SignDataUnionPtr sign_data =
      mojom::SignDataUnion::NewEthSignTypedData(std::move(eth_sign_typed_data));

  SignMessageInternal(account_id, std::move(sign_data),
                      base::ToVector(message_to_sign), std::move(callback),
                      std::move(id));
}

void EthereumProviderImpl::SignMessageInternal(
    const mojom::AccountIdPtr& account_id,
    mojom::SignDataUnionPtr sign_data,
    std::vector<uint8_t> message_to_sign,
    RequestCallback callback,
    base::Value id) {
  CHECK(sign_data);
  bool is_eip712 = sign_data->is_eth_sign_typed_data();
  auto request = mojom::SignMessageRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), 0, account_id.Clone(),
      std::move(sign_data), mojom::CoinType::ETH,
      json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH,
                                        delegate_->GetOrigin()));

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&EthereumProviderImpl::OnSignMessageRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), account_id.Clone(),
                     std::move(message_to_sign), is_eip712));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::OnSignMessageRequestProcessed(
    RequestCallback callback,
    base::Value id,
    const mojom::AccountIdPtr& account_id,
    std::vector<uint8_t> message,
    bool is_eip712,
    bool approved,
    mojom::EthereumSignatureBytesPtr hw_signature,
    const std::optional<std::string>& error) {
  bool reject = false;
  if (error && !error->empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError, *error);
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", false));
    return;
  }
  if (!approved) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    reject = true;
    std::move(callback).Run(mojom::EthereumProviderResponse::New(
        std::move(id), std::move(formed_response), reject, "", false));
    return;
  }

  base::Value formed_response;
  if (account_id->kind != mojom::AccountKind::kHardware) {
    auto signature = keyring_service_->SignMessageByDefaultKeyring(
        account_id, message, is_eip712);
    if (!signature.has_value()) {
      formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError, signature.error());
      reject = true;
    } else {
      formed_response = base::Value(ToHex(signature.value()));
    }
  } else {
    if (!hw_signature) {  // Missing hardware signature.
      formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      reject = true;
    } else {
      formed_response = base::Value(ToHex(hw_signature->bytes));
    }
  }

  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), reject, "", false));
}

bool EthereumProviderImpl::CheckAccountAllowed(
    const mojom::AccountIdPtr& account_id,
    const std::vector<std::string>& allowed_accounts) {
  for (const auto& allowed_account : allowed_accounts) {
    if (base::EqualsCaseInsensitiveASCII(account_id->address,
                                         allowed_account)) {
      return true;
    }
  }
  return false;
}

void EthereumProviderImpl::OnAddEthereumChainRequestCompleted(
    const std::string& chain_id,
    const std::string& error) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (!chain_callbacks_.contains(chain_id_lower) ||
      !chain_ids_.contains(chain_id_lower)) {
    return;
  }
  if (error.empty()) {
    // To match MM for webcompat, after adding a chain we should prompt
    // again to switch to the chain. And the error result only depends on
    // what the switch action is at that point.
    SwitchEthereumChain(chain_id_lower,
                        std::move(chain_callbacks_[chain_id_lower]),
                        std::move(chain_ids_[chain_id_lower]));
    chain_callbacks_.erase(chain_id_lower);
    chain_ids_.erase(chain_id_lower);
    return;
  }
  bool reject = true;
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUserRejectedRequest, error);
  std::move(chain_callbacks_[chain_id_lower])
      .Run(mojom::EthereumProviderResponse::New(
          std::move(chain_ids_[chain_id_lower]), std::move(formed_response),
          reject, "", false));

  chain_callbacks_.erase(chain_id_lower);
  chain_ids_.erase(chain_id_lower);
}

void EthereumProviderImpl::Request(base::Value input,
                                   RequestCallback callback) {
  CommonRequestOrSendAsync(std::move(input), std::move(callback), false);
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::SendAsync(base::Value input,
                                     SendAsyncCallback callback) {
  CommonRequestOrSendAsync(std::move(input), std::move(callback), true);
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::SendErrorOnRequest(const mojom::ProviderError& error,
                                              const std::string& error_message,
                                              RequestCallback callback,
                                              base::Value id) {
  base::Value formed_response =
      GetProviderErrorDictionary(error, error_message);
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), true, "", false));
}

void EthereumProviderImpl::SetupMethodHandlers() {
  auto add_handler = [this](std::string_view method, auto ptr) {
    method_handlers_.emplace(method,
                             base::BindRepeating(ptr, base::Unretained(this)));
  };

  add_handler(kEthAccounts, &EthereumProviderImpl::HandleEthAccountsMethod);
  add_handler(kEthCoinbase, &EthereumProviderImpl::HandleEthCoinbaseMethod);
  add_handler(kEthRequestAccounts,
              &EthereumProviderImpl::HandleEthRequestAccountsMethod);
  add_handler(kAddEthereumChainMethod,
              &EthereumProviderImpl::HandleAddEthereumChainMethodMethod);
  add_handler(kSwitchEthereumChainMethod,
              &EthereumProviderImpl::HandleSwitchEthereumChainMethodMethod);
  add_handler(kEthSendTransaction,
              &EthereumProviderImpl::HandleEthSendTransactionMethod);
  add_handler(kEthSignTransaction,
              &EthereumProviderImpl::HandleEthSignTransactionMethod);
  add_handler(kEthSendRawTransaction,
              &EthereumProviderImpl::HandleEthSendRawTransactionMethod);
  add_handler(kEthSign, &EthereumProviderImpl::HandleEthSignMethod);
  add_handler(kPersonalSign, &EthereumProviderImpl::HandlePersonalSignMethod);
  add_handler(kPersonalEcRecover,
              &EthereumProviderImpl::HandlePersonalEcRecoverMethod);
  add_handler(kEthSignTypedDataV3,
              &EthereumProviderImpl::HandleEthSignTypedDataV3Method);
  add_handler(kEthSignTypedDataV4,
              &EthereumProviderImpl::HandleEthSignTypedDataV4Method);
  add_handler(kEthGetEncryptionPublicKey,
              &EthereumProviderImpl::HandleEthGetEncryptionPublicKeyMethod);
  add_handler(kEthDecrypt, &EthereumProviderImpl::HandleEthDecryptMethod);
  add_handler(kWalletWatchAsset,
              &EthereumProviderImpl::HandleWalletWatchAssetMethod);
  add_handler(kMetamaskWatchAsset,
              &EthereumProviderImpl::HandleMetamaskWatchAssetMethod);
  add_handler(kRequestPermissionsMethod,
              &EthereumProviderImpl::HandleRequestPermissionsMethod);
  add_handler(kGetPermissionsMethod,
              &EthereumProviderImpl::HandleGetPermissionsMethod);
  add_handler(kWeb3ClientVersion,
              &EthereumProviderImpl::HandleWeb3ClientVersionMethod);
  add_handler(kEthSubscribe, &EthereumProviderImpl::HandleEthSubscribeMethod);
  add_handler(kEthUnsubscribe,
              &EthereumProviderImpl::HandleEthUnsubscribeMethod);
}

void EthereumProviderImpl::HandleEthAccountsMethod(JsonRpcRequest request,
                                                   RequestCallback callback) {
  GetAllowedAccountsInternal(std::move(callback), std::move(request.id),
                             kEthAccounts, false);
}

void EthereumProviderImpl::HandleEthCoinbaseMethod(JsonRpcRequest request,
                                                   RequestCallback callback) {
  GetAllowedAccountsInternal(std::move(callback), std::move(request.id),
                             kEthCoinbase, false);
}

void EthereumProviderImpl::HandleEthRequestAccountsMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  RequestEthereumPermissions(std::move(callback), std::move(request.id),
                             kEthRequestAccounts, delegate_->GetOrigin());
}

void EthereumProviderImpl::HandleAddEthereumChainMethodMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  AddEthereumChain(std::move(request.params), std::move(callback),
                   std::move(request.id));
}

void EthereumProviderImpl::HandleSwitchEthereumChainMethodMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto chain_id = ParseSwitchEthereumChainParams(request.params);
  if (!chain_id) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  SwitchEthereumChain(*chain_id, std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleEthSendTransactionMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  SendOrSignTransactionInternal(std::move(callback), std::move(request.id),
                                request.params, false);
}

void EthereumProviderImpl::HandleEthSignTransactionMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  SendOrSignTransactionInternal(std::move(callback), std::move(request.id),
                                request.params, true);
}

void EthereumProviderImpl::HandleEthSendRawTransactionMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto signed_transaction = ParseEthSendRawTransactionParams(request.params);
  if (!signed_transaction) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  json_rpc_service_->SendRawTransaction(
      json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH,
                                        delegate_->GetOrigin()),
      *signed_transaction,
      base::BindOnce(&EthereumProviderImpl::OnSendRawTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(request.id)));
}

void EthereumProviderImpl::HandleEthSignMethod(JsonRpcRequest request,
                                               RequestCallback callback) {
  auto sign_params = ParseEthSignParams(request.params);
  if (!sign_params) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  // Typed data should only be signed by eth_signTypedData
  if (IsTypedDataStructure(request.params)) {
    return RejectInvalidParams(std::move(request.id), std::move(callback));
  }
  SignMessage(sign_params->address, sign_params->message, std::move(callback),
              std::move(request.id));
}

void EthereumProviderImpl::HandlePersonalSignMethod(JsonRpcRequest request,
                                                    RequestCallback callback) {
  auto sign_params = ParsePersonalSignParams(request.params);
  if (!sign_params) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  // Typed data should only be signed by eth_signTypedData
  if (IsTypedDataStructure(request.params)) {
    return RejectInvalidParams(std::move(request.id), std::move(callback));
  }
  SignMessage(sign_params->address, sign_params->message, std::move(callback),
              std::move(request.id));
}

void EthereumProviderImpl::HandlePersonalEcRecoverMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto recover_params = ParsePersonalEcRecoverParams(request.params);
  if (!recover_params) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  RecoverAddress(recover_params->message, recover_params->signature,
                 std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleEthSignTypedDataV3Method(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto eth_sign_typed_data = ParseEthSignTypedDataParams(
      request.params, EthSignTypedDataHelper::Version::kV3);

  if (!eth_sign_typed_data) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }

  SignTypedMessage(std::move(eth_sign_typed_data), std::move(callback),
                   std::move(request.id));
}

void EthereumProviderImpl::HandleEthSignTypedDataV4Method(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto eth_sign_typed_data = ParseEthSignTypedDataParams(
      request.params, EthSignTypedDataHelper::Version::kV4);

  if (!eth_sign_typed_data) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }

  SignTypedMessage(std::move(eth_sign_typed_data), std::move(callback),
                   std::move(request.id));
}

void EthereumProviderImpl::HandleEthGetEncryptionPublicKeyMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto address = ParseEthGetEncryptionPublicKeyParams(request.params);
  if (!address) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  GetEncryptionPublicKey(*address, std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleEthDecryptMethod(JsonRpcRequest request,
                                                  RequestCallback callback) {
  auto eth_decrypt_params = ParseEthDecryptParams(request.params);
  if (!eth_decrypt_params) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  Decrypt(eth_decrypt_params->untrusted_encrypted_data_json,
          eth_decrypt_params->address, delegate_->GetOrigin(),
          std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleWalletWatchAssetMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  mojom::ProviderError error = mojom::ProviderError::kUnsupportedMethod;
  std::string error_message;
  mojom::BlockchainTokenPtr token =
      ParseWalletWatchAssetParams(request.params, error_message);
  if (!token) {
    if (!error_message.empty()) {
      error = mojom::ProviderError::kInvalidParams;
    }
    SendErrorOnRequest(error, error_message, std::move(callback),
                       std::move(request.id));
    return;
  }
  token->chain_id = json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH,
                                                      delegate_->GetOrigin());
  AddSuggestToken(std::move(token), std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleMetamaskWatchAssetMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  mojom::ProviderError error = mojom::ProviderError::kUnsupportedMethod;
  std::string error_message;
  mojom::BlockchainTokenPtr token =
      ParseWalletWatchAssetParams(request.params, error_message);
  if (!token) {
    if (!error_message.empty()) {
      error = mojom::ProviderError::kInvalidParams;
    }
    SendErrorOnRequest(error, error_message, std::move(callback),
                       std::move(request.id));
    return;
  }
  token->chain_id = json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH,
                                                      delegate_->GetOrigin());
  AddSuggestToken(std::move(token), std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleRequestPermissionsMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto restricted_methods = ParseRequestPermissionsParams(request.params);
  if (!restricted_methods) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  if (!restricted_methods->contains("eth_accounts")) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }

  RequestEthereumPermissions(std::move(callback), std::move(request.id),
                             kRequestPermissionsMethod, delegate_->GetOrigin());
}

void EthereumProviderImpl::HandleGetPermissionsMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  GetAllowedAccountsInternal(std::move(callback), std::move(request.id),
                             kGetPermissionsMethod, true);
}

void EthereumProviderImpl::HandleWeb3ClientVersionMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  Web3ClientVersion(std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::HandleEthSubscribeMethod(JsonRpcRequest request,
                                                    RequestCallback callback) {
  auto eth_subscribe_params = ParseEthSubscribeParams(request.params);
  if (!eth_subscribe_params) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  EthSubscribe(std::move(eth_subscribe_params->event_type),
               std::move(eth_subscribe_params->filter), std::move(callback),
               std::move(request.id));
}

void EthereumProviderImpl::HandleEthUnsubscribeMethod(
    JsonRpcRequest request,
    RequestCallback callback) {
  auto subscription_id = ParseEthUnsubscribeParams(request.params);
  if (!subscription_id) {
    RejectUnsupportedMethod(std::move(request.id), std::move(callback));
    return;
  }
  EthUnsubscribe(*subscription_id, std::move(callback), std::move(request.id));
}

void EthereumProviderImpl::CommonRequestOrSendAsync(
    base::Value input_value,
    RequestCallback request_callback,
    bool format_json_rpc_response) {
  auto callback = base::BindOnce(
      &EthereumProviderImpl::OnResponse, weak_factory_.GetWeakPtr(),
      format_json_rpc_response, std::move(request_callback));

  auto json_rpc_request = ParseJsonRpcRequest(std::move(input_value));
  if (!json_rpc_request) {
    RejectUnsupportedMethod(base::Value(), std::move(callback));
    return;
  }

  // That check prevents from pop ups from background pages.
  // We need to add any method that requires a dialog to interact with.
  if (!delegate_->IsTabVisible() &&
      kMethodsRequireInteraction.contains(json_rpc_request->method)) {
    SendErrorOnRequest(
        mojom::ProviderError::kResourceUnavailable,
        l10n_util::GetStringUTF8(IDS_WALLET_TAB_IS_NOT_ACTIVE_ERROR),
        std::move(callback), base::Value());
    return;
  }

  auto it = method_handlers_.find(json_rpc_request->method);
  if (it != method_handlers_.end()) {
    it->second.Run(std::move(*json_rpc_request), std::move(callback));
    return;
  }

  json_rpc_service_->Request(json_rpc_service_->GetChainIdSync(
                                 mojom::CoinType::ETH, delegate_->GetOrigin()),
                             std::move(*json_rpc_request), std::move(callback));
}

void EthereumProviderImpl::Send(const std::string& method,
                                base::ListValue params,
                                SendCallback callback) {
  CommonRequestOrSendAsync(
      base::Value(GetJsonRpcRequest(method, std::move(params))),
      std::move(callback), true);
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::RequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin) {
  DCHECK(delegate_);
  if (delegate_->IsPermissionDenied(mojom::CoinType::ETH)) {
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kNone,
                                 std::vector<std::string>());
    return;
  }

  std::vector<std::string> addresses;
  for (auto& account_info : keyring_service_->GetAllAccountInfos()) {
    if (account_info->account_id->coin == mojom::CoinType::ETH) {
      addresses.push_back(
          GetAccountPermissionIdentifier(account_info->account_id));
    }
  }

  if (addresses.empty()) {
    if (!wallet_onboarding_shown_) {
      delegate_->ShowWalletOnboarding();
      wallet_onboarding_shown_ = true;
    }
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kInternal,
                                 std::nullopt);
    return;
  }

  if (keyring_service_->IsLockedSync()) {
    if (pending_request_ethereum_permissions_callback_) {
      OnRequestEthereumPermissions(
          std::move(callback), std::move(id), method, origin,
          RequestPermissionsError::kRequestInProgress, std::nullopt);
      return;
    }
    pending_request_ethereum_permissions_callback_ = std::move(callback);
    pending_request_ethereum_permissions_id_ = std::move(id);
    pending_request_ethereum_permissions_method_ = method;
    pending_request_ethereum_permissions_origin_ = origin;
    keyring_service_->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  const auto allowed_accounts =
      delegate_->GetAllowedAccounts(mojom::CoinType::ETH, addresses);
  const bool success = allowed_accounts.has_value();

  if (!success) {
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kInternal,
                                 std::nullopt);
    return;
  }

  if (success && !allowed_accounts->empty()) {
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kNone,
                                 allowed_accounts);
  } else {
    // Request accounts if no accounts are connected.
    delegate_->RequestPermissions(
        mojom::CoinType::ETH, addresses,
        base::BindOnce(&EthereumProviderImpl::OnRequestEthereumPermissions,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), method, origin));
  }
}

void EthereumProviderImpl::Enable(EnableCallback callback) {
  if (!delegate_->IsTabVisible()) {
    SendErrorOnRequest(
        mojom::ProviderError::kResourceUnavailable,
        l10n_util::GetStringUTF8(IDS_WALLET_TAB_IS_NOT_ACTIVE_ERROR),
        std::move(callback), base::Value());
    return;
  }
  RequestEthereumPermissions(std::move(callback), base::Value(), "",
                             delegate_->GetOrigin());
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::OnRequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin,
    RequestPermissionsError error,
    const std::optional<std::vector<std::string>>&
        allowed_accounts_identifiers) {
  base::Value formed_response;

  bool success = error == RequestPermissionsError::kNone;
  std::vector<std::string> accounts;
  if (success && allowed_accounts_identifiers) {
    accounts = FilterAccountsByPermissionIdentifier(
        *allowed_accounts_identifiers,
        keyring_service_->GetSelectedEthereumDappAccount());
  }

  std::string first_allowed_account;
  if (accounts.size() > 0) {
    first_allowed_account = base::ToLowerASCII(accounts[0]);
  }
  if (success && accounts.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  } else if (!success) {
    switch (error) {
      case RequestPermissionsError::kRequestInProgress:
        formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kUserRejectedRequest,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
        break;
      case RequestPermissionsError::kInternal:
        formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
        break;
      default:
        NOTREACHED() << error;
    }
  } else if (method == kRequestPermissionsMethod) {
    formed_response =
        base::Value(PermissionRequestResponseToValue(origin, accounts));
  } else {
    base::ListValue list;
    for (const auto& account : accounts) {
      list.Append(base::ToLowerASCII(account));
    }
    formed_response = base::Value(std::move(list));
  }
  bool reject = !success || accounts.empty();

  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), reject, first_allowed_account,
      true));
}

std::optional<std::vector<std::string>>
EthereumProviderImpl::GetAllowedAccounts(bool include_accounts_when_locked) {
  std::vector<std::string> addresses;
  for (const auto& account_info : keyring_service_->GetAllAccountInfos()) {
    if (account_info->account_id->coin == mojom::CoinType::ETH) {
      addresses.push_back(base::ToLowerASCII(
          GetAccountPermissionIdentifier(account_info->account_id)));
    }
  }

  const auto selected_account =
      keyring_service_->GetSelectedEthereumDappAccount();

  DCHECK(delegate_);
  const auto allowed_accounts =
      delegate_->GetAllowedAccounts(mojom::CoinType::ETH, addresses);

  if (!allowed_accounts) {
    return std::nullopt;
  }

  std::vector<std::string> filtered_accounts;
  if (!keyring_service_->IsLockedSync() || include_accounts_when_locked) {
    filtered_accounts = FilterAccountsByPermissionIdentifier(*allowed_accounts,
                                                             selected_account);
  }

  return filtered_accounts;
}

void EthereumProviderImpl::GetAllowedAccountsInternal(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    bool include_accounts_when_locked) {
  const auto accounts_opt = GetAllowedAccounts(include_accounts_when_locked);
  if (!accounts_opt) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }
  const auto& accounts = accounts_opt.value();

  bool reject = false;
  bool update_bindings = false;
  base::Value formed_response;

  if (method == kEthAccounts) {
    base::ListValue list;
    for (const auto& account : accounts) {
      list.Append(base::ToLowerASCII(account));
    }
    formed_response = base::Value(std::move(list));
    update_bindings = false;
  } else if (method == kEthCoinbase) {
    if (accounts.empty()) {
      formed_response = base::Value();
    } else {
      formed_response = base::Value(base::ToLowerASCII(accounts[0]));
    }
    update_bindings = false;
  } else {
    formed_response = base::Value(
        PermissionRequestResponseToValue(delegate_->GetOrigin(), accounts));
    update_bindings = true;
  }
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response), reject, "", update_bindings));
}

void EthereumProviderImpl::UpdateKnownAccounts() {
  const auto allowed_accounts = GetAllowedAccounts(false);
  if (!allowed_accounts) {
    return;
  }
  bool accounts_changed = allowed_accounts != known_allowed_accounts_;
  known_allowed_accounts_ = *allowed_accounts;
  if (!first_known_accounts_check_ && events_listener_.is_bound() &&
      accounts_changed) {
    events_listener_->AccountsChangedEvent(known_allowed_accounts_);
  }
  first_known_accounts_check_ = false;
}

void EthereumProviderImpl::Web3ClientVersion(RequestCallback callback,
                                             base::Value id) {
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), base::Value(GetWeb3ClientVersion()), false, "", false));
}

void EthereumProviderImpl::GetChainId(GetChainIdCallback callback) {
  if (json_rpc_service_) {
    json_rpc_service_->GetChainIdForOrigin(
        mojom::CoinType::ETH, delegate_->GetOrigin(), std::move(callback));
  }
}

mojom::AccountIdPtr EthereumProviderImpl::FindAuthenticatedAccountByAddress(
    const std::string& address,
    base::Value& id,
    mojom::EthereumProvider::RequestCallback& callback) {
  if (!EthAddress::IsValidAddress(address)) {
    RejectInvalidParams(std::move(id), std::move(callback));
    return nullptr;
  }
  auto account_id = FindAccountByAddress(address);
  if (!account_id) {
    RejectAccountNotAuthed(std::move(id), std::move(callback));
    return nullptr;
  }
  const auto allowed_accounts = GetAllowedAccounts(false);
  if (!allowed_accounts) {
    RejectInvalidParams(std::move(id), std::move(callback));
    return nullptr;
  }
  if (!CheckAccountAllowed(account_id, *allowed_accounts)) {
    RejectAccountNotAuthed(std::move(id), std::move(callback));
    return nullptr;
  }
  return account_id;
}

mojom::AccountIdPtr EthereumProviderImpl::FindAccountByAddress(
    const std::string& address) {
  AccountResolverDelegateImpl resolver(*keyring_service_);

  auto account_id = resolver.ResolveAccountId(nullptr, &address);
  if (!account_id || account_id->coin != mojom::CoinType::ETH) {
    return nullptr;
  }

  return account_id;
}

void EthereumProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void EthereumProviderImpl::ChainChangedEvent(
    const std::string& chain_id,
    mojom::CoinType coin,
    const std::optional<url::Origin>& origin) {
  if (!events_listener_.is_bound() || coin != mojom::CoinType::ETH) {
    return;
  }

  if (origin.has_value() && *origin != delegate_->GetOrigin()) {
    return;
  }

  eth_logs_tracker_.Stop();
  events_listener_->ChainChangedEvent(chain_id);
}

void EthereumProviderImpl::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  auto tx_status = tx_info->tx_status;
  if (tx_status != mojom::TransactionStatus::Submitted &&
      tx_status != mojom::TransactionStatus::Signed &&
      tx_status != mojom::TransactionStatus::Rejected &&
      tx_status != mojom::TransactionStatus::Error) {
    return;
  }

  std::string tx_meta_id = tx_info->id;
  if (!add_tx_callbacks_.contains(tx_meta_id) ||
      !add_tx_ids_.contains(tx_meta_id)) {
    return;
  }

  std::string tx_hash = tx_info->tx_hash;
  base::Value formed_response;
  bool reject = true;
  if (tx_status == mojom::TransactionStatus::Submitted) {
    formed_response = base::Value(tx_hash);
    reject = false;
  } else if (tx_status == mojom::TransactionStatus::Signed) {
    std::string signed_transaction;
    if (tx_info->tx_data_union->is_eth_tx_data()) {
      DCHECK(tx_info->tx_data_union->get_eth_tx_data()->signed_transaction);
      signed_transaction =
          *tx_info->tx_data_union->get_eth_tx_data()->signed_transaction;
    } else if (tx_info->tx_data_union->is_eth_tx_data_1559()) {
      DCHECK(tx_info->tx_data_union->get_eth_tx_data_1559()
                 ->base_data->signed_transaction);
      signed_transaction = *tx_info->tx_data_union->get_eth_tx_data_1559()
                                ->base_data->signed_transaction;
    }
    formed_response = base::Value(signed_transaction);
    reject = false;
  } else if (tx_status == mojom::TransactionStatus::Rejected) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_USER_REJECTED));
    reject = true;
  } else if (tx_status == mojom::TransactionStatus::Error) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_ERROR));
    reject = true;
  }
  std::move(add_tx_callbacks_[tx_meta_id])
      .Run(mojom::EthereumProviderResponse::New(
          std::move(add_tx_ids_[tx_meta_id]), std::move(formed_response),
          reject, "", false));
  add_tx_callbacks_.erase(tx_meta_id);
  add_tx_ids_.erase(tx_meta_id);
}

void EthereumProviderImpl::SelectedDappAccountChanged(
    mojom::CoinType coin,
    mojom::AccountInfoPtr account) {
  if (coin != mojom::CoinType::ETH) {
    return;
  }
  UpdateKnownAccounts();
}

void EthereumProviderImpl::Locked() {
  UpdateKnownAccounts();
}

void EthereumProviderImpl::Unlocked() {
  if (pending_request_ethereum_permissions_callback_) {
    RequestEthereumPermissions(
        std::move(pending_request_ethereum_permissions_callback_),
        std::move(pending_request_ethereum_permissions_id_),
        pending_request_ethereum_permissions_method_,
        pending_request_ethereum_permissions_origin_);
  } else {
    UpdateKnownAccounts();
  }
}

void EthereumProviderImpl::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type == ContentSettingsType::BRAVE_ETHEREUM) {
    UpdateKnownAccounts();
  }
}

void EthereumProviderImpl::AddSuggestToken(mojom::BlockchainTokenPtr token,
                                           RequestCallback callback,
                                           base::Value id) {
  if (!token) {
    return RejectInvalidParams(std::move(id), std::move(callback));
  }

  auto request = mojom::AddSuggestTokenRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), std::move(token));
  brave_wallet_service_->AddSuggestTokenRequest(
      std::move(request), std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::OnSendRawTransaction(
    RequestCallback callback,
    base::Value id,
    const std::string& tx_hash,
    mojom::ProviderError error,
    const std::string& error_message) {
  base::Value formed_response;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else {
    formed_response = base::Value(tx_hash);
  }
  std::move(callback).Run(mojom::EthereumProviderResponse::New(
      std::move(id), std::move(formed_response),
      error != mojom::ProviderError::kSuccess, "", false));
}

// EthBlockTracker::Observer:
void EthereumProviderImpl::OnLatestBlock(const std::string& chain_id,
                                         uint256_t block_num) {
  json_rpc_service_->GetBlockByNumber(
      chain_id, kEthereumBlockTagLatest,
      base::BindOnce(&EthereumProviderImpl::OnGetBlockByNumber,
                     weak_factory_.GetWeakPtr()));
}

void EthereumProviderImpl::OnGetBlockByNumber(
    base::Value result,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (events_listener_.is_bound() && error == mojom::ProviderError::kSuccess) {
    std::ranges::for_each(eth_subscriptions_,
                          [this, &result](const std::string& subscription_id) {
                            events_listener_->MessageEvent(subscription_id,
                                                           result.Clone());
                          });
  }
}

void EthereumProviderImpl::OnNewBlock(const std::string& chain_id,
                                      uint256_t block_num) {}

void EthereumProviderImpl::OnLogsReceived(const std::string& subscription,
                                          base::Value rawlogs) {
  if (!rawlogs.is_dict() || !events_listener_.is_bound()) {
    return;
  }

  auto& dict = rawlogs.GetDict();
  const base::ListValue* results = dict.FindList("result");

  if (results == nullptr) {
    return;
  }

  for (auto& results_item : *results) {
    events_listener_->MessageEvent(subscription, results_item.Clone());
  }
}

void EthereumProviderImpl::OnResponse(
    bool format_json_rpc_response,
    RequestCallback callback,
    mojom::EthereumProviderResponsePtr response) {
  if (format_json_rpc_response) {
    auto json_rpc_formed_response = ToProviderResponse(
        response->id.Clone(),
        response->reject ? nullptr : &response->formed_response,
        response->reject ? &response->formed_response : nullptr);
    response->formed_response = std::move(json_rpc_formed_response);
  }

  std::move(callback).Run(std::move(response));
}

}  // namespace brave_wallet
