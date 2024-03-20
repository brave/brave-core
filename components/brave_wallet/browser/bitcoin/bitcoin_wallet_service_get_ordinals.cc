/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service_get_ordinals.h"

#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/rand_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/bitcoin_ordinals_rpc_responses.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

GetOrdinalsTask::GetOrdinalsTask(
    base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
    const std::string& chain_id,
    const std::vector<BitcoinOutpoint>& outpoints,
    GetOrdinalsCallback callback)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      chain_id_(chain_id),
      pending_outpoints_(outpoints),
      callback_(std::move(callback)) {}

GetOrdinalsTask::~GetOrdinalsTask() = default;

void GetOrdinalsTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetOrdinalsTask::WorkOnTask, this));
}

void GetOrdinalsTask::MaybeSendRequests() {
  if (requests_sent_) {
    return;
  }
  requests_sent_ = true;

  // Shuffle outpoints so requests are always done in different order to
  // increase privacy a bit.
  base::RandomShuffle(pending_outpoints_.begin(), pending_outpoints_.end());

  std::vector<BitcoinOutpoint> requested_outpoints;
  for (const auto& outpoint : pending_outpoints_) {
    // Don't request ordinals info if it's already in cache.
    if (bitcoin_wallet_service_ &&
        bitcoin_wallet_service_->ordinals_cache().contains(outpoint)) {
      ordinals_[outpoint] =
          bitcoin_wallet_service_->ordinals_cache().at(outpoint).Clone();
      continue;
    }

    requested_outpoints.push_back(outpoint);
    bitcoin_wallet_service_->bitcoin_ordinals_rpc().GetOutpointInfo(
        chain_id_, outpoint,
        base::BindOnce(&GetOrdinalsTask::OnGetOutpointInfo, this, outpoint));
  }
  pending_outpoints_ = std::move(requested_outpoints);

  if (pending_outpoints_.empty()) {
    result_ = OrdinalsMap();
    ScheduleWorkOnTask();
    return;
  }
}

void GetOrdinalsTask::OnGetOutpointInfo(
    BitcoinOutpoint outpoint,
    base::expected<bitcoin_ordinals_rpc::OutpointInfo, std::string>
        output_info) {
  if (output_info.has_value()) {
    ordinals_[outpoint] = std::move(output_info.value());

    if (bitcoin_wallet_service_) {
      bitcoin_wallet_service_->ordinals_cache()[outpoint] =
          ordinals_[outpoint].Clone();
    }
  } else {
    // Don't fail fetching whole set of ordinals.
    // Some of outpoint might just get 'unknown' ordinals state and produce a
    // warning.
  }

  CHECK(std::erase(pending_outpoints_, outpoint));
  if (pending_outpoints_.empty()) {
    result_ = std::move(ordinals_);
  }

  WorkOnTask();
}

void GetOrdinalsTask::WorkOnTask() {
  if (!callback_) {
    return;
  }
  if (!bitcoin_wallet_service_) {
    std::move(callback_).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(base::ok(std::move(*result_)));
    return;
  }

  MaybeSendRequests();
}

}  // namespace brave_wallet
