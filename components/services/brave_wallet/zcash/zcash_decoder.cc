/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/zcash/zcash_decoder.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/big_endian.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/services/brave_wallet/public/cpp/utils/protobuf_utils.h"
#include "brave/components/services/brave_wallet/public/proto/zcash_grpc_data.pb.h"

namespace brave_wallet {

namespace {
template <class T>
std::vector<uint8_t> ToVector(const T& t) {
  return std::vector<uint8_t>(t.begin(), t.end());
}

bool IsAllZero(const std::string& bytes) {
  return std::ranges::all_of(bytes, [](char c) { return c == 0; });
}

// XXXZZ diagnostic: walks the raw wire bytes of a lite-proto unknown-field set
// and logs every (field_number, wire_type) tag it contains. Used to discover
// field numbers the server sends that our .proto doesn't map -- e.g. ironwood
// actions arriving at a field number other than the one we declared.
void LogUnknownProtoFields(const std::string& where, const std::string& bytes) {
  if (bytes.empty()) {
    return;
  }
  const base::span<const uint8_t> data = base::as_byte_span(bytes);
  const size_t n = data.size();
  size_t i = 0;
  auto read_varint = [&](uint64_t* out) -> bool {
    uint64_t value = 0;
    int shift = 0;
    while (i < n && shift < 64) {
      uint8_t b = data[i++];
      value |= static_cast<uint64_t>(b & 0x7f) << shift;
      if ((b & 0x80) == 0) {
        *out = value;
        return true;
      }
      shift += 7;
    }
    return false;
  };
  std::string summary;
  while (i < n) {
    uint64_t tag = 0;
    if (!read_varint(&tag)) {
      break;
    }
    const uint32_t field_number = static_cast<uint32_t>(tag >> 3);
    const uint32_t wire_type = static_cast<uint32_t>(tag & 0x7);
    if (wire_type == 0) {  // varint
      uint64_t value = 0;
      if (!read_varint(&value)) {
        break;
      }
      summary += base::StringPrintf(" {field=%u varint value=%llu}",
                                    field_number,
                                    static_cast<unsigned long long>(value));
    } else if (wire_type == 2) {  // length-delimited (message/bytes/string)
      uint64_t len = 0;
      if (!read_varint(&len) || len > n - i) {
        break;
      }
      i += len;
      summary += base::StringPrintf(" {field=%u len-delimited bytes=%llu}",
                                    field_number,
                                    static_cast<unsigned long long>(len));
    } else if (wire_type == 1) {  // 64-bit
      if (n - i < 8) {
        break;
      }
      i += 8;
      summary += base::StringPrintf(" {field=%u fixed64}", field_number);
    } else if (wire_type == 5) {  // 32-bit
      if (n - i < 4) {
        break;
      }
      i += 4;
      summary += base::StringPrintf(" {field=%u fixed32}", field_number);
    } else {
      summary += base::StringPrintf(" {field=%u wire=%u unhandled}",
                                    field_number, wire_type);
      break;
    }
  }
  LOG(ERROR) << "XXXZZ unknown proto fields in " << where << " ("
             << bytes.size() << " bytes):" << summary;
}
}  // namespace

ZCashDecoder::ZCashDecoder() = default;

ZCashDecoder::~ZCashDecoder() = default;

void ZCashDecoder::ParseRawTransaction(const std::string& data,
                                       ParseRawTransactionCallback callback) {
  ::zcash::RawTransaction result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message || serialized_message->empty() ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::move(callback).Run(zcash::mojom::RawTransaction::New(
      ToVector(result.data()), result.height()));
}

void ZCashDecoder::ParseBlockID(const std::string& data,
                                ParseBlockIDCallback callback) {
  ::zcash::BlockID result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message || serialized_message->empty() ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::move(callback).Run(
      zcash::mojom::BlockID::New(result.height(), ToVector(result.hash())));
}

void ZCashDecoder::ParseGetAddressUtxos(const std::string& data,
                                        ParseGetAddressUtxosCallback callback) {
  ::zcash::GetAddressUtxosResponse result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
  for (int i = 0; i < result.addressutxos_size(); i++) {
    const ::zcash::ZCashUtxo& item = result.addressutxos(i);
    utxos.push_back(zcash::mojom::ZCashUtxo::New(
        item.address(), ToVector(item.txid()), item.index(),
        ToVector(item.script()), item.valuezat(), item.height()));
  }
  std::move(callback).Run(
      zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos)));
}

void ZCashDecoder::ParseSendResponse(const std::string& data,
                                     ParseSendResponseCallback callback) {
  ::zcash::SendResponse result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message || serialized_message->empty() ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::move(callback).Run(zcash::mojom::SendResponse::New(
      result.errorcode(), result.errormessage()));
}

void ZCashDecoder::ParseTreeState(const std::string& data,
                                  ParseTreeStateCallback callback) {
  ::zcash::TreeState result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message || serialized_message->empty() ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }

  LOG(ERROR) << "XXXZZ ParseTreeState height=" << result.height()
             << " orchardTree_len=" << result.orchardtree().size()
             << " ironwoodTree_len=" << result.ironwoodtree().size();

  std::move(callback).Run(zcash::mojom::TreeState::New(
      result.network(), result.height(), result.hash(), result.time(),
      result.saplingtree(), result.orchardtree(), result.ironwoodtree()));
}

