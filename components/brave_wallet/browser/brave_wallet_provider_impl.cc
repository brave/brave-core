/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : delegate_(std::move(delegate)), prefs_(prefs), weak_factory_(this) {
  DCHECK(rpc_controller);
  rpc_controller_.Bind(std::move(rpc_controller));
  DCHECK(rpc_controller_);
  rpc_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
}

BraveWalletProviderImpl::~BraveWalletProviderImpl() {}

bool BraveWalletProviderImpl::OnAddEthereumChainRequest(
    const std::string& json_payload,
    RequestCallback callback) {
  if (!delegate_)
    return false;
  base::Value payload;
  if (!ParsePayload(json_payload, "params", &payload))
    return false;
  std::vector<EthereumChain> chains = ValueToEthereumChain(payload);
  if (chains.empty()) {
    return false;
  }
  DCHECK_LT(chains.size(), size_t(2));
  // TODO(spylogsster): Add support for multiple chains;
  auto chain = chains[0];
  if (GetNetworkURL(prefs_, chain.chain_id).is_valid()) {
    // OnChainAddedResult(std::move(callback), std::string());
    // return true;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  if (chain.chain_id.empty() || chain.rpc_urls.empty() ||
      chain.chain_name.empty()) {
    // return false;
  }
  auto valueValue = brave_wallet::EthereumChainToValue(chain);
  std::string chainJson;
  if (!base::JSONWriter::Write(valueValue, &chainJson))
    return false;
  delegate_->RequestUserApproval(
      chainJson,
      base::BindOnce(&BraveWalletProviderImpl::OnChainAddedResult,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
  return true;
}

void BraveWalletProviderImpl::Request(const std::string& json_payload,
                                      bool auto_retry_on_network_change,
                                      RequestCallback callback) {
  std::string method = brave_wallet::ParseRequestMethodName(json_payload);
  if (method == kAddEthereumChainMethod &&
      OnAddEthereumChainRequest(json_payload, std::move(callback))) {
    return;
  }
  if (rpc_controller_) {
    rpc_controller_->Request(json_payload, true, std::move(callback));
  }
}

void BraveWalletProviderImpl::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  DCHECK(delegate_);
  delegate_->RequestEthereumPermissions(
      base::BindOnce(&BraveWalletProviderImpl::OnRequestEthereumPermissions,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnRequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback,
    bool success,
    const std::vector<std::string>& accounts) {
  std::move(callback).Run(success, accounts);
}

void BraveWalletProviderImpl::GetAllowedAccounts(
    GetAllowedAccountsCallback callback) {
  DCHECK(delegate_);
  delegate_->GetAllowedAccounts(
      base::BindOnce(&BraveWalletProviderImpl::OnGetAllowedAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnGetAllowedAccounts(
    GetAllowedAccountsCallback callback,
    bool success,
    const std::vector<std::string>& accounts) {
  std::move(callback).Run(success, accounts);
}

void BraveWalletProviderImpl::GetChainId(GetChainIdCallback callback) {
  if (rpc_controller_) {
    rpc_controller_->GetChainId(std::move(callback));
  }
}

void BraveWalletProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
    if (rpc_controller_) {
      rpc_controller_->AddObserver(
          observer_receiver_.BindNewPipeAndPassRemote());
    }
  }
}

void BraveWalletProviderImpl::ChainChangedEvent(const std::string& chain_id) {
  if (!events_listener_.is_bound())
    return;

  events_listener_->ChainChangedEvent(chain_id);
}

void BraveWalletProviderImpl::OnConnectionError() {
  rpc_controller_.reset();
  observer_receiver_.reset();
}

void BraveWalletProviderImpl::OnChainAddedResult(RequestCallback callback,
                                                 const std::string& error) {
  base::flat_map<std::string, std::string> headers;
  std::string value = R"("result" : null)";
  if (!error.empty()) {
    value = error;
  }
  std::string response = base::StringPrintf(kJsonResponseF, value.c_str());
  std::move(callback).Run(200, response, headers);
}

}  // namespace brave_wallet
