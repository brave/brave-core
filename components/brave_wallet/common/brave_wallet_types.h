/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_

#include <string>
#include <vector>

#include "boost/multiprecision/cpp_int.hpp"

namespace brave_wallet {

typedef boost::multiprecision::uint256_t uint256_t;
// Note that boost's int256_t has 256 precision bits and it uses an extra
// sign bit so its max and min value differs from 2's complement types
typedef boost::multiprecision::int256_t int256_t;
typedef unsigned _BitInt(128) uint128_t;
typedef _BitInt(128) int128_t;

constexpr uint64_t kMaxSafeIntegerUint64 = 9007199254740991;  // 2^53-1

struct TransactionReceipt {
  TransactionReceipt();
  ~TransactionReceipt();
  TransactionReceipt(const TransactionReceipt&);
  bool operator==(const TransactionReceipt&) const;
  bool operator!=(const TransactionReceipt&) const;

  std::string transaction_hash;
  uint256_t transaction_index;
  std::string block_hash;
  uint256_t block_number;
  std::string from;
  std::string to;
  uint256_t cumulative_gas_used;
  uint256_t gas_used;
  std::string contract_address;
  std::vector<std::string> logs;
  std::string logs_bloom;
  bool status;
};

struct ImportInfo {
  std::string mnemonic;
  bool is_legacy_crypto_wallets;
  size_t number_of_accounts;
};

enum class ImportError {
  kNone = 0,
  kJsonError,
  kPasswordError,
  kInternalError
};

// https://docs.rs/solana-program/latest/solana_program/system_instruction/enum.SystemInstruction.html
enum class SolanaSystemInstruction {
  kCreateAccount = 0,
  kAssign,
  kTransfer,
  kCreateAccountWithSeed,
  kAdvanceNonceAccount,
  kWithdrawNonceAccount,
  kInitializeNonceAccount,
  kAuthorizeNonceAccount,
  kAllocate,
  kAllocateWithSeed,
  kAssignWithSeed,
  kTransferWithSeed
};

// https://docs.rs/spl-token/latest/spl_token/instruction/enum.TokenInstruction.html
enum class SolanaTokenInstruction {
  kInitializeMint = 0,
  kInitializeAccount,
  kInitializeMultisig,
  kTransfer,
  kApprove,
  kRevoke,
  kSetAuthority,
  kMintTo,
  kBurn,
  kCloseAccount,
  kFreezeAccount,
  kThawAccount,
  kTransferChecked,
  kApproveChecked,
  kMintToChecked,
  kBurnChecked,
  kInitializeAccount2,
  kSyncNative,
  kInitializeAccount3,
  kInitializeMultisig2,
  kInitializeMint2
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_