void ZCashDecoder::ParseCompactBlocks(const std::vector<std::string>& data,
                                      ParseCompactBlocksCallback callback) {
  std::vector<zcash::mojom::CompactBlockPtr> parsed_blocks;
  // XXXZZ wire-boundary tally: counts raw actions carried by the gRPC proto
  // (before any feature flag / scanning) so we can tell whether the testnet
  // lightwalletd is sending any ironwood data at all.
  size_t wire_orchard_actions = 0;
  size_t wire_ironwood_actions = 0;
  uint64_t max_ironwood_tree_size = 0;
  for (const auto& data_block : data) {
    ::zcash::CompactBlock result;
    auto serialized_message = ResolveSerializedMessage(data_block);
    if (!serialized_message ||
        !result.ParseFromString(serialized_message.value()) ||
        serialized_message->empty()) {
      std::move(callback).Run(std::nullopt);
      return;
    }

    max_ironwood_tree_size =
        std::max<uint64_t>(max_ironwood_tree_size,
                           result.chainmetadata().ironwoodcommitmenttreesize());

    LogUnknownProtoFields(
        base::StringPrintf("CompactBlock height=%llu",
                           static_cast<unsigned long long>(result.height())),
        result.unknown_fields());
    LogUnknownProtoFields(
        base::StringPrintf("ChainMetadata height=%llu",
                           static_cast<unsigned long long>(result.height())),
        result.chainmetadata().unknown_fields());

    std::vector<zcash::mojom::CompactTxPtr> transactions;
    for (int i = 0; i < result.vtx_size(); i++) {
      const auto& vtx = result.vtx(i);
      wire_orchard_actions += vtx.actions_size();
      wire_ironwood_actions += vtx.ironwoodactions_size();

      // Log any tx that carries shielded actions or fields we don't map, so a
      // known ironwood txid can be located and its real wire layout inspected.
      // The hash on the wire is internal (little-endian) byte order; explorers
      // display the reverse, so log both to make correlation easy.
      if (vtx.actions_size() > 0 || vtx.ironwoodactions_size() > 0 ||
          !vtx.unknown_fields().empty()) {
        auto hash_bytes = base::as_byte_span(vtx.hash());
        std::vector<uint8_t> reversed(hash_bytes.rbegin(), hash_bytes.rend());
        std::string tx_desc = base::StringPrintf(
            "CompactTx height=%llu index=%llu txid=%s (wire=%s) "
            "orchard_actions=%d ironwood_actions=%d",
            static_cast<unsigned long long>(result.height()),
            static_cast<unsigned long long>(vtx.index()),
            base::HexEncode(reversed).c_str(),
            base::HexEncode(hash_bytes).c_str(), vtx.actions_size(),
            vtx.ironwoodactions_size());
        LOG(ERROR) << "XXXZZ " << tx_desc;

        // Break the tx's ironwood actions down into outputs (a note commitment
        // is present) vs spends (a nullifier is present). This is raw wire
        // structure -- ownership/discovery is decided later in the scanner with
        // the FVK, which the decoder process does not have.
        size_t iw_notes = 0;
        size_t iw_spends = 0;
        for (int j = 0; j < vtx.ironwoodactions_size(); j++) {
          const auto& action = vtx.ironwoodactions(j);
          if (!IsAllZero(action.cmx())) {
            iw_notes++;
          }
          if (!IsAllZero(action.nullifier())) {
            iw_spends++;
          }
        }
        if (vtx.ironwoodactions_size() > 0) {
          LOG(ERROR) << "XXXZZ   ironwood in tx: notes(cmx!=0)=" << iw_notes
                     << " spends(nullifier!=0)=" << iw_spends
                     << " total_actions=" << vtx.ironwoodactions_size();
        }

        LogUnknownProtoFields(tx_desc, vtx.unknown_fields());
      }
      std::vector<zcash::mojom::CompactOrchardActionPtr> orchard_actions;
      for (int j = 0; j < vtx.actions_size(); j++) {
        const auto& action = vtx.actions(j);
        orchard_actions.push_back(zcash::mojom::CompactOrchardAction::New(
            ToVector(action.nullifier()), ToVector(action.cmx()),
            ToVector(action.ephemeralkey()), ToVector(action.ciphertext())));
      }
      std::vector<zcash::mojom::CompactOrchardActionPtr> ironwood_actions;
      for (int j = 0; j < vtx.ironwoodactions_size(); j++) {
        const auto& action = vtx.ironwoodactions(j);
        ironwood_actions.push_back(zcash::mojom::CompactOrchardAction::New(
            ToVector(action.nullifier()), ToVector(action.cmx()),
            ToVector(action.ephemeralkey()), ToVector(action.ciphertext())));
      }
      auto tx = zcash::mojom::CompactTx::New(
          vtx.index(), ToVector(vtx.hash()), vtx.fee(),
          std::move(orchard_actions), std::move(ironwood_actions));
      transactions.push_back(std::move(tx));
    }

    parsed_blocks.push_back(zcash::mojom::CompactBlock::New(
        result.protoversion(), result.height(), ToVector(result.hash()),
        ToVector(result.prevhash()), result.time(), ToVector(result.header()),
        std::move(transactions),
        zcash::mojom::ChainMetadata::New(
            result.chainmetadata().orchardcommitmenttreesize(),
            result.chainmetadata().ironwoodcommitmenttreesize())));
  }

  LOG(ERROR) << "XXXZZ ParseCompactBlocks: blocks=" << parsed_blocks.size()
             << " wire_orchard_actions=" << wire_orchard_actions
             << " wire_ironwood_actions=" << wire_ironwood_actions
             << " max_ironwood_commitment_tree_size=" << max_ironwood_tree_size;

  std::move(callback).Run(std::move(parsed_blocks));
}

void ZCashDecoder::ParseLightdInfo(const std::string& data,
                                   ParseLightdInfoCallback callback) {
  ::zcash::LightdInfo result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message || serialized_message->empty() ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(
      zcash::mojom::LightdInfo::New(result.consensusbranchid()));
}

}  // namespace brave_wallet
