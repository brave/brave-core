/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <optional>
#include <set>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_transaction_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_discover_next_unused_zcash_address_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_get_transparent_utxos_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_balance_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/brave_wallet/browser/zcash/zcash_create_shield_transaction_task.h"
#endif  // BUILDFLAG(ENABLE_ORCHARD)

namespace brave_wallet {
#if BUILDFLAG(ENABLE_ORCHARD)
inline constexpr char kOrchardDatabaseName[] = "orchard.db";
#endif  // BUILDFLAG(ENABLE_ORCHARD)

namespace {

#if BUILDFLAG(ENABLE_ORCHARD)
// Creates address key id for receiving funds on internal orchard address
mojom::ZCashKeyIdPtr CreateOrchardInternalKeyId(
    const mojom::AccountIdPtr& accoint_id) {
  return mojom::ZCashKeyId::New(accoint_id->account_index, 1 /* internal */, 0);
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace

void ZCashWalletService::Bind(
    mojo::PendingReceiver<mojom::ZCashWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

ZCashWalletService::ZCashWalletService(
    base::FilePath zcash_data_path,
    KeyringService* keyring_service,
    PrefService* prefs,
    NetworkManager* network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : zcash_data_path_(std::move(zcash_data_path)),
      keyring_service_(keyring_service) {
  zcash_rpc_ = std::make_unique<ZCashRpc>(network_manager, url_loader_factory);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
  complete_manager_ = std::make_unique<ZCashTransactionCompleteManager>(this);
#if BUILDFLAG(ENABLE_ORCHARD)
  background_orchard_storage_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      zcash_data_path_.AppendASCII(kOrchardDatabaseName));
#endif
}

ZCashWalletService::ZCashWalletService(base::FilePath zcash_data_path,
                                       KeyringService* keyring_service,
                                       std::unique_ptr<ZCashRpc> zcash_rpc)
    : zcash_data_path_(std::move(zcash_data_path)),
      keyring_service_(keyring_service) {
  CHECK_IS_TEST();
  zcash_rpc_ = std::move(zcash_rpc);
  if (keyring_service_) {
    keyring_service_->AddObserver(
        keyring_observer_receiver_.BindNewPipeAndPassRemote());
  }
  complete_manager_ = std::make_unique<ZCashTransactionCompleteManager>(this);
#if BUILDFLAG(ENABLE_ORCHARD)
  background_orchard_storage_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      zcash_data_path_.AppendASCII(kOrchardDatabaseName));
#endif
}

ZCashWalletService::~ZCashWalletService() = default;

void ZCashWalletService::GetBalance(const std::string& chain_id,
                                    mojom::AccountIdPtr account_id,
                                    GetBalanceCallback callback) {
  auto balance_task = std::make_unique<ZCashResolveBalanceTask>(
      this, chain_id, std::move(account_id),
      base::BindOnce(&ZCashWalletService::OnResolveBalanceResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  balance_task->ScheduleWorkOnTask();
  resolve_balance_tasks_.push_back(std::move(balance_task));
}

void ZCashWalletService::OnResolveBalanceResult(
    GetBalanceCallback callback,
    base::expected<mojom::ZCashBalancePtr, std::string> result) {
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

void ZCashWalletService::MakeAccountShielded(
    mojom::AccountIdPtr account_id,
    MakeAccountShieldedCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    // Only 1 account can be shieldable at the moment
    const auto& accounts = keyring_service_->GetAllAccountInfos();
    for (const auto& account : accounts) {
      if (IsZCashAccount(*(account->account_id)) &&
          !GetAccountShieldBirthday(account->account_id).is_null()) {
        std::move(callback).Run("Already has shieldable account");
        return;
      }
    }
    GetLatestBlockForAccountBirthday(account_id.Clone(), std::move(callback));
    return;
  }
#endif
  std::move(callback).Run("Not supported");
}

void ZCashWalletService::StartShieldSync(mojom::AccountIdPtr account_id,
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
            this, account_id, account_birthday, fvk.value(),
            weak_ptr_factory_.GetWeakPtr());

    shield_sync_services_[account_id.Clone()]->StartSyncing();

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
  auto task = base::WrapRefCounted<ZCashDiscoverNextUnusedZCashAddressTask>(
      new ZCashDiscoverNextUnusedZCashAddressTask(
          weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
          std::move(start_address), std::move(callback)));
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
        chain_id, address,
        base::BindOnce(&ZCashWalletService::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr(), context, address));
  }
}

void ZCashWalletService::CompleteTransactionDone(
    std::string chain_id,
    ZCashTransaction original_zcash_transaction,
    SignAndPostTransactionCallback callback,
    base::expected<ZCashTransaction, std::string> result) {
  if (!result.has_value()) {
    std::move(callback).Run("", std::move(original_zcash_transaction),
                            result.error());
    return;
  }

  auto tx = ZCashSerializer::SerializeRawTransaction(result.value());
  zcash_rpc_->SendTransaction(
      chain_id, tx,
      base::BindOnce(&ZCashWalletService::OnSendTransactionResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(result.value())));
}

void ZCashWalletService::SignAndPostTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const ZCashTransaction& zcash_transaction,
    SignAndPostTransactionCallback callback) {
  complete_manager_->CompleteTransaction(
      chain_id, zcash_transaction, account_id.Clone(),
      base::BindOnce(&ZCashWalletService::CompleteTransactionDone,
                     weak_ptr_factory_.GetWeakPtr(), chain_id,
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
#if BUILDFLAG(ENABLE_ORCHARD)
    if (IsZCashShieldedTransactionsEnabled() &&
        ExtractOrchardPart(addr, testnet)) {
      std::move(callback).Run(mojom::ZCashAddressValidationResult::Success);
      return;
    }
#endif
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
    std::move(callback).Run(
        "", std::move(tx), l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  }
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

void ZCashWalletService::CreateFullyTransparentTransaction(
    const std::string& chain_id,
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
      base::WrapUnique<ZCashCreateTransparentTransactionTask>(
          new ZCashCreateTransparentTransactionTask(this, chain_id, account_id,
                                                    final_address, amount,
                                                    std::move(callback))));
  task->ScheduleWorkOnTask();
}

#if BUILDFLAG(ENABLE_ORCHARD)
void ZCashWalletService::CreateShieldTransaction(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    std::optional<OrchardMemo> memo,
    CreateTransactionCallback callback) {
  CHECK(IsZCashShieldedTransactionsEnabled());

  auto receiver_addr =
      GetOrchardRawBytes(address_to, chain_id == mojom::kZCashTestnet);
  if (!receiver_addr) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto shield_funds_task = base::WrapUnique<ZCashCreateShieldTransactionTask>(
      new ZCashCreateShieldTransactionTask(this, chain_id, account_id,
                                           *receiver_addr, std::move(memo),
                                           amount, std::move(callback)));
  shield_funds_task->ScheduleWorkOnTask();
  create_shield_transaction_tasks_.push_back(std::move(shield_funds_task));
}
#endif

void ZCashWalletService::ShieldAllFunds(const std::string& chain_id,
                                        mojom::AccountIdPtr account_id,
                                        ShieldAllFundsCallback callback) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    CreateShieldAllTransaction(
        chain_id, account_id.Clone(),
        base::BindOnce(&ZCashWalletService::CreateShieldAllTransactionTaskDone,
                       weak_ptr_factory_.GetWeakPtr(), chain_id,
                       account_id.Clone(), std::move(callback)));
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

#if BUILDFLAG(ENABLE_ORCHARD)

void ZCashWalletService::CreateShieldAllTransaction(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    CreateTransactionCallback callback) {
  CHECK(IsZCashShieldedTransactionsEnabled());

  auto internal_addr = keyring_service_->GetOrchardRawBytes(
      account_id, CreateOrchardInternalKeyId(account_id));

  auto shield_funds_task = base::WrapUnique<ZCashCreateShieldTransactionTask>(
      new ZCashCreateShieldTransactionTask(
          this, chain_id, account_id, *internal_addr, std::nullopt,
          kZCashFullAmount, std::move(callback)));
  shield_funds_task->ScheduleWorkOnTask();
  create_shield_transaction_tasks_.push_back(std::move(shield_funds_task));
}

void ZCashWalletService::CreateShieldAllTransactionTaskDone(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    ShieldAllFundsCallback callback,
    base::expected<ZCashTransaction, std::string> transaction) {
  if (!transaction.has_value()) {
    std::move(callback).Run(
        std::nullopt, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  SignAndPostTransaction(
      chain_id, account_id.Clone(), std::move(transaction.value()),
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

  // Get block info for the block that is back from latest block for
  // kChainReorgBlockDelta to ensure account birthday won't be affected by chain
  // reorg.
  if ((*result)->height < kChainReorgBlockDelta) {
    std::move(callback).Run("Failed to retrieve latest block");
    return;
  }

  auto block_id = zcash::mojom::BlockID::New(
      (*result)->height - kChainReorgBlockDelta, std::vector<uint8_t>());

  zcash_rpc_->GetTreeState(
      GetNetworkForZCashKeyring(account_id->keyring_id), std::move(block_id),
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
    base::expected<zcash::mojom::RawTransactionPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    std::move(callback).Run(base::unexpected(result.error()));
    return;
  }
  std::move(callback).Run((*result)->height > 0);
}

void ZCashWalletService::CreateTransactionTaskDone(
    ZCashCreateTransparentTransactionTask* task) {
  CHECK(create_transaction_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}

#if BUILDFLAG(ENABLE_ORCHARD)
void ZCashWalletService::CreateTransactionTaskDone(
    ZCashCreateShieldTransactionTask* task) {
  CHECK(create_shield_transaction_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashWalletService::ResolveBalanceTaskDone(ZCashResolveBalanceTask* task) {
  CHECK(resolve_balance_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}

ZCashRpc* ZCashWalletService::zcash_rpc() {
  return zcash_rpc_.get();
}

KeyringService* ZCashWalletService::keyring_service() {
  return keyring_service_.get();
}

#if BUILDFLAG(ENABLE_ORCHARD)
base::SequenceBound<ZCashOrchardStorage>&
ZCashWalletService::orchard_storage() {
  return background_orchard_storage_;
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashWalletService::Unlocked() {
  auto accounts = keyring_service_->GetAllAccountsSync();
  for (const auto& account : accounts->accounts) {
    if (IsZCashKeyring(account->account_id->keyring_id)) {
      RunDiscovery(account->account_id.Clone(), base::DoNothing());
    }
  }
}

void ZCashWalletService::Locked() {
#if BUILDFLAG(ENABLE_ORCHARD)
  shield_sync_services_.clear();
#endif  // BUILDFLAG(ENABLE_ORCHARD)
}

void ZCashWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
#if BUILDFLAG(ENABLE_ORCHARD)
  shield_sync_services_.clear();
  background_orchard_storage_.AsyncCall(&ZCashOrchardStorage::ResetDatabase);
#endif  // BUILDFLAG(ENABLE_ORCHARD)
}

}  // namespace brave_wallet
