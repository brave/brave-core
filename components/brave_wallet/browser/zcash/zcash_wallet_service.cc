/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <optional>
#include <set>
#include <utility>

#include "base/barrier_callback.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service_tasks.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

void ZCashWalletService::Bind(
    mojo::PendingReceiver<mojom::ZCashWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

ZCashWalletService::ZCashWalletService(
    KeyringService* keyring_service,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service) {
  zcash_rpc_ = std::make_unique<ZCashRpc>(prefs, url_loader_factory);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

ZCashWalletService::ZCashWalletService(KeyringService* keyring_service,
                                       std::unique_ptr<ZCashRpc> zcash_rpc)
    : keyring_service_(keyring_service) {
  zcash_rpc_ = std::move(zcash_rpc);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

ZCashWalletService::~ZCashWalletService() = default;

void ZCashWalletService::GetBalance(const std::string& chain_id,
                                    mojom::AccountIdPtr account_id,
                                    GetBalanceCallback callback) {
  RunDiscovery(
      account_id.Clone(),
      base::BindOnce(&ZCashWalletService::OnDiscoveryDoneForBalance,
                     weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
                     chain_id, std::move(callback)));
}

void ZCashWalletService::GetReceiverAddress(
    mojom::AccountIdPtr account_id,
    GetReceiverAddressCallback callback) {
  auto id = mojom::ZCashKeyId::New(account_id->account_index, 0, 0);
  auto addr = keyring_service_->GetZCashAddress(*account_id, *id);
  if (!addr) {
    std::move(callback).Run(
        nullptr, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  }
  auto str_addr = addr->address_string;
  std::move(callback).Run(mojom::ZCashAddress::New(str_addr, std::move(id)),
                          std::nullopt);
}

void ZCashWalletService::GetZCashAccountInfo(
    mojom::AccountIdPtr account_id,
    GetZCashAccountInfoCallback callback) {
  std::move(callback).Run(keyring_service_->GetZCashAccountInfo(account_id));
}

void ZCashWalletService::RunDiscovery(mojom::AccountIdPtr account_id,
                                      RunDiscoveryCallback callback) {
  auto barrier_callback = base::BarrierCallback<
      base::expected<mojom::ZCashAddressPtr, std::string>>(
      2, base::BindOnce(&ZCashWalletService::OnRunDiscoveryDone,
                        weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
                        std::move(callback)));

  DiscoverNextUnusedAddress(
      account_id, false,
      base::BindOnce(&ZCashWalletService::AccumulateRunDiscovery,
                     weak_ptr_factory_.GetWeakPtr(), barrier_callback));
  DiscoverNextUnusedAddress(
      account_id, true,
      base::BindOnce(&ZCashWalletService::AccumulateRunDiscovery,
                     weak_ptr_factory_.GetWeakPtr(), barrier_callback));
}

void ZCashWalletService::AccumulateRunDiscovery(
    base::OnceCallback<void(
        base::expected<mojom::ZCashAddressPtr, std::string>)> barrier_callback,
    base::expected<mojom::ZCashAddressPtr, std::string> data) {
  std::move(barrier_callback).Run(std::move(data));
}

void ZCashWalletService::OnRunDiscoveryDone(
    mojom::AccountIdPtr account_id,
    RunDiscoveryCallback callback,
    std::vector<base::expected<mojom::ZCashAddressPtr, std::string>>
        discovered_address) {
  std::vector<mojom::ZCashAddressPtr> result;
  for (const auto& item : discovered_address) {
    if (item.has_value()) {
      UpdateNextUnusedAddressForAccount(account_id, *item);
      result.push_back(item->Clone());
    } else {
      std::move(callback).Run(base::unexpected(
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
      return;
    }
  }

  std::move(callback).Run(std::move(result));
}

void ZCashWalletService::UpdateNextUnusedAddressForAccount(
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashAddressPtr& address) {
  std::optional<uint32_t> next_receive_index = address->key_id->change
                                                   ? std::optional<uint32_t>()
                                                   : address->key_id->index;
  std::optional<uint32_t> next_change_index = !address->key_id->change
                                                  ? std::optional<uint32_t>()
                                                  : address->key_id->index;
  keyring_service_->UpdateNextUnusedAddressForZCashAccount(
      account_id, next_receive_index, next_change_index);
}

void ZCashWalletService::DiscoverNextUnusedAddress(
    const mojom::AccountIdPtr& account_id,
    bool change,
    DiscoverNextUnusedAddressCallback callback) {
  CHECK(IsZCashAccount(*account_id));

  auto account_info = keyring_service_->GetZCashAccountInfo(account_id);
  if (!account_info) {
    return std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  }
  auto start_address =
      change ? account_info->next_transparent_change_address.Clone()
             : account_info->next_transparent_receive_address.Clone();
  auto task = base::MakeRefCounted<DiscoverNextUnusedZCashAddressTask>(
      weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
      std::move(start_address), std::move(callback));
  task->ScheduleWorkOnTask();
}

void ZCashWalletService::GetUtxos(const std::string& chain_id,
                                  mojom::AccountIdPtr account_id,
                                  GetUtxosCallback callback) {
  if (!IsZCashNetwork(chain_id)) {
    // Desktop frontend sometimes does that.
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  const auto& addresses = keyring_service_->GetZCashAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto context = base::MakeRefCounted<GetTransparentUtxosContext>();

  context->callback = std::move(callback);
  for (const auto& address : addresses.value()) {
    context->addresses.insert(address->address_string);
  }

  if (context->addresses.empty()) {
    std::move(context->callback).Run(UtxoMap());
    return;
  }

  // Copy context->addresses to allow sync calls in tests
  for (const auto& address : std::set<std::string>(context->addresses)) {
    zcash_rpc_->GetUtxoList(
        chain_id, address,
        base::BindOnce(&ZCashWalletService::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr(), context, address));
  }
}

bool ZCashWalletService::SignTransactionInternal(
    ZCashTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  return ZCashSerializer::SignTransparentPart(keyring_service_, account_id, tx);
}

void ZCashWalletService::SignAndPostTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction zcash_transaction,
    SignAndPostTransactionCallback callback) {
  zcash_rpc_->GetLatestBlock(
      chain_id,
      base::BindOnce(
          &ZCashWalletService::OnResolveLastBlockHeightForSendTransaction,
          weak_ptr_factory_.GetWeakPtr(), chain_id, account_id.Clone(),
          std::move(zcash_transaction), std::move(callback)));
}

void ZCashWalletService::SetZCashRpcForTesting(
    std::unique_ptr<ZCashRpc> zcash_rpc) {
  zcash_rpc_ = std::move(zcash_rpc);
}

void ZCashWalletService::ValidateZCashAddress(
    const std::string& addr,
    bool testnet,
    ValidateZCashAddressCallback callback) {
  if (IsUnifiedAddress(addr)) {
    if (testnet != IsUnifiedTestnetAddress(addr)) {
      std::move(callback).Run(
          mojom::ZCashAddressValidationResult::NetworkMismatch);
      return;
    }
    if (!ExtractTransparentPart(addr, testnet)) {
      std::move(callback).Run(
          mojom::ZCashAddressValidationResult::InvalidUnified);
      return;
    }
  } else {
    auto decoded = DecodeZCashAddress(addr);
    if (!decoded) {
      std::move(callback).Run(
          mojom::ZCashAddressValidationResult::InvalidTransparent);
      return;
    }
    if (decoded->testnet != testnet) {
      std::move(callback).Run(
          mojom::ZCashAddressValidationResult::NetworkMismatch);
      return;
    }
  }

  std::move(callback).Run(mojom::ZCashAddressValidationResult::Success);
}

void ZCashWalletService::PostShieldTransaction(
    const std::string& chain_id,
    ZCashTransaction zcash_transaction,
    SignAndPostTransactionCallback callback) {
  auto tx = ZCashSerializer::SerializeRawTransaction(zcash_transaction);
  std::string as_string(reinterpret_cast<char*>(tx.data()), tx.size());
  zcash_rpc_->SendTransaction(
      chain_id, as_string,
      base::BindOnce(&ZCashWalletService::OnSendTransactionResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(zcash_transaction)));
}

void ZCashWalletService::OnResolveLastBlockHeightForSendTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction zcash_transaction,
    SignAndPostTransactionCallback callback,
    base::expected<mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    std::move(callback).Run(
        "", std::move(zcash_transaction),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  zcash_transaction.set_expiry_height((*result)->height +
                                      kDefaultZCashBlockHeightDelta);

  if (!zcash_transaction.IsTransparentPartSigned() &&
      !SignTransactionInternal(zcash_transaction, account_id)) {
    std::move(callback).Run(
        "", std::move(zcash_transaction),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto tx = ZCashSerializer::SerializeRawTransaction(zcash_transaction);
  std::string as_string(reinterpret_cast<char*>(tx.data()), tx.size());
  zcash_rpc_->SendTransaction(
      chain_id, as_string,
      base::BindOnce(&ZCashWalletService::OnSendTransactionResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(zcash_transaction)));
}

void ZCashWalletService::OnSendTransactionResult(
    SignAndPostTransactionCallback callback,
    ZCashTransaction tx,
    base::expected<mojom::SendResponsePtr, std::string> result) {
  if (result.has_value() && result.value() && (*result)->error_code == 0) {
    auto tx_id = ZCashSerializer::CalculateTxIdDigest(tx);
    auto tx_id_hex = ToHex(tx_id);
    CHECK(tx_id_hex.starts_with("0x"));
    std::move(callback).Run(tx_id_hex.substr(2), std::move(tx), "");
  } else {
    std::move(callback).Run(
        "", std::move(tx), l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  }
}

void ZCashWalletService::OnGetUtxos(
    scoped_refptr<GetTransparentUtxosContext> context,
    const std::string& address,
    base::expected<mojom::GetAddressUtxosResponsePtr, std::string> result) {
  DCHECK(context->addresses.contains(address));
  DCHECK(!context->utxos.contains(address));

  if (!result.has_value() || !result.value()) {
    context->SetError(result.error());
    WorkOnGetUtxos(std::move(context));
    return;
  }

  context->addresses.erase(address);
  context->utxos[address] = std::move(result.value()->address_utxos);

  WorkOnGetUtxos(std::move(context));
}

void ZCashWalletService::WorkOnGetUtxos(
    scoped_refptr<GetTransparentUtxosContext> context) {
  if (!context->ShouldRespond()) {
    return;
  }

  if (context->error) {
    std::move(context->callback)
        .Run(base::unexpected(std::move(*context->error)));
    return;
  }

  std::move(context->callback).Run(std::move(context->utxos));
}

void ZCashWalletService::OnDiscoveryDoneForBalance(
    mojom::AccountIdPtr account_id,
    std::string chain_id,
    GetBalanceCallback callback,
    RunDiscoveryResult discovery_result) {
  if (!discovery_result.has_value()) {
    std::move(callback).Run(
        nullptr, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  GetUtxos(chain_id, std::move(account_id),
           base::BindOnce(&ZCashWalletService::OnUtxosResolvedForBalance,
                          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashWalletService::OnUtxosResolvedForBalance(
    GetBalanceCallback initial_callback,
    base::expected<UtxoMap, std::string> utxos) {
  if (!utxos.has_value()) {
    std::move(initial_callback).Run(nullptr, utxos.error());
    return;
  }

  auto result = mojom::ZCashBalance::New();
  result->total_balance = 0;
  for (const auto& by_addr : utxos.value()) {
    uint64_t balance_by_addr = 0;
    for (const auto& by_utxo : by_addr.second) {
      balance_by_addr += by_utxo->value_zat;
    }
    result->total_balance += balance_by_addr;
    result->balances[by_addr.first] = balance_by_addr;
  }
  std::move(initial_callback).Run(std::move(result), std::nullopt);
}

void ZCashWalletService::CreateTransaction(const std::string& chain_id,
                                           mojom::AccountIdPtr account_id,
                                           const std::string& address_to,
                                           uint64_t amount,
                                           CreateTransactionCallback callback) {
  std::string final_address = address_to;
  if (IsUnifiedAddress(address_to)) {
    auto transparent =
        ExtractTransparentPart(address_to, chain_id == mojom::kZCashTestnet);
    if (!transparent) {
      std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
          IDS_BRAVE_WALLET_ZCASH_UNIFIED_ADDRESS_ERROR)));
      return;
    }
    final_address = transparent.value();
  }

  auto& task = create_transaction_tasks_.emplace_back(
      std::make_unique<CreateTransparentTransactionTask>(
          this, chain_id, account_id, final_address, amount,
          std::move(callback)));
  task->ScheduleWorkOnTask();
}

void ZCashWalletService::ShieldFunds(const std::string& chain_id,
                                     mojom::AccountIdPtr account_id,
                                     ShieldFundsCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  CreateShieldAllTransaction(
      chain_id, account_id.Clone(),
      base::BindOnce(&ZCashWalletService::CreateShieldTransactionTaskDone,
                     weak_ptr_factory_.GetWeakPtr(), chain_id,
                     account_id.Clone(), std::move(callback)));
#else
  std::move(callback).Run(base::unexpected("Unsupported"));
#endif
}

#if BUILDFLAG(ENABLE_ORCHARD)

void ZCashWalletService::CreateShieldAllTransaction(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    CreateTransactionCallback callback) {
  if (!IsZCashShieldedTransactionsEnabled()) {
    NOTREACHED();
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (shield_funds_task_ || !create_transaction_tasks_.empty()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  shield_funds_task_ = std::make_unique<CreateShieldAllTransactionTask>(
      this, chain_id, account_id, std::move(callback),
      random_seed_for_testing_);
  shield_funds_task_->ScheduleWorkOnTask();
}

void ZCashWalletService::CreateShieldTransactionTaskDone(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    ShieldFundsCallback callback,
    base::expected<ZCashTransaction, std::string> transaction) {
  if (!transaction.has_value()) {
    std::move(callback).Run(
        std::nullopt, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  PostShieldTransaction(
      chain_id, std::move(transaction.value()),
      base::BindOnce(&ZCashWalletService::OnPostShieldTransactionDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashWalletService::OnPostShieldTransactionDone(
    ShieldFundsCallback callback,
    std::string tx_id,
    ZCashTransaction transaction,
    std::string error) {
  shield_funds_task_ = nullptr;
  if (!tx_id.empty()) {
    std::move(callback).Run(tx_id, std::nullopt);
  } else {
    std::move(callback).Run(std::nullopt, error);
  }
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashWalletService::GetTransactionStatus(
    const std::string& chain_id,
    const std::string& tx_hash,
    GetTransactionStatusCallback callback) {
  zcash_rpc_->GetTransaction(
      chain_id, tx_hash,
      base::BindOnce(&ZCashWalletService::OnTransactionResolvedForStatus,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashWalletService::OnTransactionResolvedForStatus(
    GetTransactionStatusCallback callback,
    base::expected<mojom::RawTransactionPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    std::move(callback).Run(base::unexpected(result.error()));
    return;
  }

  std::move(callback).Run((*result)->height > 0);
}

void ZCashWalletService::CreateTransactionTaskDone(
    CreateTransparentTransactionTask* task) {
  CHECK(create_transaction_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}

ZCashRpc* ZCashWalletService::zcash_rpc() {
  return zcash_rpc_.get();
}

KeyringService* ZCashWalletService::keyring_service() {
  return keyring_service_.get();
}

void ZCashWalletService::Unlocked() {
  auto accounts = keyring_service_->GetAllAccountsSync();
  for (const auto& account : accounts->accounts) {
    if (IsZCashKeyring(account->account_id->keyring_id)) {
      RunDiscovery(account->account_id.Clone(), base::DoNothing());
    }
  }
}

}  // namespace brave_wallet
