/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <optional>
#include <set>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_transaction_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_discover_next_unused_zcash_address_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_get_transparent_utxos_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_balance_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_transaction_status_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/brave_wallet/browser/zcash/zcash_auto_sync_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_orchard_transaction_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_to_orchard_transaction_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_get_zcash_chain_tip_status_task.h"
#endif  // BUILDFLAG(ENABLE_ORCHARD)

namespace brave_wallet {
#if BUILDFLAG(ENABLE_ORCHARD)
inline constexpr char kOrchardDatabaseName[] = "orchard.db";
#endif  // BUILDFLAG(ENABLE_ORCHARD)

namespace {

#if BUILDFLAG(ENABLE_ORCHARD)
// Creates address key id for receiving funds on internal orchard address
mojom::ZCashKeyIdPtr CreateOrchardInternalKeyId(
    const mojom::AccountIdPtr& account_id) {
  return mojom::ZCashKeyId::New(account_id->account_index, 1 /* internal */, 0);
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace

void ZCashWalletService::Bind(
    mojo::PendingReceiver<mojom::ZCashWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

ZCashWalletService::ZCashWalletService(
    base::FilePath zcash_data_path,
    KeyringService& keyring_service,
    NetworkManager* network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : zcash_data_path_(std::move(zcash_data_path)),
      keyring_service_(keyring_service),
      zcash_rpc_(
          std::make_unique<ZCashRpc>(network_manager, url_loader_factory)) {
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
#if BUILDFLAG(ENABLE_ORCHARD)
  sync_state_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      zcash_data_path_.AppendASCII(kOrchardDatabaseName));
#endif
}

ZCashWalletService::ZCashWalletService(base::FilePath zcash_data_path,
                                       KeyringService& keyring_service,
                                       std::unique_ptr<ZCashRpc> zcash_rpc)
    : zcash_data_path_(std::move(zcash_data_path)),
      keyring_service_(keyring_service),
      zcash_rpc_(std::move(zcash_rpc)) {
  CHECK_IS_TEST();
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
#if BUILDFLAG(ENABLE_ORCHARD)
  sync_state_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      zcash_data_path_.AppendASCII(kOrchardDatabaseName));
#endif
}

ZCashWalletService::~ZCashWalletService() = default;

void ZCashWalletService::GetBalance(mojom::AccountIdPtr account_id,
                                    GetBalanceCallback callback) {
  auto [task_it, inserted] =
      resolve_balance_tasks_.insert(std::make_unique<ZCashResolveBalanceTask>(
          base::PassKey<ZCashWalletService>(), *this,
          CreateActionContext(account_id)));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(&ZCashWalletService::OnResolveBalanceResult,
                                 weak_ptr_factory_.GetWeakPtr(), task_ptr,
                                 std::move(callback)));
}

void ZCashWalletService::OnResolveBalanceResult(
    ZCashResolveBalanceTask* task,
    GetBalanceCallback callback,
    base::expected<mojom::ZCashBalancePtr, std::string> result) {
  CHECK(resolve_balance_tasks_.erase(task));

  if (result.has_value()) {
    std::move(callback).Run(std::move(result.value()), std::nullopt);
  } else {
    std::move(callback).Run(nullptr, result.error());
  }
}

void ZCashWalletService::GetReceiverAddress(
    mojom::AccountIdPtr account_id,
    GetReceiverAddressCallback callback) {
  auto id = mojom::ZCashKeyId::New(account_id->account_index, 0, 0);
  auto addr = keyring_service_->GetZCashAddress(account_id, *id);
  if (!addr) {
    std::move(callback).Run(nullptr, WalletInternalErrorMessage());
    return;
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

void ZCashWalletService::MakeAccountShielded(
    mojom::AccountIdPtr account_id,
    uint32_t account_birthday_block,
    MakeAccountShieldedCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    // Only 1 account can be shieldable at the moment
    const auto& accounts = keyring_service_->GetAllAccountInfos();
    for (const auto& account : accounts) {
      if (IsZCashAccount(account->account_id) &&
          !GetAccountShieldBirthday(account->account_id).is_null()) {
        std::move(callback).Run("Already has shieldable account");
        return;
      }
    }
    if (account_birthday_block == 0) {
      GetLatestBlockForAccountBirthday(account_id.Clone(), std::move(callback));
    } else {
      GetTreeStateForAccountBirthday(
          std::move(account_id), account_birthday_block, std::move(callback));
    }
    return;
  }
#endif
  std::move(callback).Run("Not supported");
}

void ZCashWalletService::StartShieldSync(mojom::AccountIdPtr account_id,
                                         uint32_t to,
                                         StartShieldSyncCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    auto account_birthday = GetAccountShieldBirthday(account_id);
    if (!account_birthday) {
      std::move(callback).Run("Account not supported");
      return;
    }

    auto fvk = keyring_service_->GetOrchardFullViewKey(account_id);
    if (!fvk) {
      std::move(callback).Run("Cannot resolve FVK");
      return;
    }

    if (shield_sync_services_.find(account_id) != shield_sync_services_.end()) {
      std::move(callback).Run("Already in sync");
      return;
    }

    shield_sync_services_[account_id.Clone()] =
        std::make_unique<ZCashShieldSyncService>(
            *this, CreateActionContext(account_id), account_birthday,
            fvk.value(), weak_ptr_factory_.GetWeakPtr());

    shield_sync_services_[account_id.Clone()]->StartSyncing(
        to == 0 ? std::nullopt : std::optional<uint32_t>(to));

    std::move(callback).Run(std::nullopt);
    return;
  }
#endif
  std::move(callback).Run("Not supported");
}

void ZCashWalletService::StopShieldSync(mojom::AccountIdPtr account_id,
                                        StopShieldSyncCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    auto it = shield_sync_services_.find(account_id);
    if (it != shield_sync_services_.end()) {
      shield_sync_services_.erase(it);
      OnSyncStop(account_id);
    } else {
      std::move(callback).Run("Not syncing");
      return;
    }
    std::move(callback).Run(std::nullopt);
    return;
  }
#endif
  std::move(callback).Run("Not supported");
}

void ZCashWalletService::IsSyncInProgress(mojom::AccountIdPtr account_id,
                                          IsSyncInProgressCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    std::move(callback).Run(shield_sync_services_.contains(account_id),
                            std::nullopt);
    return;
  }
#endif  // BUILDFLAG(ENABLE_ORCHARD)
  std::move(callback).Run(false, "Not supported");
}

