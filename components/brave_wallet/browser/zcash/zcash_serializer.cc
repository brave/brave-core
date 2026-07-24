/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <array>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

namespace brave_wallet {

// static
void ZCashSerializer::SerializeSignature(
    const ZCashTransaction& tx,
    ZCashTransaction::TxInput& input,
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& signature) {
  BtcLikeSerializerStream stream;
  stream.PushCompactSize(signature.size() + 1);
  stream.PushBytes(signature);
  stream.Push8(tx.sighash_type());
  stream.PushSizeAndBytes(pubkey);
  input.script_sig = std::move(stream).Take();
}

// static
std::array<uint8_t, kZCashDigestSize> ZCashSerializer::CalculateTxIdDigest(
    const ZCashTransaction& zcash_transaction) {
  return ZCashV5Serializer::CalculateTxIdDigest(zcash_transaction);
}

// static
std::vector<uint8_t> ZCashSerializer::SerializeRawTransaction(
    const ZCashTransaction& zcash_transaction) {
  return ZCashV5Serializer::SerializeRawTransaction(zcash_transaction);
}

// static
bool ZCashSerializer::SignTransparentPart(KeyringService& keyring_service,
                                          const mojom::AccountIdPtr& account_id,
                                          ZCashTransaction& tx) {
  return ZCashV5Serializer::SignTransparentPartV5(keyring_service, account_id,
                                                  tx);
}

}  // namespace brave_wallet
