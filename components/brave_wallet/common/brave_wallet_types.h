/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_

#include <limits>
#include <string>
#include <vector>

#include "boost/multiprecision/cpp_int.hpp"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

typedef boost::multiprecision::uint256_t uint256_t;
typedef boost::multiprecision::int256_t int256_t;

typedef boost::multiprecision::uint128_t uint128_t;
typedef boost::multiprecision::int128_t int128_t;

// Note that boost's int256/128_t has 256/128 precision bits and it uses an
// extra sign bit so its max and min value differs from 2's complement types.

// 2^255 - 1
constexpr int256_t kMax256BitInt = std::numeric_limits<int256_t>::max() >> 1;
// -(2^255 -1)
constexpr int256_t kMin256BitInt = std::numeric_limits<int256_t>::min() >> 1;

// 2^128 - 1
constexpr int128_t kMax128BitInt = std::numeric_limits<int128_t>::max() >> 1;
// -(2^128 -1)
constexpr int128_t kMin128BitInt = std::numeric_limits<int128_t>::min() >> 1;

constexpr uint64_t kMaxSafeIntegerUint64 = 9007199254740991;  // 2^53-1

// Determines the min/max value for Solidity types such as uint56
// uintN where 0 < N <= 256; N % 8 == 0
// Note that you shouldn't use uint256_t and int256_t in general
// for passing around values that need to be capped in those ranges.
// This is being used for sign typed data where values are not passed
// around.
bool ValidSolidityBits(size_t bits);
absl::optional<uint256_t> MaxSolidityUint(size_t bits);
absl::optional<int256_t> MaxSolidityInt(size_t bits);
absl::optional<int256_t> MinSolidityInt(size_t bits);

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

struct SolanaSignatureStatus {
  SolanaSignatureStatus() = default;
  SolanaSignatureStatus(uint64_t slot,
                        uint64_t confirmations,
                        const std::string& err,
                        const std::string& confirmation_status);
  ~SolanaSignatureStatus() = default;
  SolanaSignatureStatus(const SolanaSignatureStatus&) = default;
  bool operator==(const SolanaSignatureStatus&) const;
  bool operator!=(const SolanaSignatureStatus&) const;

  base::Value ToValue() const;
  static absl::optional<SolanaSignatureStatus> FromValue(
      const base::Value& value);

  // The slot the transaction was processed.
  uint64_t slot = 0;
  // Number of blocks since signature confirmation. It is specified as usize
  // (a Rust type) in the getSignatureStatuses JSON-RPC API spec, which will
  // be 4 bytes on a 32 bit target and 8 bytes on a 64 bit target. We use
  // uint64_t instead of size_t here to make sure our container is large enough
  // to handle both cases from server response.
  uint64_t confirmations = 0;
  // Non-empty if transaction failed. TransactionError object from the
  // getSignatureStatuses JSON-RPC API response will be written as a json
  // string to store here.
  std::string err;
  // The transaction's cluster confirmation status; either processed, confirmed,
  // or finalized.
  std::string confirmation_status;
};

struct SolanaAccountInfo {
  SolanaAccountInfo() = default;
  ~SolanaAccountInfo() = default;
  bool operator==(const SolanaAccountInfo&) const;
  bool operator!=(const SolanaAccountInfo&) const;

  // Number of lamports assigned to this account.
  uint64_t lamports;

  // base-58 encoded Pubkey of the program this account has been assigned to.
  std::string owner;

  // Data associated with the account, base64 encoded.
  std::string data;

  // Indicating if the account contains a program.
  bool executable;

  // The epoch at which this account will next owe rent.
  uint64_t rent_epoch;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_
