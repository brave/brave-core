/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_discover_account_task.h"

#include <stdint.h>

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_import_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_task_utils.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

const uint32_t kAddressDiscoveryGapLimit = 20;

std::string InternalErrorString() {
  return l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
}

std::string ParsingErrorString() {
  return l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR);
}

}  // namespace

DiscoverAccountTaskBase::State::State() = default;
DiscoverAccountTaskBase::State::~State() = default;

DiscoveredBitcoinAccount::DiscoveredBitcoinAccount() = default;
DiscoveredBitcoinAccount::~DiscoveredBitcoinAccount() = default;
DiscoveredBitcoinAccount::DiscoveredBitcoinAccount(
    DiscoveredBitcoinAccount&& other) = default;

bool DiscoveredBitcoinAccount::operator==(
    const DiscoveredBitcoinAccount& other) const {
  return std::tie(this->balance, this->next_unused_receive_index,
                  this->next_unused_change_index) ==
         std::tie(other.balance, other.next_unused_receive_index,
                  other.next_unused_change_index);
}

DiscoverAccountTaskBase::DiscoverAccountTaskBase(
    BitcoinWalletService& bitcoin_wallet_service,
    const std::string& network_id)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      network_id_(network_id),
      balance_(mojom::BitcoinBalance::New()) {
  CHECK(IsBitcoinNetwork(network_id));
}
DiscoverAccountTaskBase::~DiscoverAccountTaskBase() = default;

DiscoverAccountTaskBase::State& DiscoverAccountTaskBase::GetState(
    bool receive_state) {
  return receive_state ? receive_addresses_state_ : change_addresses_state_;
}

void DiscoverAccountTaskBase::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&DiscoverAccountTaskBase::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

bool DiscoverAccountTaskBase::MaybeQueueRequests(bool receive_state) {
  auto& state = GetState(receive_state);

  uint32_t start_index = 0;

  // Start with next non-transacted address.
  if (state.last_transacted_address) {
    start_index = state.last_transacted_address->key_id->index + 1;
  }

  for (auto address_index = start_index;
       address_index < start_index + kAddressDiscoveryGapLimit;
       ++address_index) {
    // Skip request if already sent.
    if (state.last_requested_address &&
        state.last_requested_address->key_id->index >= address_index) {
      continue;
    }

    auto address = GetAddressById(mojom::BitcoinKeyId::New(
        receive_state ? kBitcoinReceiveIndex : kBitcoinChangeIndex,
        address_index));
    if (!address) {
      return false;
    }

    active_requests_++;
    state.last_requested_address = address->Clone();
    bitcoin_wallet_service_->bitcoin_rpc().GetAddressStats(
        network_id_, address->address_string,
        base::BindOnce(&DiscoverAccountTaskBase::OnGetAddressStats,
                       weak_ptr_factory_.GetWeakPtr(), receive_state,
                       address->Clone()));
  }

  return true;
}

void DiscoverAccountTaskBase::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  bool queue_requests_failed = false;
  if (!MaybeQueueRequests(true)) {
    queue_requests_failed = true;
  }
  if (account_is_used_) {
    if (!MaybeQueueRequests(false)) {
      queue_requests_failed = true;
    }
  }
  if (queue_requests_failed) {
    error_ = InternalErrorString();
    ScheduleWorkOnTask();
    return;
  }

  if (active_requests_) {
    return;
  }

  DiscoveredBitcoinAccount result;

  result.next_unused_receive_index = 0;
  if (receive_addresses_state_.last_transacted_address) {
    result.next_unused_receive_index =
        1 + receive_addresses_state_.last_transacted_address->key_id->index;
  }

  result.next_unused_change_index = 0;
  if (change_addresses_state_.last_transacted_address) {
    result.next_unused_change_index =
        1 + change_addresses_state_.last_transacted_address->key_id->index;
  }

  result.balance = std::move(balance_);

  std::move(callback_).Run(base::ok(std::move(result)));
}

