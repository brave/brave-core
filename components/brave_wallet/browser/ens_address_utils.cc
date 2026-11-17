/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ens_address_utils.h"

#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/bech32.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/solana_address.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

constexpr uint64_t kEthMainnetChainId = 1;
constexpr uint64_t kEthSepoliaChainId = 11155111;
constexpr uint64_t kEthHoodiChainId = 560048;
constexpr uint32_t kEnsEvmCoinTypeBase = 0x80000000;  // ENSIP-11

constexpr size_t kHash160Size = 20;
constexpr size_t kSha256Size = 32;
constexpr uint8_t kOpDup = 0x76;
constexpr uint8_t kOpHash160 = 0xa9;
constexpr uint8_t kOpEqual = 0x87;
constexpr uint8_t kOpEqualVerify = 0x88;
constexpr uint8_t kOpCheckSig = 0xac;
constexpr uint8_t kOp0 = 0x00;
constexpr uint8_t kOp1 = 0x51;

constexpr uint8_t kBtcP2pkhVersion = 0x00;
constexpr uint8_t kBtcP2shVersion = 0x05;
constexpr char kBtcHrp[] = "bc";

// OP_DUP OP_HASH160 <20> OP_EQUALVERIFY OP_CHECKSIG
std::optional<base::span<const uint8_t>> GetP2pkhHash(
    base::span<const uint8_t> script) {
  if (script.size() != 25 || script[0] != kOpDup || script[1] != kOpHash160 ||
      script[2] != kHash160Size || script[23] != kOpEqualVerify ||
      script[24] != kOpCheckSig) {
    return std::nullopt;
  }
  return script.subspan(3u, kHash160Size);
}

// OP_HASH160 <20> OP_EQUAL
std::optional<base::span<const uint8_t>> GetP2shHash(
    base::span<const uint8_t> script) {
  if (script.size() != 23 || script[0] != kOpHash160 ||
      script[1] != kHash160Size || script[22] != kOpEqual) {
    return std::nullopt;
  }
  return script.subspan(2u, kHash160Size);
}

std::string Base58CheckWithVersion(uint8_t version,
                                   base::span<const uint8_t> hash) {
  std::vector<uint8_t> payload = {version};
  payload.insert(payload.end(), hash.begin(), hash.end());
  return Base58EncodeWithCheck(payload);
}

std::optional<std::string> EncodeBtc(base::span<const uint8_t> script) {
  if (auto hash = GetP2pkhHash(script)) {
    return Base58CheckWithVersion(kBtcP2pkhVersion, *hash);
  }
  if (auto hash = GetP2shHash(script)) {
    return Base58CheckWithVersion(kBtcP2shVersion, *hash);
  }
  // OP_n <len> <program>. Only witness types Brave can send to.
  if (script.size() < 2 || script[1] != script.size() - 2) {
    return std::nullopt;
  }
  auto program = script.subspan(2u);
  if (script[0] == kOp0 &&
      (program.size() == kHash160Size || program.size() == kSha256Size)) {
    return bech32::EncodeForBitcoin(program, kBtcHrp, 0);
  }
  if (script[0] == kOp1 && program.size() == kSha256Size) {
    return bech32::EncodeForBitcoin(program, kBtcHrp, 1);
  }
  return std::nullopt;
}

std::optional<std::string> EncodeZec(base::span<const uint8_t> script) {
  // Brave's ZEC send only targets transparent P2PKH (t1).
  auto hash = GetP2pkhHash(script);
  if (!hash) {
    return std::nullopt;
  }
  return PubkeyHashToTransparentAddress(*hash, /*is_testnet=*/false);
}

std::optional<std::string> EncodeAda(base::span<const uint8_t> record) {
  if (record.empty()) {
    return std::nullopt;
  }
  // CIP-19 header: address type in the high nibble, network in the low one.
  // address-encoder stores Byron without the tag-24 wrapper and CRC that
  // FromCborBytes expects, so Byron is rejected.
  const uint8_t type = record[0] >> 4;
  const uint8_t network = record[0] & 0x0f;
  if (type ==
          static_cast<uint8_t>(CardanoAddress::AddressType::kByronAddress) ||
      network != static_cast<uint8_t>(CardanoAddress::NetworkTag::kMainnet)) {
    return std::nullopt;
  }
  auto address = CardanoAddress::FromCborBytes(record);
  if (!address || address->IsStakeOnlyAddress()) {
    return std::nullopt;
  }
  return address->ToString();
}

