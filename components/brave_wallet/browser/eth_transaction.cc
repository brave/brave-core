/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <utility>

#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"

namespace brave_wallet {

EthTransaction::EthTransaction(const uint256_t& nonce,
                               const uint256_t& gas_price,
                               const uint256_t& gas_limit,
                               const EthAddress& to,
                               const uint256_t& value,
                               const std::vector<uint8_t> data)
    : nonce_(nonce),
      gas_price_(gas_price),
      gas_limit_(gas_limit),
      to_(to),
      value_(value),
      data_(data),
      v_(0) {}

EthTransaction::~EthTransaction() = default;

std::vector<uint8_t> EthTransaction::GetMessageToSign(uint64_t chain_id) const {
  base::ListValue list;
  list.Append(RLPUint256ToBlobValue(nonce_));
  list.Append(RLPUint256ToBlobValue(gas_price_));
  list.Append(RLPUint256ToBlobValue(gas_limit_));
  list.Append(base::Value(to_.bytes()));
  list.Append(RLPUint256ToBlobValue(value_));
  list.Append(base::Value(data_));
  if (chain_id) {
    list.Append(RLPUint256ToBlobValue(chain_id));
    list.Append(RLPUint256ToBlobValue(0));
    list.Append(RLPUint256ToBlobValue(0));
  }

  const std::string message = RLPEncode(std::move(list));
  return KeccakHash(std::vector<uint8_t>(message.begin(), message.end()));
}

std::string EthTransaction::GetSignedTransaction() const {
  base::ListValue list;
  list.Append(RLPUint256ToBlobValue(nonce_));
  list.Append(RLPUint256ToBlobValue(gas_price_));
  list.Append(RLPUint256ToBlobValue(gas_limit_));
  list.Append(base::Value(to_.bytes()));
  list.Append(RLPUint256ToBlobValue(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  return ToHex(RLPEncode(std::move(list)));
}

// signature and recid will be used to produce v, r, s
void EthTransaction::ProcessSignature(const std::vector<uint8_t> signature,
                                      int recid,
                                      uint64_t chain_id) {
  if (signature.size() != 64) {
    LOG(ERROR) << __func__ << ": signature length should be 64 bytes";
    return;
  }
  if (recid < 0 || recid > 3) {
    LOG(ERROR) << __func__ << ": recovery id must be 0, 1, 2 or 3";
    return;
  }
  r_ = std::vector<uint8_t>(signature.begin(),
                            signature.begin() + signature.size() / 2);
  s_ = std::vector<uint8_t>(signature.begin() + signature.size() / 2,
                            signature.end());
  v_ = chain_id ? recid + (chain_id * 2 + 35) : recid + 27;
}

bool EthTransaction::IsSigned() const {
  return v_ != 0 && r_.size() != 0 && s_.size() != 0;
}

}  // namespace brave_wallet
