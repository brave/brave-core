// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_to_ironwood_transaction_task.h"

#include <variant>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashCreateTransparentToIronwoodTransactionTask::
    ZCashCreateTransparentToIronwoodTransactionTask(
        std::variant<
            base::PassKey<
                class ZCashCreateTransparentToIronwoodTransactionTaskTest>,
            base::PassKey<ZCashWalletService>> pass_key,
        ZCashWalletService& zcash_wallet_service,
        ZCashActionContext context,
        const OrchardAddrRawPart& receiver,
        std::optional<OrchardMemo> memo,
        uint64_t amount)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      receiver_(receiver),
      memo_(memo),
      amount_(amount) {}

ZCashCreateTransparentToIronwoodTransactionTask::
    ~ZCashCreateTransparentToIronwoodTransactionTask() = default;

void ZCashCreateTransparentToIronwoodTransactionTask::Start(
    CreateTransactionCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCreateTransparentToIronwoodTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ZCashCreateTransparentToIronwoodTransactionTask::WorkOnTask,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToIronwoodTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  if (!utxo_map_) {
    GetAllUtxos();
    return;
  }

  if (!chain_tip_height_) {
    GetLatestBlock();
    return;
  }

  if (amount_ != kZCashFullAmount && !change_address_) {
    GetChangeAddress();
    return;
  }

  if (!transaction_) {
    CreateTransaction();
    return;
  }

  std::move(callback_).Run(std::move(transaction_.value()));
}

void ZCashCreateTransparentToIronwoodTransactionTask::CreateTransaction() {
  CHECK(utxo_map_);
  ZCashTransaction zcash_transaction;
  zcash_transaction.ConvertToV6();

  auto pick_transparent_inputs_result = PickZCashTransparentInputs(
      *utxo_map_, amount_, ZCashTargetOutputType::kOrchard);
  if (!pick_transparent_inputs_result) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  zcash_transaction.transparent_part().inputs =
      pick_transparent_inputs_result->inputs;
  zcash_transaction.set_fee(pick_transparent_inputs_result->fee);

  if (pick_transparent_inputs_result->change != 0) {
    CHECK_NE(amount_, kZCashFullAmount);
    auto& change_output =
        zcash_transaction.transparent_part().outputs.emplace_back();
    change_output.address = change_address_->address_string;
    change_output.amount = pick_transparent_inputs_result->change;
    change_output.script_pubkey =
        ZCashAddressToScriptPubkey(
            change_output.address,
            IsZCashTestnetKeyring(context_.account_id->keyring_id))
            .value();
  }

  OrchardOutput& orchard_output =
      zcash_transaction.v6_part().ironwood.outputs.emplace_back();
  auto value = base::CheckSub<uint64_t>(zcash_transaction.TotalInputsAmount(),
                                        pick_transparent_inputs_result->fee,
                                        pick_transparent_inputs_result->change);
  if (!value.AssignIfValid(&orchard_output.value)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  orchard_output.addr = receiver_;
  orchard_output.memo = memo_;
  zcash_transaction.v6_part().ironwood.anchor_block_height =
      chain_tip_height_.value();

  auto orchard_unified_addr = GetOrchardUnifiedAddress(
      receiver_, IsZCashTestnetKeyring(context_.account_id->keyring_id));
  if (!orchard_unified_addr) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  zcash_transaction.set_amount(orchard_output.value);
  zcash_transaction.set_to(*orchard_unified_addr);
  zcash_transaction.set_memo(memo_);

  transaction_ = std::move(zcash_transaction);
  ScheduleWorkOnTask();
}

void ZCashCreateTransparentToIronwoodTransactionTask::GetAllUtxos() {
  zcash_wallet_service_->GetUtxos(
      context_.account_id.Clone(),
      base::BindOnce(
          &ZCashCreateTransparentToIronwoodTransactionTask::OnGetUtxos,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToIronwoodTransactionTask::GetLatestBlock() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(&ZCashCreateTransparentToIronwoodTransactionTask::
                         OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToIronwoodTransactionTask::OnGetLatestBlockHeight(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_height_ = result.value()->height;
  ScheduleWorkOnTask();
}

void ZCashCreateTransparentToIronwoodTransactionTask::GetChangeAddress() {
  zcash_wallet_service_->DiscoverNextUnusedAddress(
      context_.account_id.Clone(), true,
      base::BindOnce(
          &ZCashCreateTransparentToIronwoodTransactionTask::OnGetChangeAddress,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToIronwoodTransactionTask::OnGetChangeAddress(
    base::expected<mojom::ZCashAddressPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }

  change_address_ = std::move(result.value());
  ScheduleWorkOnTask();
}

void ZCashCreateTransparentToIronwoodTransactionTask::OnGetUtxos(
    base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    error_ = utxo_map.error();
  } else {
    utxo_map_ = std::move(*utxo_map);
  }

  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
