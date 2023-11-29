/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

namespace brave_wallet {

// https://zips.z.cash/zip-0244
class ZCashSerializer {
 public:
  static std::array<uint8_t, 32> CalculateTxIdDigest(
      const ZCashTransaction& zcash_transaction);
  static std::array<uint8_t, 32> CalculateSignatureDigest(
      const ZCashTransaction& zcash_transaction,
      const ZCashTransaction::TxInput& input);
  static std::vector<uint8_t> SerializeRawTransaction(
      const ZCashTransaction& zcash_transaction);

 private:
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashHeader);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashOutputs);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashPrevouts);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashSequences);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashTxIn);

  static std::array<uint8_t, 32> HashHeader(const ZCashTransaction& tx);
  static std::array<uint8_t, 32> HashPrevouts(const ZCashTransaction& tx);
  static std::array<uint8_t, 32> HashSequences(const ZCashTransaction& tx);
  static std::array<uint8_t, 32> HashOutputs(const ZCashTransaction& tx);
  static std::array<uint8_t, 32> HashTxIn(
      const ZCashTransaction::TxInput& tx_in);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
