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

ZCashDecoder::ZCashDecoder() = default;

ZCashDecoder::~ZCashDecoder() = default;

void ZCashDecoder::ParseRawTransaction(const std::string& data,
                                       ParseRawTransactionCallback callback) {
  zcash::RawTransaction result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::vector<uint8_t> tx_data(result.data().begin(), result.data().end());
  std::move(callback).Run(
      mojom::RawTransaction::New(std::move(tx_data), result.height()));
}

void ZCashDecoder::ParseBlockID(const std::string& data,
                                ParseBlockIDCallback callback) {
  zcash::BlockID result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::vector<uint8_t> hash(result.hash().begin(), result.hash().end());
  std::move(callback).Run(
      mojom::BlockID::New(result.height(), std::move(hash)));
}

void ZCashDecoder::ParseGetAddressUtxos(const std::string& data,
                                        ParseGetAddressUtxosCallback callback) {
  zcash::GetAddressUtxosResponse result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::vector<mojom::ZCashUtxoPtr> utxos;
  for (int i = 0; i < result.addressutxos_size(); i++) {
    const zcash::ZCashUtxo& item = result.addressutxos(i);
    std::vector<uint8_t> tx_id(item.txid().begin(), item.txid().end());
    std::vector<uint8_t> script(item.script().begin(), item.script().end());

    utxos.push_back(mojom::ZCashUtxo::New(item.address(), std::move(tx_id),
                                          item.index(), std::move(script),
                                          item.valuezat(), item.height()));
  }
  std::move(callback).Run(
      mojom::GetAddressUtxosResponse::New(std::move(utxos)));
}

void ZCashDecoder::ParseSendResponse(const std::string& data,
                                     ParseSendResponseCallback callback) {
  zcash::SendResponse result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }
  std::move(callback).Run(
      mojom::SendResponse::New(result.errorcode(), result.errormessage()));
}

void ZCashDecoder::ParseTreeState(const std::string& data,
                                  ParseTreeStateCallback callback) {
  zcash::TreeState result;
  auto serialized_message = ResolveSerializedMessage(data);
  if (!serialized_message ||
      !result.ParseFromString(serialized_message.value())) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(mojom::TreeState::New(
      result.network(), result.height(), result.hash(), result.time(),
      result.saplingtree(), result.orchardtree()));
}

void ZCashDecoder::ParseCompactBlocks(const std::vector<std::string>& data,
                                      ParseCompactBlocksCallback callback) {
  std::vector<mojom::CompactBlockPtr> parsed_blocks;
  for (const auto& data_block : data) {
    zcash::CompactBlock result;
    auto serialized_message = ResolveSerializedMessage(data_block);
    if (!serialized_message ||
        !result.ParseFromString(serialized_message.value())) {
      LOG(ERROR) << "XXXZZZ 1";
      std::move(callback).Run(std::nullopt);
      return;
    }

    std::vector<mojom::CompactTxPtr> transactions;
    for (int i = 0; i < result.vtx_size(); i++) {
      const auto& vtx = result.vtx(i);
      std::vector<mojom::CompactOrchardActionPtr> orchard_actions;
      for (int j = 0; j < vtx.actions_size(); j++) {
        const auto& action = vtx.actions(j);
        orchard_actions.push_back(mojom::CompactOrchardAction::New(
            std::vector<uint8_t>(action.nullifier().begin(),
                                 action.nullifier().end()),
            std::vector<uint8_t>(action.cmx().begin(), action.cmx().end()),
            std::vector<uint8_t>(action.ephemeralkey().begin(),
                                 action.ephemeralkey().end()),
            std::vector<uint8_t>(action.ciphertext().begin(),
                                 action.ciphertext().end())));
      }
      auto tx = mojom::CompactTx::New(
          vtx.index(),
          std::vector<uint8_t>(vtx.hash().begin(), vtx.hash().end()), vtx.fee(),
          std::vector<mojom::CompactSaplingSpendPtr>(),
          std::vector<mojom::CompactSaplingOutputPtr>(),
          std::move(orchard_actions));
      transactions.push_back(std::move(tx));
    }

    parsed_blocks.push_back(mojom::CompactBlock::New(
        result.protoversion(), result.height(),
        std::vector<uint8_t>(result.hash().begin(), result.hash().end()),
        std::vector<uint8_t>(result.prevhash().begin(),
                             result.prevhash().end()),
        result.time(),
        std::vector<uint8_t>(result.header().begin(), result.header().end()),
        std::move(transactions),
        mojom::ChainMetadata::New(
            result.chainmetadata().saplingcommitmenttreesize(),
            result.chainmetadata().orchardcommitmenttreesize())));
  }

  std::move(callback).Run(std::move(parsed_blocks));
}

}  // namespace brave_wallet
