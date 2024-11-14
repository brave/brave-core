/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_TYPES_H_

#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace mojom {
// TODO(apaymyshev): Remove these aliases eventually.
inline constexpr KeyringId kDefaultKeyringId = KeyringId::kDefault;
inline constexpr KeyringId kSolanaKeyringId = KeyringId::kSolana;
inline constexpr KeyringId kFilecoinKeyringId = KeyringId::kFilecoin;
inline constexpr KeyringId kFilecoinTestnetKeyringId =
    KeyringId::kFilecoinTestnet;
}  // namespace mojom

using uint256_t = unsigned _BitInt(256);
using int256_t = _BitInt(256);

using uint128_t = unsigned _BitInt(128);
using int128_t = _BitInt(128);

// 2^255 - 1
inline constexpr int256_t kMax256BitInt = std::numeric_limits<int256_t>::max();
// -(2^255 -1)
inline constexpr int256_t kMin256BitInt = std::numeric_limits<int256_t>::min();

// 2^127 - 1
inline constexpr int128_t kMax128BitInt = std::numeric_limits<int128_t>::max();
// -(2^127 -1)
inline constexpr int128_t kMin128BitInt = std::numeric_limits<int128_t>::min();

inline constexpr uint64_t kMaxSafeIntegerUint64 = 9007199254740991;  // 2^53-1

// Determines the min/max value for Solidity types such as uint56
// uintN where 0 < N <= 256; N % 8 == 0
// Note that you shouldn't use uint256_t and int256_t in general
// for passing around values that need to be capped in those ranges.
// This is being used for sign typed data where values are not passed
// around.
bool ValidSolidityBits(size_t bits);
std::optional<uint256_t> MaxSolidityUint(size_t bits);
std::optional<int256_t> MaxSolidityInt(size_t bits);
std::optional<int256_t> MinSolidityInt(size_t bits);

struct Log {
  Log();
  ~Log();
  Log(const Log&);
  bool operator==(const Log&) const;
  bool operator!=(const Log&) const;

  std::string address;
  std::string block_hash;
  uint256_t block_number;
  std::string data;
  uint32_t log_index;
  bool removed;
  std::vector<std::string> topics;
  std::string transaction_hash;
  uint32_t transaction_index;
};

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
  std::vector<Log> logs;
  std::string logs_bloom;
  bool status = false;
};

struct ImportInfo {
  std::string mnemonic;
  bool is_legacy_crypto_wallets;
  size_t number_of_accounts;

  bool operator==(const ImportInfo&) const = default;
};

enum class ImportError { kJsonError = 1, kPasswordError, kInternalError };

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

  base::Value::Dict ToValue() const;
  static std::optional<SolanaSignatureStatus> FromValue(
      const base::Value::Dict& value);

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
