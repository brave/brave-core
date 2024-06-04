/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service_tasks.h"

#include <algorithm>
#include <utility>

#include "base/check_is_test.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

const uint32_t kDefaultTransparentOutputsCount = 2;
const uint32_t kGraceActionsCount = 2;
const uint64_t kMarginalFee = 5000;

bool OutputAddressSupported(const std::string& address, bool is_testnet) {
  auto decoded_address = DecodeZCashAddress(address);
  if (!decoded_address) {
    return false;
  }
  if (decoded_address->testnet != is_testnet) {
    return false;
  }

  return true;
}

// https://zips.z.cash/zip-0317
uint64_t CalculateTxFee(const uint32_t tx_input_count,
                        const uint32_t orchard_actions_count) {
  // Use simplified calcultion fee form since we don't support p2psh
  // and shielded addresses
  auto actions_count = std::max(tx_input_count + orchard_actions_count,
                                kDefaultTransparentOutputsCount);
  return kMarginalFee * std::max(kGraceActionsCount, actions_count);
}

}  // namespace

// GetTransparentUtxosContext
GetTransparentUtxosContext::GetTransparentUtxosContext() = default;
GetTransparentUtxosContext::~GetTransparentUtxosContext() = default;

// DiscoverNextUnusedZCashAddressTask
DiscoverNextUnusedZCashAddressTask::DiscoverNextUnusedZCashAddressTask(
    base::WeakPtr<ZCashWalletService> zcash_wallet_service,
    mojom::AccountIdPtr account_id,
    mojom::ZCashAddressPtr start_address,
    ZCashWalletService::DiscoverNextUnusedAddressCallback callback)
    : zcash_wallet_service_(std::move(zcash_wallet_service)),
      account_id_(std::move(account_id)),
      start_address_(std::move(start_address)),
      callback_(std::move(callback)) {}

DiscoverNextUnusedZCashAddressTask::~DiscoverNextUnusedZCashAddressTask() =
    default;

void DiscoverNextUnusedZCashAddressTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&DiscoverNextUnusedZCashAddressTask::WorkOnTask, this));
}

mojom::ZCashAddressPtr DiscoverNextUnusedZCashAddressTask::GetNextAddress(
    const mojom::ZCashAddressPtr& address) {
  auto* keyring_service = zcash_wallet_service_->keyring_service_.get();
  CHECK(keyring_service);
  auto next_key_id = current_address_->key_id.Clone();

  next_key_id->index++;

  return keyring_service->GetZCashAddress(*account_id_, *next_key_id);
}

void DiscoverNextUnusedZCashAddressTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (!zcash_wallet_service_) {
    std::move(callback_).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(base::ok(std::move(result_)));
    return;
  }

  if (!block_end_) {
    zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
        GetNetworkForZCashKeyring(account_id_->keyring_id),
        base::BindOnce(&DiscoverNextUnusedZCashAddressTask::OnGetLastBlock,
                       this));
    return;
  }

  if (start_address_) {
    current_address_ = std::move(start_address_);
  } else {
    current_address_ = GetNextAddress(current_address_);
  }

  if (!current_address_) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  zcash_wallet_service_->zcash_rpc()->IsKnownAddress(
      GetNetworkForZCashKeyring(account_id_->keyring_id),
      current_address_->address_string, 1, block_end_.value(),
      base::BindOnce(&DiscoverNextUnusedZCashAddressTask::OnGetIsKnownAddress,
                     this));
}

void DiscoverNextUnusedZCashAddressTask::OnGetLastBlock(
    base::expected<mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    error_ = result.error();
    WorkOnTask();
    return;
  }

  block_end_ = (*result)->height;
  WorkOnTask();
}

void DiscoverNextUnusedZCashAddressTask::OnGetIsKnownAddress(
    base::expected<bool, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    WorkOnTask();
    return;
  }

  if (!result.value()) {
    result_ = current_address_->Clone();
  }

  WorkOnTask();
}

// CreateTransparentTransactionTask
CreateTransparentTransactionTask::CreateTransparentTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      callback_(std::move(callback)) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
}

CreateTransparentTransactionTask::~CreateTransparentTransactionTask() = default;

void CreateTransparentTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateTransparentTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateTransparentTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    zcash_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  if (!chain_height_) {
    zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
        chain_id_,
        base::BindOnce(&CreateTransparentTransactionTask::OnGetChainHeight,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!change_address_) {
    zcash_wallet_service_->DiscoverNextUnusedAddress(
        account_id_, true,
        base::BindOnce(&CreateTransparentTransactionTask::OnGetChangeAddress,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (utxo_map_.empty()) {
    zcash_wallet_service_->GetUtxos(
        chain_id_, account_id_.Clone(),
        base::BindOnce(&CreateTransparentTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(cypt4): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
  transaction_.set_locktime(chain_height_.value());

  if (!PickInputs()) {
    // TODO(cypt4) : switch to IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE when ready
    SetError(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    ScheduleWorkOnTask();
    return;
  }

  if (!PrepareOutputs()) {
    SetError(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    ScheduleWorkOnTask();
    return;
  }

  DCHECK_EQ(kDefaultTransparentOutputsCount,
            transaction_.transparent_part().outputs.size());

  std::move(callback_).Run(base::ok(std::move(transaction_)));
  zcash_wallet_service_->CreateTransactionTaskDone(this);
}

void CreateTransparentTransactionTask::OnGetChainHeight(
    base::expected<mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  chain_height_ = (*result)->height;
  WorkOnTask();
}

void CreateTransparentTransactionTask::OnGetChangeAddress(
    base::expected<mojom::ZCashAddressPtr, std::string> result) {
  if (!result.has_value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  change_address_ = std::move(result.value());
  WorkOnTask();
}

void CreateTransparentTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    SetError(std::move(utxo_map).error());
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxo_map.value());
  WorkOnTask();
}

bool CreateTransparentTransactionTask::PickInputs() {
  bool done = false;

  // TODO(apaymyshev): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus fee. Needs something better than such
  // greedy strategy.
  std::vector<ZCashTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map_) {
    for (const auto& utxo : item.second) {
      if (!utxo) {
        error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
        return false;
      }
      if (auto input =
              ZCashTransaction::TxInput::FromRpcUtxo(item.first, *utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  base::ranges::sort(all_inputs, [](auto& input1, auto& input2) {
    return input1.utxo_value < input2.utxo_value;
  });

  for (auto& input : all_inputs) {
    transaction_.transparent_part().inputs.push_back(std::move(input));
    transaction_.set_fee(
        CalculateTxFee(transaction_.transparent_part().inputs.size(), 0));

    if (transaction_.TotalInputsAmount() >=
        transaction_.amount() + transaction_.fee()) {
      done = true;
    }

    if (done) {
      break;
    }
  }

  DCHECK(!transaction_.transparent_part().inputs.empty());
  return done;
}

bool CreateTransparentTransactionTask::PrepareOutputs() {
  auto& target_output = transaction_.transparent_part().outputs.emplace_back();
  target_output.address = transaction_.to();
  if (!OutputAddressSupported(target_output.address, IsTestnet())) {
    return false;
  }

  target_output.amount = transaction_.amount();
  target_output.script_pubkey =
      ZCashAddressToScriptPubkey(target_output.address, IsTestnet());

  CHECK_GE(transaction_.TotalInputsAmount(),
           transaction_.amount() + transaction_.fee());
  uint64_t change_amount = transaction_.TotalInputsAmount() -
                           transaction_.amount() - transaction_.fee();
  if (change_amount == 0) {
    return true;
  }

  const auto& change_address = change_address_;
  if (!change_address) {
    return false;
  }

  CHECK(OutputAddressSupported(change_address->address_string, IsTestnet()));
  auto& change_output = transaction_.transparent_part().outputs.emplace_back();
  change_output.address = change_address_->address_string;
  change_output.amount = change_amount;
  change_output.script_pubkey =
      ZCashAddressToScriptPubkey(change_output.address, IsTestnet());
  return true;
}

#if BUILDFLAG(ENABLE_ORCHARD)

CreateShieldAllTransactionTask::CreateShieldAllTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    ZCashWalletService::CreateTransactionCallback callback,
    std::optional<uint64_t> random_seed_for_testing)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      callback_(std::move(callback)),
      random_seed_for_testing_(random_seed_for_testing) {}

CreateShieldAllTransactionTask::~CreateShieldAllTransactionTask() = default;

::rust::Box<orchard::OrchardUnauthorizedBundleResult>
CreateShieldAllTransactionTask::CreateOrchardUnauthorizedBundle(
    ::rust::Slice<const uint8_t> tree_state,
    ::rust::Vec<orchard::OrchardOutput> outputs) {
  if (random_seed_for_testing_) {
    CHECK_IS_TEST();
    return create_testing_orchard_bundle(std::move(tree_state),
                                         std::move(outputs),
                                         random_seed_for_testing_.value());
  } else {
    return create_orchard_bundle(std::move(tree_state), std::move(outputs));
  }
}

void CreateShieldAllTransactionTask::ScheduleWorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  if (!tree_state_) {
    GetTreeState();
    return;
  }

  if (!utxo_map_) {
    GetAllUtxos();
    return;
  }

  if (!chain_height_) {
    GetChainHeight();
    return;
  }

  if (!CreateTransaction()) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  // TODO(cypt4): This call should be async
  if (!CompleteTransaction()) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  std::move(callback_).Run(std::move(transaction_.value()));
}

bool CreateShieldAllTransactionTask::CreateTransaction() {
  CHECK(utxo_map_);
  CHECK(chain_height_);

  ZCashTransaction zcash_transaction;

  // Pick inputs
  std::vector<ZCashTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map_.value()) {
    for (const auto& utxo : item.second) {
      if (!utxo) {
        error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
        return false;
      }
      if (auto input =
              ZCashTransaction::TxInput::FromRpcUtxo(item.first, *utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  zcash_transaction.transparent_part().inputs = std::move(all_inputs);
  // TODO(cypt4): Calculate orchard actions count
  zcash_transaction.set_fee(CalculateTxFee(
      zcash_transaction.transparent_part().inputs.size(),
      2 /* actions count for 1 orchard output no orchard inputs */));

  // Pick orchard outputs
  ZCashTransaction::OrchardOutput orchard_output;
  auto addr_bytes =
      zcash_wallet_service_->keyring_service()->GetOrchardRawBytes(
          account_id_, mojom::ZCashKeyId::New(account_id_->account_index,
                                              1 /* internal */, 0));
  if (!addr_bytes) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  if (zcash_transaction.fee() > zcash_transaction.TotalInputsAmount()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  orchard_output.value =
      zcash_transaction.TotalInputsAmount() - zcash_transaction.fee();
  orchard_output.address = std::move(addr_bytes.value());

  zcash_transaction.orchard_part().outputs.push_back(std::move(orchard_output));

  zcash_transaction.set_locktime(chain_height_.value());
  zcash_transaction.set_expiry_height(chain_height_.value() +
                                      kDefaultZCashBlockHeightDelta);

  transaction_ = std::move(zcash_transaction);

  return true;
}

bool CreateShieldAllTransactionTask::CompleteTransaction() {
  CHECK(transaction_);
  CHECK_EQ(1u, transaction_->orchard_part().outputs.size());
  CHECK(tree_state_);

  // Sign shielded part
  auto state_tree_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", tree_state_.value()->orchardTree}));
  if (!state_tree_bytes) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  ::rust::Vec<orchard::OrchardOutput> outputs;
  for (const auto& output : transaction_->orchard_part().outputs) {
    outputs.push_back(orchard::OrchardOutput{output.value, output.address});
  }

  auto unauthorized_orchard_bundle = CreateOrchardUnauthorizedBundle(
      ::rust::Slice<const uint8_t>{state_tree_bytes->data(),
                                   state_tree_bytes->size()},
      std::move(outputs));

  if (!unauthorized_orchard_bundle->is_ok()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  transaction_->orchard_part().digest =
      unauthorized_orchard_bundle->unwrap()->orchard_digest();

  // Calculate Orchard sighash
  auto sighash = ZCashSerializer::CalculateSignatureDigest(transaction_.value(),
                                                           std::nullopt);

  {
    // TODO(cypt4) : Move on background process
    auto complete_orchard_bundle =
        unauthorized_orchard_bundle->unwrap()->complete(sighash);

    if (!complete_orchard_bundle->is_ok()) {
      error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
      return false;
    }

    auto orchard_raw_part = complete_orchard_bundle->unwrap()->raw_tx();

    transaction_->orchard_part().raw_tx =
        std::vector<uint8_t>(orchard_raw_part.begin(), orchard_raw_part.end());
  }

  // Sign transparent part
  if (!zcash_wallet_service_->SignTransactionInternal(transaction_.value(),
                                                      account_id_)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  return true;
}

void CreateShieldAllTransactionTask::GetAllUtxos() {
  zcash_wallet_service_->GetUtxos(
      chain_id_, account_id_.Clone(),
      base::BindOnce(&CreateShieldAllTransactionTask::OnGetUtxos,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::GetTreeState() {
  zcash_wallet_service_->zcash_rpc()->GetLatestTreeState(
      chain_id_, base::BindOnce(&CreateShieldAllTransactionTask::OnGetTreeState,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::GetChainHeight() {
  zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
      chain_id_,
      base::BindOnce(&CreateShieldAllTransactionTask::OnGetChainHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::OnGetUtxos(
    base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    error_ = utxo_map.error();
  } else {
    utxo_map_ = std::move(*utxo_map);
  }

  ScheduleWorkOnTask();
}

void CreateShieldAllTransactionTask::OnGetTreeState(
    base::expected<mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value()) {
    error_ = tree_state.error();
  } else {
    tree_state_ = std::move(*tree_state);
  }

  ScheduleWorkOnTask();
}

void CreateShieldAllTransactionTask::OnGetChainHeight(
    base::expected<mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    error_ = result.error();
  } else {
    chain_height_ = (*result)->height;
  }

  ScheduleWorkOnTask();
}
#endif  // BUILDFLAG(ENABLE_ORCHARD_CRATE)

}  // namespace brave_wallet
