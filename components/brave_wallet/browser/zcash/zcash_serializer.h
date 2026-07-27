/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_

#include <array>
#include <vector>

#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

// Dispatch entry points for ZCash transaction serialization.
// Version-specific work is handled by ZCashV5Serializer; shared ZIP-244
// primitives live in ZCashSerializerUtils.
class ZCashSerializer {
 public:
  static std::array<uint8_t, kZCashDigestSize> CalculateTxIdDigest(
      const ZCashTransaction& zcash_transaction);
  static std::vector<uint8_t> SerializeRawTransaction(
      const ZCashTransaction& zcash_transaction);
  static bool SignTransparentPart(KeyringService& keyring_service,
                                  const mojom::AccountIdPtr& account_id,
                                  ZCashTransaction& tx);

  static void SerializeSignature(const ZCashTransaction& tx,
                                 ZCashTransaction::TxInput& input,
                                 const std::vector<uint8_t>& pubkey,
                                 const std::vector<uint8_t>& signature);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
