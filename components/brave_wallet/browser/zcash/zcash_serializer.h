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

class ZCashSerializer {
 public:
  static std::vector<uint8_t> CalculateTxIdDigest(
      const ZCashTransaction& zcash_transaction);
  static std::vector<uint8_t> CalculateSignatureDigest(
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

  static std::vector<uint8_t> HashHeader(const ZCashTransaction& tx);
  static std::vector<uint8_t> HashPrevouts(const ZCashTransaction& tx);
  static std::vector<uint8_t> HashSequences(const ZCashTransaction& tx);
  static std::vector<uint8_t> HashOutputs(const ZCashTransaction& tx);
  static std::vector<uint8_t> HashTxIn(const ZCashTransaction::TxInput& tx_in);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