std::optional<std::string> EncodeFil(base::span<const uint8_t> record) {
  // Only secp256k1 (f1) and BLS (f3). FilAddress::FromBytes hits NOTREACHED()
  // on ID and actor records, and address-encoder can't write real f410 ones.
  if (record.empty() ||
      (record[0] !=
           static_cast<uint8_t>(mojom::FilecoinAddressProtocol::SECP256K1) &&
       record[0] !=
           static_cast<uint8_t>(mojom::FilecoinAddressProtocol::BLS))) {
    return std::nullopt;
  }
  auto address = FilAddress::FromBytes(mojom::kFilecoinMainnet, record);
  if (address.IsEmpty()) {
    return std::nullopt;
  }
  return address.EncodeAsString();
}

}  // namespace

std::optional<uint32_t> GetEnsCoinType(mojom::CoinType coin,
                                       std::string_view chain_id) {
  switch (coin) {
    case mojom::CoinType::ETH: {
      uint64_t id = 0;
      if (!chain_id.starts_with("0x") ||
          !base::HexStringToUInt64(chain_id, &id) || id == 0 ||
          id >= kEnsEvmCoinTypeBase) {
        return std::nullopt;
      }
      // ENSIP-19: L1 testnets may use coin type 60. They share mainnet
      // records, as EnsGetEthAddr does.
      if (id == kEthMainnetChainId || id == kEthSepoliaChainId ||
          id == kEthHoodiChainId) {
        return kEnsEthCoinType;
      }
      return kEnsEvmCoinTypeBase | static_cast<uint32_t>(id);
    }
    case mojom::CoinType::SOL:
      if (chain_id != mojom::kSolanaMainnet) {
        return std::nullopt;
      }
      break;
    case mojom::CoinType::BTC:
      if (chain_id != mojom::kBitcoinMainnet) {
        return std::nullopt;
      }
      break;
    case mojom::CoinType::ZEC:
      if (chain_id != mojom::kZCashMainnet) {
        return std::nullopt;
      }
      break;
    case mojom::CoinType::ADA:
      if (chain_id != mojom::kCardanoMainnet) {
        return std::nullopt;
      }
      break;
    case mojom::CoinType::DOT:
      if (chain_id != mojom::kPolkadotMainnet &&
          chain_id != mojom::kPolkadotMainnetAssetHub) {
        return std::nullopt;
      }
      break;
    case mojom::CoinType::FIL:
      if (chain_id != mojom::kFilecoinMainnet) {
        return std::nullopt;
      }
      break;
  }
  // mojom::CoinType values are SLIP-44 coin types.
  return static_cast<uint32_t>(coin);
}

std::optional<std::string> EnsAddrRecordToAddress(
    mojom::CoinType coin,
    base::span<const uint8_t> record) {
  switch (coin) {
    case mojom::CoinType::ETH: {
      auto bytes = record.to_fixed_extent<kEthAddressLength>();
      if (!bytes) {
        return std::nullopt;
      }
      auto address = EthAddress::FromBytes(*bytes);
      if (address.IsZeroAddress()) {
        return std::nullopt;
      }
      return address.ToHex();
    }
    case mojom::CoinType::SOL: {
      auto address = SolanaAddress::FromBytes(record);
      if (!address) {
        return std::nullopt;
      }
      return address->ToBase58();
    }
    case mojom::CoinType::DOT: {
      auto public_key = record.to_fixed_extent<kSs58PublicKeySize>();
      if (!public_key) {
        return std::nullopt;
      }
      Ss58Address address;
      address.prefix = 0;  // Polkadot
      base::span(address.public_key).copy_from(*public_key);
      return address.Encode();
    }
    case mojom::CoinType::BTC:
      return EncodeBtc(record);
    case mojom::CoinType::ZEC:
      return EncodeZec(record);
    case mojom::CoinType::ADA:
      return EncodeAda(record);
    case mojom::CoinType::FIL:
      return EncodeFil(record);
  }
  return std::nullopt;
}

}  // namespace brave_wallet
