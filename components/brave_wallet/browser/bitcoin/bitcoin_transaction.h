/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace brave_wallet::bitcoin {

struct Outpoint {
  Outpoint();
  ~Outpoint();
  Outpoint(const Outpoint& other);
  Outpoint& operator=(const Outpoint& other);
  Outpoint(Outpoint&& other);
  Outpoint& operator=(Outpoint&& other);

  // TODO(apaymyshev): enforce 32 bytes array?
  std::vector<uint8_t> txid;
  uint32_t index = 0;

  std::string txid_hex() const { return base::HexEncode(txid); }

  bool operator<(const Outpoint& other) const {
    return std::tie(txid, index) < std::tie(other.txid, other.index);
  }
};

struct Input {
  Input();
  ~Input();
  Input(const Input& other);
  Input& operator=(const Input& other);
  Input(Input&& other);
  Input& operator=(Input&& other);

  Outpoint outpoint;
  std::string scriptpubkey;
  std::string scriptpubkey_type;
  std::string scriptpubkey_address;

  // TODO(apaymyshev): support large values
  uint64_t value = 0;

  // TODO(apaymyshev): need these fields
  // scriptsig
  // scriptsig_asm
  // witness
  // is_coinbase
  // sequence
};

struct Output {
  Output();
  ~Output();
  Output(const Output& other);
  Output& operator=(const Output& other);
  Output(Output&& other);
  Output& operator=(Output&& other);

  Outpoint outpoint;
  std::string scriptpubkey_type;
  std::string scriptpubkey_address;
  // TODO(apaymyshev): support large values
  // https://blockstream.info/api/tx/b36bced99cc459506ad2b3af6990920b12f6dc84f9c7ed0dd2c3703f94a4b692
  uint64_t value = 0;
};

struct Transaction {
  static absl::optional<Transaction> FromRpcValue(const base::Value& value);

  Transaction();
  ~Transaction();
  Transaction(const Transaction& other);
  Transaction& operator=(const Transaction& other);
  Transaction(Transaction&& other);
  Transaction& operator=(Transaction&& other);

  std::string txid;

  std::vector<Input> vin;
  std::vector<Output> vout;

  uint32_t block_height = 0;

  bool operator<(const Transaction& other) const;
};

}  // namespace brave_wallet::bitcoin

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_H_