void ZCashWalletService::GetChainTipStatus(mojom::AccountIdPtr account_id,
                                           GetChainTipStatusCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    auto [task_it, inserted] = get_zcash_chain_tip_status_tasks_.insert(
        std::make_unique<ZCashGetZCashChainTipStatusTask>(
            base::PassKey<ZCashWalletService>(), *this,
            CreateActionContext(account_id)));
    CHECK(inserted);
    auto* task_ptr = task_it->get();

    task_ptr->Start(base::BindOnce(
        &ZCashWalletService::OnGetChainTipStatusResult,
        weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
  }
#else
  std::move(callback).Run(nullptr, "Not supported");
#endif
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
      std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
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
  CHECK(IsZCashAccount(account_id));

  auto account_info = keyring_service_->GetZCashAccountInfo(account_id);
  if (!account_info) {
    return std::move(callback).Run(
        base::unexpected(WalletInternalErrorMessage()));
  }
  auto start_address =
      change ? account_info->next_transparent_change_address.Clone()
             : account_info->next_transparent_receive_address.Clone();
  auto task = base::MakeRefCounted<ZCashDiscoverNextUnusedZCashAddressTask>(
      base::PassKey<ZCashWalletService>(), weak_ptr_factory_.GetWeakPtr(),
      account_id, start_address, std::move(callback));
  task->Start();
}

void ZCashWalletService::GetUtxos(const mojom::AccountIdPtr& account_id,
                                  GetUtxosCallback callback) {
  if (!IsZCashAccount(account_id)) {
    // Desktop frontend sometimes does that.
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  const auto& addresses = keyring_service_->GetZCashAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  auto context = base::WrapRefCounted<ZCashGetTransparentUtxosContext>(
      new ZCashGetTransparentUtxosContext());

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
        GetNetworkForZCashAccount(account_id), address,
        base::BindOnce(&ZCashWalletService::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr(), context, address));
  }
}

