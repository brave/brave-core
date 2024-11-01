/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/zcash/zcash_decoder.h"

#include <utility>

#include "base/big_endian.h"
#include "brave/components/services/brave_wallet/public/cpp/utils/protobuf_utils.h"
#include "brave/components/services/brave_wallet/public/proto/zcash_grpc_data.pb.h"

namespace brave_wallet {

namespace {
template <class T>
std::vector<uint8_t> ToVector(const T& t) {
  return std::vector<uint8_t>(t.begin(), t.end());
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

  std::move(callback).Run(zcash::mojom::TreeState::New(
      result.network(), result.height(), result.hash(), result.time(),
      result.saplingtree(), result.orchardtree()));
}

void ZCashDecoder::ParseCompactBlocks(const std::vector<std::string>& data,
                                      ParseCompactBlocksCallback callback) {
  std::vector<zcash::mojom::CompactBlockPtr> parsed_blocks;
  for (const auto& data_block : data) {
    ::zcash::CompactBlock result;
    auto serialized_message = ResolveSerializedMessage(data_block);
    if (!serialized_message ||
        !result.ParseFromString(serialized_message.value()) ||
        serialized_message->empty()) {
      std::move(callback).Run(std::nullopt);
      return;
    }

    std::vector<zcash::mojom::CompactTxPtr> transactions;
    for (int i = 0; i < result.vtx_size(); i++) {
      const auto& vtx = result.vtx(i);
      std::vector<zcash::mojom::CompactOrchardActionPtr> orchard_actions;
      for (int j = 0; j < vtx.actions_size(); j++) {
        const auto& action = vtx.actions(j);
        orchard_actions.push_back(zcash::mojom::CompactOrchardAction::New(
            ToVector(action.nullifier()), ToVector(action.cmx()),
            ToVector(action.ephemeralkey()), ToVector(action.ciphertext())));
      }
      auto tx =
          zcash::mojom::CompactTx::New(vtx.index(), ToVector(vtx.hash()),
                                       vtx.fee(), std::move(orchard_actions));
      transactions.push_back(std::move(tx));
    }

    parsed_blocks.push_back(zcash::mojom::CompactBlock::New(
        result.protoversion(), result.height(), ToVector(result.hash()),
        ToVector(result.prevhash()), result.time(), ToVector(result.header()),
        std::move(transactions),
        zcash::mojom::ChainMetadata::New(
            result.chainmetadata().orchardcommitmenttreesize())));
  }

  std::move(callback).Run(std::move(parsed_blocks));
}

void ZCashDecoder::ParseSubtreeRoots(const std::vector<std::string>& data,
                                     ParseSubtreeRootsCallback callback) {
  std::vector<zcash::mojom::SubtreeRootPtr> roots;
  for (const auto& data_block : data) {
    ::zcash::SubtreeRoot result;
    auto serialized_message = ResolveSerializedMessage(data_block);
    if (!serialized_message ||
        !result.ParseFromString(serialized_message.value()) ||
        serialized_message->empty()) {
      std::move(callback).Run(std::nullopt);
      return;
    }
    roots.push_back(zcash::mojom::SubtreeRoot::New(
        ToVector(result.roothash()), ToVector(result.completingblockhash()),
        result.completingblockheight()));
  }
  std::move(callback).Run(std::move(roots));
}

}  // namespace brave_wallet