void DiscoverAccountTaskBase::OnGetAddressStats(
    bool receive_state,
    mojom::BitcoinAddressPtr address,
    base::expected<bitcoin_rpc::AddressStats, std::string> stats) {
  DCHECK_GT(active_requests_, 0u);
  active_requests_--;

  if (!stats.has_value()) {
    error_ = stats.error();
    WorkOnTask();
    return;
  }

  UpdateBalance(balance_, *stats);

  uint32_t chain_stats_tx_count = 0;
  uint32_t mempool_stats_tx_count = 0;
  if (!base::StringToUint(stats->chain_stats.tx_count, &chain_stats_tx_count) ||
      !base::StringToUint(stats->mempool_stats.tx_count,
                          &mempool_stats_tx_count)) {
    error_ = ParsingErrorString();
    WorkOnTask();
    return;
  }
  auto address_is_transacted = chain_stats_tx_count || mempool_stats_tx_count;
  if (address_is_transacted) {
    account_is_used_ = true;
  }

  CHECK_EQ(address->key_id->change, !receive_state);
  auto& state = GetState(receive_state);

  if (address_is_transacted) {
    if (!state.last_transacted_address ||
        state.last_transacted_address->key_id->index < address->key_id->index) {
      state.last_transacted_address = address->Clone();
    }
  }

  WorkOnTask();
}

DiscoverWalletAccountTask::DiscoverWalletAccountTask(
    BitcoinWalletService& bitcoin_wallet_service,
    mojom::KeyringId keyring_id,
    uint32_t account_index)
    : DiscoverAccountTaskBase(bitcoin_wallet_service,
                              GetNetworkForBitcoinKeyring(keyring_id)),
      keyring_id_(keyring_id),
      account_index_(account_index) {
  CHECK(IsBitcoinKeyring(keyring_id_));
}
DiscoverWalletAccountTask::~DiscoverWalletAccountTask() = default;

mojom::BitcoinAddressPtr DiscoverWalletAccountTask::GetAddressById(
    const mojom::BitcoinKeyIdPtr& key_id) {
  return bitcoin_wallet_service()
      .keyring_service()
      .GetBitcoinAccountDiscoveryAddress(keyring_id_, account_index_, key_id);
}

DiscoverExtendedKeyAccountTask::DiscoverExtendedKeyAccountTask(
    BitcoinWalletService& bitcoin_wallet_service,
    const std::string& network_id,
    const std::string& extended_key)
    : DiscoverAccountTaskBase(bitcoin_wallet_service, network_id),
      testnet_(network_id == mojom::kBitcoinTestnet) {
  CHECK(IsBitcoinNetwork(network_id));

  auto parsed_key = HDKey::GenerateFromExtendedKey(extended_key);
  if (!parsed_key) {
    return;
  }

  if (testnet_ && parsed_key->version != ExtendedKeyVersion::kTpub) {
    return;
  }
  if (!testnet_ && parsed_key->version != ExtendedKeyVersion::kXpub) {
    return;
  }
  account_key_ = std::move(parsed_key->hdkey);
}
DiscoverExtendedKeyAccountTask::~DiscoverExtendedKeyAccountTask() = default;

mojom::BitcoinAddressPtr DiscoverExtendedKeyAccountTask::GetAddressById(
    const mojom::BitcoinKeyIdPtr& key_id) {
  if (!account_key_) {
    return nullptr;
  }

  auto key = account_key_->DeriveNormalChild(key_id->change);
  if (!key) {
    return nullptr;
  }

  key = key->DeriveNormalChild(key_id->index);
  if (!key) {
    return nullptr;
  }

  return mojom::BitcoinAddress::New(
      PubkeyToSegwitAddress(key->GetPublicKeyBytes(), testnet_),
      key_id.Clone());
}

}  // namespace brave_wallet
