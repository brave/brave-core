/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_types.h"

namespace brave_wallet {

TransactionReceipt::TransactionReceipt() = default;
TransactionReceipt::~TransactionReceipt() = default;
TransactionReceipt::TransactionReceipt(const TransactionReceipt&) = default;

bool TransactionReceipt::operator==(
    const TransactionReceipt& tx_receipt) const {
  return transaction_hash == tx_receipt.transaction_hash &&
         transaction_index == tx_receipt.transaction_index &&
         block_hash == tx_receipt.block_hash &&
         block_number == tx_receipt.block_number && from == tx_receipt.from &&
         to == tx_receipt.to &&
         cumulative_gas_used == tx_receipt.cumulative_gas_used &&
         gas_used == tx_receipt.gas_used &&
         contract_address == tx_receipt.contract_address &&
         logs_bloom == tx_receipt.logs_bloom && status == tx_receipt.status;
}

bool TransactionReceipt::operator!=(
    const TransactionReceipt& tx_receipt) const {
  return !operator==(tx_receipt);
}

}  // namespace brave_wallet
