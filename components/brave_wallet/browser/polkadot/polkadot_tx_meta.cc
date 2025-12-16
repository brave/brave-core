/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

PolkadotTxMeta::PolkadotTxMeta(const mojom::AccountIdPtr& from,
                               const PolkadotChainMetadata& chain_metadata,
                               const PolkadotUnsignedTransfer& extrinsic)
    : recipient_(base::HexEncodeLower(extrinsic.recipient())),
      encoded_extrinsic_(extrinsic.Encode(chain_metadata)) {
  set_from(from.Clone());

  // The only way this code winds up dying is if somehow we're trying to save an
  // invalid transaction. Currently the front-end only permits transferring
  // uint64s so if we hit this codepath it means there's a bug either in our
  // front-end interactions or we're attempting to save an invalid transaction
  // and we have a bug.
  amount_ = base::checked_cast<uint64_t>(extrinsic.send_amount());
}

PolkadotTxMeta::~PolkadotTxMeta() = default;

base::Value::Dict PolkadotTxMeta::ToValue() const {
  auto dict = TxMeta::ToValue();
  dict.Set("extrinsic", encoded_extrinsic_);
  return dict;
}

mojom::TransactionInfoPtr PolkadotTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewPolkadotTxData(
          mojom::PolkadotTxdata::New(recipient_, amount_, false, 0)),
      status_, mojom::TransactionType::Other,
      std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_, "",
      false, nullptr);
}

mojom::CoinType PolkadotTxMeta::GetCoinType() const {
  return mojom::CoinType::DOT;
}

}  // namespace brave_wallet