void ZCashWalletService::OnCompleteTransactionTaskDone(
    ZCashCompleteTransactionTask* task,
    mojom::AccountIdPtr account_id,
    ZCashTransaction original_zcash_transaction,
    SignAndPostTransactionCallback callback,
    base::expected<ZCashTransaction, std::string> result) {
  CHECK(complete_transaction_tasks_.erase(task));
  CHECK(original_zcash_transaction.ValidateAmounts());

  if (!result.has_value()) {
    std::move(callback).Run("", std::move(original_zcash_transaction),
                            result.error());
    return;
  }

  auto tx = ZCashSerializer::SerializeRawTransaction(result.value());

  zcash_rpc_->SendTransaction(
      GetNetworkForZCashAccount(account_id), std::move(tx),
      base::BindOnce(&ZCashWalletService::OnSendTransactionResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(result.value())));
}

void ZCashWalletService::SignAndPostTransaction(
    const mojom::AccountIdPtr& account_id,
    const ZCashTransaction& zcash_transaction,
    SignAndPostTransactionCallback callback) {
  auto [task_it, inserted] = complete_transaction_tasks_.insert(
      std::make_unique<ZCashCompleteTransactionTask>(
          base::PassKey<ZCashWalletService>(), *this,
          CreateActionContext(account_id), keyring_service_.get(),
          zcash_transaction));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &ZCashWalletService::OnCompleteTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, account_id.Clone(),
      zcash_transaction, std::move(callback)));
}

void ZCashWalletService::SetZCashRpcForTesting(
    std::unique_ptr<ZCashRpc> zcash_rpc) {
  zcash_rpc_ = std::move(zcash_rpc);
}

void ZCashWalletService::AddObserver(
    mojo::PendingRemote<mojom::ZCashWalletServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

base::expected<mojom::ZCashTxType, mojom::ZCashAddressError>
ZCashWalletService::GetTransactionType(const mojom::AccountIdPtr& account_id,
                                       bool use_shielded_pool,
                                       const std::string& addr) {
  if (!IsZCashAccount(account_id)) {
    return base::unexpected(mojom::ZCashAddressError::kNotZCashAccount);
  }
  bool testnet = IsZCashTestnetKeyring(account_id->keyring_id);

#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    if (use_shielded_pool) {
      auto validation_result = ValidateOrchardRecipientAddress(testnet, addr);
      if (validation_result.has_value()) {
        return base::ok(mojom::ZCashTxType::kOrchardToOrchard);
      }
      return base::unexpected(validation_result.error());
    }

    if (ValidateOrchardRecipientAddress(testnet, addr).has_value()) {
      const auto& account_infos = keyring_service_->GetAllAccountInfos();
      for (const auto& account_info : account_infos) {
        if (account_info->account_id->keyring_id != account_id->keyring_id) {
          continue;
        }
        auto zcash_account_info =
            keyring_service_->GetZCashAccountInfo(account_info->account_id);
        if (zcash_account_info->orchard_internal_address == addr) {
          return base::ok(mojom::ZCashTxType::kShielding);
        }
      }
      return base::ok(mojom::ZCashTxType::kTransparentToOrchard);
    }
  }
#endif
  auto validation_result = ValidateTransparentRecipientAddress(testnet, addr);
  if (validation_result.has_value()) {
    return base::ok(mojom::ZCashTxType::kTransparentToTransparent);
  }

  return base::unexpected(validation_result.error());
}

void ZCashWalletService::GetTransactionType(
    mojom::AccountIdPtr account_id,
    bool use_shielded_pool,
    const std::string& addr,
    GetTransactionTypeCallback callback) {
  auto result = GetTransactionType(account_id, use_shielded_pool, addr);
  if (result.has_value()) {
    std::move(callback).Run(result.value(), mojom::ZCashAddressError::kNoError);
  } else {
    std::move(callback).Run(mojom::ZCashTxType::kUnknown, result.error());
  }
}

