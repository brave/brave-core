/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <map>
#include <set>
#include <utility>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class GetTransparentBalanceContext
    : public base::RefCountedThreadSafe<GetTransparentBalanceContext> {
 public:
  using GetBalanceCallback = mojom::ZCashWalletService::GetBalanceCallback;

  std::set<std::string> addresses;
  std::map<std::string, uint64_t> balances;
  absl::optional<std::string> error;
  GetBalanceCallback callback;

  bool ShouldRespond() { return callback && (error || addresses.empty()); }

  void SetError(const std::string& error_string) { error = error_string; }

 protected:
  friend class base::RefCountedThreadSafe<GetTransparentBalanceContext>;
  virtual ~GetTransparentBalanceContext() = default;
};

ZCashWalletService::ZCashWalletService(
    KeyringService* keyring_service,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      zcash_rpc_(
          std::make_unique<zcash_rpc::ZCashRpc>(prefs, url_loader_factory)) {
  zcash_rpc_ = std::make_unique<zcash_rpc::ZCashRpc>(prefs, url_loader_factory);
}

mojo::PendingRemote<mojom::ZCashWalletService>
ZCashWalletService::MakeRemote() {
  mojo::PendingRemote<mojom::ZCashWalletService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void ZCashWalletService::Bind(
    mojo::PendingReceiver<mojom::ZCashWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

ZCashWalletService::~ZCashWalletService() = default;

void ZCashWalletService::GetBalance(const std::string& chain_id,
                                    mojom::AccountIdPtr account_id,
                                    GetBalanceCallback callback) {
  if (!IsZCashNetwork(chain_id)) {
    // Desktop frontend sometimes does that.
    std::move(callback).Run(nullptr, "Invalid bitcoin chain id " + chain_id);
    return;
  }

  const auto& addresses = keyring_service_->GetZCashAddresses(*account_id);
  if (!addresses) {
    std::move(callback).Run(nullptr, "Couldn't get balance");
    return;
  }

  auto context = base::MakeRefCounted<GetTransparentBalanceContext>();
  context->callback = std::move(callback);
  for (const auto& address : addresses.value()) {
    context->addresses.insert(address.first);
  }

  for (const auto& address : context->addresses) {
    zcash_rpc_->GetUtxoList(
        chain_id, {address},
        base::BindOnce(&ZCashWalletService::OnGetUtxosForBalance,
                       weak_ptr_factory_.GetWeakPtr(), context, address));
  }
}

void ZCashWalletService::OnGetUtxosForBalance(
    scoped_refptr<GetTransparentBalanceContext> context,
    const std::string& address,
    base::expected<std::vector<zcash::ZCashUtxo>, std::string> result) {
  DCHECK(context->addresses.contains(address));
  DCHECK(!context->balances.contains(address));

  if (!result.has_value()) {
    context->SetError(result.error());
    WorkOnGetBalance(std::move(context));
    return;
  }

  uint64_t total_amount = 0;
  for (const auto& output : result.value()) {
    total_amount += output.valuezat();
  }

  context->addresses.erase(address);
  context->balances[address] = total_amount;

  WorkOnGetBalance(std::move(context));
}

void ZCashWalletService::WorkOnGetBalance(
    scoped_refptr<GetTransparentBalanceContext> context) {
  if (!context->ShouldRespond()) {
    return;
  }

  if (context->error) {
    std::move(context->callback).Run(nullptr, std::move(*context->error));
    return;
  }

  auto result = mojom::ZCashBalance::New();
  for (auto& balance : context->balances) {
    result->total_balance += balance.second;
  }

  result->balances.insert(context->balances.begin(), context->balances.end());
  std::move(context->callback).Run(std::move(result), absl::nullopt);
}

}  // namespace brave_wallet
