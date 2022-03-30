/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TRANSACTION_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class FilTransaction {
 public:
  FilTransaction();
  FilTransaction(const FilTransaction&);
  virtual ~FilTransaction();
  bool operator==(const FilTransaction&) const;
  bool operator!=(const FilTransaction&) const;

  static absl::optional<FilTransaction> FromTxData(
      const mojom::FilTxDataPtr& tx_data);

  // https://github.com/filecoin-project/lotus/blob/master/chain/types/message.go
  absl::optional<uint64_t> nonce() const { return nonce_; }
  std::string gas_premium() const { return gas_premium_; }
  std::string gas_fee_cap() const { return gas_fee_cap_; }
  int64_t gas_limit() const { return gas_limit_; }
  std::string max_fee() const { return max_fee_; }
  FilAddress to() const { return to_; }
  FilAddress from() const { return from_; }
  std::string value() const { return value_; }

  void set_to(FilAddress to) { to_ = to; }
  void set_from(FilAddress from) { from_ = from; }
  void set_value(const std::string& value) { value_ = value; }
  void set_nonce(absl::optional<uint64_t> nonce) { nonce_ = nonce; }
  void set_gas_premium(const std::string& gas_premium) {
    gas_premium_ = gas_premium;
  }
  void set_fee_cap(const std::string& gas_fee_cap) {
    gas_fee_cap_ = gas_fee_cap;
  }
  void set_gas_limit(int64_t gas_limit) { gas_limit_ = gas_limit; }
  void set_max_fee(const std::string& max_fee) { max_fee_ = max_fee; }

  std::string GetMessageToSign() const;
  base::Value ToValue() const;
  mojom::FilTxDataPtr ToFilTxData() const;
  absl::optional<std::string> GetSignedTransaction(const std::string& private_key_base64) const;
  static absl::optional<FilTransaction> FromValue(const base::Value& value);

 private:
  bool IsEqual(const FilTransaction& tx) const;
  base::Value GetMessageToSignAsValue() const;

  absl::optional<uint64_t> nonce_;
  std::string gas_premium_;
  std::string gas_fee_cap_;
  int64_t gas_limit_ = 0;
  std::string max_fee_;
  std::string cid_;
  FilAddress to_;
  FilAddress from_;
  std::string value_;

  std::string signature_;

 private:
  FilTransaction(absl::optional<uint64_t> nonce,
                 const std::string& gas_premium,
                 const std::string& gas_fee_cap,
                 int64_t gas_limit,
                 const std::string& max_fee,
                 const FilAddress& to,
                 const FilAddress& from,
                 const std::string& value,
                 const std::string& cid);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TRANSACTION_H_