void ZCashWalletService::OnSendTransactionResult(
    SignAndPostTransactionCallback callback,
    ZCashTransaction tx,
    base::expected<zcash::mojom::SendResponsePtr, std::string> result) {
  if (result.has_value() && result.value() && (*result)->error_code == 0) {
    auto tx_id = ZCashSerializer::CalculateTxIdDigest(tx);
    auto tx_id_hex = ToHex(tx_id);
    CHECK(tx_id_hex.starts_with("0x"));
    std::move(callback).Run(tx_id_hex.substr(2), std::move(tx), "");
  } else {
    std::move(callback).Run("", std::move(tx), WalletInternalErrorMessage());
  }
}

void ZCashWalletService::OnDiscoveryDoneForBalance(
    mojom::AccountIdPtr account_id,
    GetBalanceCallback callback,
    RunDiscoveryResult discovery_result) {
  if (!discovery_result.has_value()) {
    std::move(callback).Run(nullptr, WalletInternalErrorMessage());
    return;
  }
  GetUtxos(std::move(account_id),
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

void ZCashWalletService::OnGetUtxos(
    scoped_refptr<ZCashGetTransparentUtxosContext> context,
    const std::string& address,
    base::expected<zcash::mojom::GetAddressUtxosResponsePtr, std::string>
        result) {
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
    scoped_refptr<ZCashGetTransparentUtxosContext> context) {
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

void ZCashWalletService::CreateFullyTransparentTransaction(
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback) {
  std::string final_address = address_to;
  if (IsUnifiedAddress(address_to)) {
    auto transparent = ExtractTransparentPart(
        address_to, IsZCashTestnetKeyring(account_id->keyring_id));
    if (!transparent) {
      std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
          IDS_BRAVE_WALLET_ZCASH_UNIFIED_ADDRESS_ERROR)));
      return;
    }
    final_address = transparent.value();
  }

  auto [task_it, inserted] = create_transaction_tasks_.insert(
      std::make_unique<ZCashCreateTransparentTransactionTask>(
          base::PassKey<ZCashWalletService>(), *this,
          CreateActionContext(account_id), final_address, amount));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &ZCashWalletService::OnCreateTransparentTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void ZCashWalletService::ShieldAllFunds(mojom::AccountIdPtr account_id,
                                        ShieldAllFundsCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    CreateShieldAllTransaction(
        account_id.Clone(),
        base::BindOnce(&ZCashWalletService::CreateShieldAllTransactionTaskDone,
                       weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
                       std::move(callback)));
  } else {
    std::move(callback).Run(
        std::nullopt,
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
  }
#else
  std::move(callback).Run(
      std::nullopt,
      l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
#endif
}

void ZCashWalletService::ResetSyncState(mojom::AccountIdPtr account_id,
                                        ResetSyncStateCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    if (shield_sync_services_.find(account_id) != shield_sync_services_.end()) {
      std::move(callback).Run("Sync in progress");
      return;
    }
    sync_state_.AsyncCall(&OrchardSyncState::ResetAccountSyncState)
        .WithArgs(account_id.Clone())
        .Then(base::BindOnce(&ZCashWalletService::OnResetSyncState,
                             weak_ptr_factory_.GetWeakPtr(),
                             std::move(callback)));
  } else {
    std::move(callback).Run(
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
  }
#else
  std::move(callback).Run(
      l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
#endif
}

#if BUILDFLAG(ENABLE_ORCHARD)
void ZCashWalletService::MaybeInitAutoSyncManagers() {
  if (!IsZCashShieldedTransactionsEnabled()) {
    return;
  }
  if (keyring_service_->IsLockedSync()) {
    return;
  }
  const auto& accounts = keyring_service_->GetAllAccountInfos();
  for (const auto& account : accounts) {
    if (account->account_id->coin != mojom::CoinType::ZEC) {
      continue;
    }

    if (!IsZCashAccount(account->account_id)) {
      continue;
    }

    auto account_info =
        keyring_service_->GetZCashAccountInfo(account->account_id);
    if (!account_info || !account_info->account_shield_birthday) {
      continue;
    }

    if (auto_sync_managers_.contains(account->account_id)) {
      continue;
    }

    auto async_manager = std::make_unique<ZCashAutoSyncManager>(
        *this, CreateActionContext(account->account_id));
    async_manager->Start();
    auto_sync_managers_[account->account_id.Clone()] = std::move(async_manager);
  }
}

void ZCashWalletService::CreateShieldAllTransaction(
    mojom::AccountIdPtr account_id,
    CreateTransactionCallback callback) {
  CHECK(IsZCashShieldedTransactionsEnabled());

  auto internal_addr = keyring_service_->GetOrchardRawBytes(
      account_id, CreateOrchardInternalKeyId(account_id));

  auto [task_it, inserted] = create_shield_transaction_tasks_.insert(
      std::make_unique<ZCashCreateTransparentToOrchardTransactionTask>(
          base::PassKey<ZCashWalletService>(), *this,
          CreateActionContext(account_id), *internal_addr, std::nullopt,
          kZCashFullAmount));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &ZCashWalletService::OnCreateTransparentToOrchardTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void ZCashWalletService::CreateOrchardToOrchardTransaction(
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    std::optional<OrchardMemo> memo,
    CreateTransactionCallback callback) {
  auto receiver_addr = GetOrchardRawBytes(
      address_to, IsZCashTestnetKeyring(account_id->keyring_id));
  if (!receiver_addr) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  auto [task_it, inserted] = create_shielded_transaction_tasks_.insert(
      std::make_unique<ZCashCreateOrchardToOrchardTransactionTask>(
          base::PassKey<ZCashWalletService>(), *this,
          CreateActionContext(account_id), *receiver_addr, std::move(memo),
          amount));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &ZCashWalletService::OnCreateOrchardToOrchardTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void ZCashWalletService::CreateTransparentToOrchardTransaction(
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    std::optional<OrchardMemo> memo,
    CreateTransactionCallback callback) {
  CHECK(IsZCashShieldedTransactionsEnabled());

  auto receiver_addr = GetOrchardRawBytes(
      address_to, IsZCashTestnetKeyring(account_id->keyring_id));
  if (!receiver_addr) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  auto [task_it, inserted] = create_shield_transaction_tasks_.insert(
      std::make_unique<ZCashCreateTransparentToOrchardTransactionTask>(
          base::PassKey<ZCashWalletService>(), *this,
          CreateActionContext(account_id), *receiver_addr, std::move(memo),
          amount));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &ZCashWalletService::OnCreateTransparentToOrchardTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void ZCashWalletService::OnCreateTransparentToOrchardTransactionTaskDone(
    ZCashCreateTransparentToOrchardTransactionTask* task,
    CreateTransactionCallback callback,
    base::expected<ZCashTransaction, std::string> result) {
  CHECK(create_shield_transaction_tasks_.erase(task));

  std::move(callback).Run(result);
}

void ZCashWalletService::OnCreateOrchardToOrchardTransactionTaskDone(
    ZCashCreateOrchardToOrchardTransactionTask* task,
    CreateTransactionCallback callback,
    base::expected<ZCashTransaction, std::string> result) {
  CHECK(create_shielded_transaction_tasks_.erase(task));

  std::move(callback).Run(result);
}

void ZCashWalletService::CreateShieldAllTransactionTaskDone(
    mojom::AccountIdPtr account_id,
    ShieldAllFundsCallback callback,
    base::expected<ZCashTransaction, std::string> transaction) {
  if (!transaction.has_value()) {
    std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
    return;
  }
  SignAndPostTransaction(
      account_id.Clone(), std::move(transaction.value()),
      base::BindOnce(&ZCashWalletService::OnPostShieldTransactionDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashWalletService::OnPostShieldTransactionDone(
    ShieldAllFundsCallback callback,
    std::string tx_id,
    ZCashTransaction transaction,
    std::string error) {
  if (!tx_id.empty()) {
    std::move(callback).Run(tx_id, std::nullopt);
  } else {
    std::move(callback).Run(std::nullopt, error);
  }
}

void ZCashWalletService::GetLatestBlockForAccountBirthday(
    mojom::AccountIdPtr account_id,
    MakeAccountShieldedCallback callback) {
  CHECK(account_id);
  zcash_rpc_->GetLatestBlock(
      GetNetworkForZCashKeyring(account_id->keyring_id),
      base::BindOnce(&ZCashWalletService::OnGetLatestBlockForAccountBirthday,
                     weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
                     std::move(callback)));
}

void ZCashWalletService::OnGetLatestBlockForAccountBirthday(
    mojom::AccountIdPtr account_id,
    MakeAccountShieldedCallback callback,
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  CHECK(account_id);
  if (!result.has_value() || !result.value()) {
    std::move(callback).Run("Failed to retrieve latest block");
    return;
  }

  GetTreeStateForAccountBirthday(std::move(account_id), (*result)->height,
                                 std::move(callback));
}

void ZCashWalletService::GetTreeStateForAccountBirthday(
    mojom::AccountIdPtr account_id,
    uint32_t block_id,
    MakeAccountShieldedCallback callback) {
  // Get block info for the block that is back from latest block for
  // kChainReorgBlockDelta to ensure account birthday won't be affected by chain
  // reorg.
  if (block_id < kChainReorgBlockDelta) {
    std::move(callback).Run("Failed to retrieve latest block");
    return;
  }

  auto block_id_param = zcash::mojom::BlockID::New(
      block_id - kChainReorgBlockDelta, std::vector<uint8_t>());

  zcash_rpc_->GetTreeState(
      GetNetworkForZCashKeyring(account_id->keyring_id),
      std::move(block_id_param),
      base::BindOnce(&ZCashWalletService::OnGetTreeStateForAccountBirthday,
                     weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
                     std::move(callback)));
}

void ZCashWalletService::OnGetTreeStateForAccountBirthday(
    mojom::AccountIdPtr account_id,
    MakeAccountShieldedCallback callback,
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    std::move(callback).Run("Failed to retrieve tree state");
    return;
  }

  keyring_service_->SetZCashAccountBirthday(
      account_id, mojom::ZCashAccountShieldBirthday::New((*result)->height,
                                                         (*result)->hash));

  MaybeInitAutoSyncManagers();

  std::move(callback).Run(std::nullopt);
}

mojom::ZCashAccountShieldBirthdayPtr
ZCashWalletService::GetAccountShieldBirthday(
    const mojom::AccountIdPtr& account_id) {
  auto account_info = keyring_service_->GetZCashAccountInfo(account_id);
  if (!account_info || !account_info->account_shield_birthday) {
    return nullptr;
  }
  return account_info->account_shield_birthday.Clone();
}

void ZCashWalletService::OnSyncStart(const mojom::AccountIdPtr& account_id) {
  for (const auto& observer : observers_) {
    observer->OnSyncStart(account_id.Clone());
  }
}

void ZCashWalletService::OnSyncStop(const mojom::AccountIdPtr& account_id) {
  for (const auto& observer : observers_) {
    observer->OnSyncStop(account_id.Clone());
  }
}

void ZCashWalletService::OnSyncError(const mojom::AccountIdPtr& account_id,
                                     const std::string& error) {
  for (const auto& observer : observers_) {
    observer->OnSyncError(account_id.Clone(), error);
  }
}

void ZCashWalletService::OnSyncStatusUpdate(
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashShieldSyncStatusPtr& status) {
  for (const auto& observer : observers_) {
    observer->OnSyncStatusUpdate(account_id.Clone(), status.Clone());
  }
}

void ZCashWalletService::OnResetSyncState(
    ResetSyncStateCallback callback,
    base::expected<OrchardStorage::Result, OrchardStorage::Error> result) {
  if (result.has_value()) {
    std::move(callback).Run(
        result.value() == OrchardStorage::Result::kSuccess
            ? std::nullopt
            : std::optional<std::string>("Account data wasn't deleted"));
    return;
  }

  std::move(callback).Run(result.error().message);
}

void ZCashWalletService::OnGetChainTipStatusResult(
    ZCashGetZCashChainTipStatusTask* task,
    GetChainTipStatusCallback callback,
    base::expected<mojom::ZCashChainTipStatusPtr, std::string> result) {
  CHECK(get_zcash_chain_tip_status_tasks_.erase(task));

  if (result.has_value()) {
    std::move(callback).Run(std::move(result.value()), std::nullopt);
  } else {
    std::move(callback).Run(nullptr, result.error());
  }
}

void ZCashWalletService::OnSyncFinished(const mojom::AccountIdPtr& account_id) {
  shield_sync_services_.erase(account_id);
}

base::SequenceBound<OrchardSyncState>& ZCashWalletService::sync_state() {
  return sync_state_;
}

void ZCashWalletService::OverrideSyncStateForTesting(
    base::SequenceBound<OrchardSyncState> sync_state) {
  sync_state_ = std::move(sync_state);
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashWalletService::GetTransactionStatus(
    const mojom::AccountIdPtr& account_id,
    std::unique_ptr<ZCashTxMeta> tx_meta,
    GetTransactionStatusCallback callback) {
  auto [task_it, inserted] = resolve_transaction_status_tasks_.insert(
      std::make_unique<ZCashResolveTransactionStatusTask>(
          base::PassKey<ZCashWalletService>(), CreateActionContext(account_id),
          *this, std::move(tx_meta)));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &ZCashWalletService::OnTransactionResolvedForStatus,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void ZCashWalletService::OnTransactionResolvedForStatus(
    ZCashResolveTransactionStatusTask* task,
    GetTransactionStatusCallback callback,
    base::expected<ResolveTransactionStatusResult, std::string> result) {
  CHECK(resolve_transaction_status_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

void ZCashWalletService::OnCreateTransparentTransactionTaskDone(
    ZCashCreateTransparentTransactionTask* task,
    CreateTransactionCallback callback,
    base::expected<ZCashTransaction, std::string> result) {
  CHECK(create_transaction_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

ZCashRpc& ZCashWalletService::zcash_rpc() {
  return *zcash_rpc_;
}

KeyringService& ZCashWalletService::keyring_service() {
  return *keyring_service_;
}

void ZCashWalletService::Unlocked() {
  auto accounts = keyring_service_->GetAllAccountsSync();
  for (const auto& account : accounts->accounts) {
    if (IsZCashKeyring(account->account_id->keyring_id)) {
      RunDiscovery(account->account_id.Clone(), base::DoNothing());
    }
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  MaybeInitAutoSyncManagers();
#endif  // BUILDFLAG(ENABLE_ORCHARD)
}

void ZCashWalletService::Locked() {
#if BUILDFLAG(ENABLE_ORCHARD)
  auto_sync_managers_.clear();
  shield_sync_services_.clear();
#endif  // BUILDFLAG(ENABLE_ORCHARD)
}

void ZCashWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
#if BUILDFLAG(ENABLE_ORCHARD)
  shield_sync_services_.clear();
  sync_state_.AsyncCall(&OrchardSyncState::ResetDatabase);
#endif  // BUILDFLAG(ENABLE_ORCHARD)
}

ZCashActionContext ZCashWalletService::CreateActionContext(
    const mojom::AccountIdPtr& account_id) {
#if BUILDFLAG(ENABLE_ORCHARD)
  std::optional<OrchardAddrRawPart> internal_addr;
  if (IsZCashShieldedTransactionsEnabled()) {
    internal_addr = keyring_service_->GetOrchardRawBytes(
        account_id, mojom::ZCashKeyId::New(account_id->account_index, 1, 0));
  }
#endif
  return ZCashActionContext(*zcash_rpc_,
#if BUILDFLAG(ENABLE_ORCHARD)
                            internal_addr, sync_state_,
#endif
                            account_id.Clone());
}

base::PassKey<ZCashWalletService>
ZCashWalletService::CreatePassKeyForTesting() {
  CHECK_IS_TEST();
  return base::PassKey<ZCashWalletService>();
}

}  // namespace brave_wallet
