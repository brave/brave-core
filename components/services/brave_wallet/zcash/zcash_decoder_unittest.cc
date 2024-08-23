/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/zcash/zcash_decoder.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/services/brave_wallet/public/cpp/utils/protobuf_utils.h"
#include "brave/components/services/brave_wallet/public/proto/zcash_grpc_data.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::vector<uint8_t> ToBytes(const std::string& str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}

std::string GetHeader(size_t seed) {
  return "header" + base::NumberToString(seed);
}

std::string GetPrevHash(size_t seed) {
  return "prevhash" + base::NumberToString(seed);
}

std::string GetHash(size_t seed) {
  return "hash" + base::NumberToString(seed);
}

std::string GetCipherText(size_t seed) {
  return "ciphertext" + base::NumberToString(seed);
}

std::string GetCmx(size_t seed) {
  return "cmx" + base::NumberToString(seed);
}

std::string GetEphemeralKey(size_t seed) {
  return "ek" + base::NumberToString(seed);
}

std::string GetNullifier(size_t seed) {
  return "nullifier" + base::NumberToString(seed);
}

void GenerateOrchardAction(::zcash::CompactOrchardAction* action, size_t seed) {
  action->set_ciphertext(GetCipherText(seed));
  action->set_cmx(GetCmx(seed));
  action->set_ephemeralkey(GetEphemeralKey(seed));
  action->set_nullifier(GetNullifier(seed));
}

void GenerateTransaction(::zcash::CompactTx* tx, size_t seed) {
  tx->set_hash(GetHash(seed));
  tx->set_index(seed);
  tx->set_fee(seed);
  for (size_t i = 0; i < 5; i++) {
    auto* action = tx->add_actions();
    GenerateOrchardAction(action, seed + i);
  }
}

void GenerateBlock(::zcash::CompactBlock* block, size_t seed) {
  for (size_t i = 0; i < 5; i++) {
    auto* tx = block->add_vtx();
    GenerateTransaction(tx, seed + i);
  }
  block->set_height(seed);
  block->set_hash(GetHash(seed));
  block->set_prevhash(GetPrevHash(seed));
  block->set_header(GetHeader(seed));
  block->set_time(seed);
  block->mutable_chainmetadata()->set_orchardcommitmenttreesize(1);
}

std::vector<::zcash::CompactBlock> GenerateBlocks() {
  std::vector<::zcash::CompactBlock> result;
  for (size_t i = 0; i < 5; i++) {
    ::zcash::CompactBlock block;
    GenerateBlock(&block, i);
    result.push_back(std::move(block));
  }
  return result;
}

zcash::mojom::CompactBlockPtr GenerateMojoBlock(size_t seed) {
  std::vector<zcash::mojom::CompactTxPtr> transactions;
  for (size_t i = 0; i < 5; i++) {
    std::vector<zcash::mojom::CompactOrchardActionPtr> orchard_actions;
    for (size_t j = 0; j < 5; j++) {
      orchard_actions.push_back(zcash::mojom::CompactOrchardAction::New(
          ToBytes(GetNullifier(seed + i + j)), ToBytes(GetCmx(seed + i + j)),
          ToBytes(GetEphemeralKey(seed + i + j)),
          ToBytes(GetCipherText(seed + i + j))));
    }
    zcash::mojom::CompactTxPtr tx =
        zcash::mojom::CompactTx::New(seed + i, ToBytes(GetHash(seed + i)),
                                     seed + i, std::move(orchard_actions));
    transactions.push_back(std::move(tx));
  }
  auto chain_metadata = zcash::mojom::ChainMetadata::New(1);
  return zcash::mojom::CompactBlock::New(
      0, seed, ToBytes(GetHash(seed)), ToBytes(GetPrevHash(seed)), seed,
      ToBytes(GetHeader(seed)), std::move(transactions),
      std::move(chain_metadata));
}

std::vector<zcash::mojom::CompactBlockPtr> GenerateMojoBlocks() {
  std::vector<zcash::mojom::CompactBlockPtr> result;
  for (size_t i = 0; i < 5; i++) {
    zcash::mojom::CompactBlockPtr block = GenerateMojoBlock(i);
    result.push_back(std::move(block));
  }
  return result;
}

}  // namespace

class ZCashDecoderUnitTest : public testing::Test {
 public:
  ZCashDecoderUnitTest() = default;
  ZCashDecoderUnitTest(const ZCashDecoderUnitTest&) = delete;
  ZCashDecoderUnitTest& operator=(const ZCashDecoderUnitTest&) = delete;
  ~ZCashDecoderUnitTest() override = default;

  void SetUp() override { decoder_ = std::make_unique<ZCashDecoder>(); }

  void TearDown() override {}

  ZCashDecoder* decoder() { return decoder_.get(); }

 private:
  std::unique_ptr<ZCashDecoder> decoder_;
};

TEST_F(ZCashDecoderUnitTest, ParseBlockID) {
  ::zcash::BlockID response;
  response.set_height(64u);
  response.set_hash("abcd");

  // Correct input
  {
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(
        callback,
        Run(EqualsMojo(zcash::mojom::BlockID::New(64u, ToBytes("abcd")))));
    decoder()->ParseBlockID(GetPrefixedProtobuf(response.SerializeAsString()),
                            callback.Get());
  }
  // Missed protobuf prefix is incorrect
  {
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::BlockIDPtr())));
    decoder()->ParseBlockID(response.SerializeAsString(), callback.Get());
  }
  // Random string as input
  {
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::BlockIDPtr())));
    decoder()->ParseBlockID("123", callback.Get());
  }
  // Protobuf prefix exists but data format is wrong
  {
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::BlockIDPtr())));
    decoder()->ParseBlockID(GetPrefixedProtobuf(""), callback.Get());
  }
  // Corrupted input
  {
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::BlockIDPtr())));
    decoder()->ParseBlockID(
        GetPrefixedProtobuf(response.SerializeAsString()).substr(0, 5),
        callback.Get());
  }
  // Empty input
  {
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::BlockIDPtr())));
    decoder()->ParseBlockID("", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseGetAddressUtxos) {
  ::zcash::ZCashUtxo utxo1;
  utxo1.set_address("addr1");
  utxo1.set_txid("txid1");
  utxo1.set_valuezat(1u);

  ::zcash::ZCashUtxo utxo2;
  utxo2.set_address("addr2");
  utxo2.set_txid("txid2");
  utxo2.set_valuezat(2u);

  ::zcash::GetAddressUtxosResponse response;
  *(response.add_addressutxos()) = utxo1;
  *(response.add_addressutxos()) = utxo2;

  // Correct input
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(
        callback,
        Run(testing::Truly(
            [&](const zcash::mojom::GetAddressUtxosResponsePtr& result) {
              EXPECT_TRUE(result);
              EXPECT_EQ(result->address_utxos[0]->address, "addr1");
              EXPECT_EQ(result->address_utxos[0]->tx_id, ToBytes("txid1"));
              EXPECT_EQ(result->address_utxos[0]->value_zat, 1u);

              EXPECT_EQ(result->address_utxos[1]->address, "addr2");
              EXPECT_EQ(result->address_utxos[1]->tx_id, ToBytes("txid2"));
              EXPECT_EQ(result->address_utxos[1]->value_zat, 2u);
              return true;
            })));
    decoder()->ParseGetAddressUtxos(
        GetPrefixedProtobuf(response.SerializeAsString()), callback.Get());
  }
  // Missed protobuf prefix is incorrect
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponsePtr())));
    decoder()->ParseGetAddressUtxos(response.SerializeAsString(),
                                    callback.Get());
  }
  // Random string as input
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponsePtr())));
    decoder()->ParseGetAddressUtxos("123", callback.Get());
  }
  // Empty protobuf body
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponse::New())));
    decoder()->ParseGetAddressUtxos(GetPrefixedProtobuf(""), callback.Get());
  }
  // Protobuf prefix exists but data format is wrong
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponsePtr())));
    decoder()->ParseGetAddressUtxos(GetPrefixedProtobuf("1"), callback.Get());
  }
  // Corrupted input
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponsePtr())));
    decoder()->ParseGetAddressUtxos(
        GetPrefixedProtobuf(response.SerializeAsString()).substr(0, 5),
        callback.Get());
  }
  // Empty input
  {
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponsePtr())));
    decoder()->ParseGetAddressUtxos("", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseSendResponse) {
  ::zcash::SendResponse response;
  response.set_errorcode(1);
  response.set_errormessage("123");

  // Correct input
  {
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::SendResponse::New(1, "123"))));
    decoder()->ParseSendResponse(
        GetPrefixedProtobuf(response.SerializeAsString()), callback.Get());
  }
  // Missed protobuf prefix is incorrect
  {
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::SendResponsePtr())));
    decoder()->ParseSendResponse(response.SerializeAsString(), callback.Get());
  }
  // Corrupted input
  {
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::SendResponsePtr())));
    decoder()->ParseSendResponse(
        GetPrefixedProtobuf(response.SerializeAsString()).substr(0, 5),
        callback.Get());
  }
  // Protobuf prefix exists but data format is wrong
  {
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::SendResponsePtr())));
    decoder()->ParseSendResponse(GetPrefixedProtobuf(""), callback.Get());
  }
  // Random string as input
  {
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::SendResponsePtr())));
    decoder()->ParseSendResponse("123", callback.Get());
  }
  // Empty input
  {
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::SendResponsePtr())));
    decoder()->ParseSendResponse("", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseRawTransaction) {
  ::zcash::RawTransaction response;
  response.set_height(2);
  response.set_data("data");

  // Correct input
  {
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransaction::New(
                              ToBytes("data"), 2u))));
    decoder()->ParseRawTransaction(
        GetPrefixedProtobuf(response.SerializeAsString()), callback.Get());
  }
  // Missed protobuf prefix is incorrect
  {
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransactionPtr())));
    decoder()->ParseRawTransaction(response.SerializeAsString(),
                                   callback.Get());
  }
  // Corrupted input
  {
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransactionPtr())));
    decoder()->ParseRawTransaction(
        GetPrefixedProtobuf(response.SerializeAsString()).substr(0, 3),
        callback.Get());
  }
  // Protobuf prefix exists but data format is wrong
  {
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransactionPtr())));
    decoder()->ParseRawTransaction(GetPrefixedProtobuf(""), callback.Get());
  }
  // Random string as input
  {
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransactionPtr())));
    decoder()->ParseRawTransaction("123", callback.Get());
  }
  // Empty input
  {
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransactionPtr())));
    decoder()->ParseRawTransaction("", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseTreeState) {
  ::zcash::TreeState response;
  response.set_hash("hash");
  response.set_network("network");
  response.set_height(2);
  response.set_time(1);
  response.set_orchardtree("orchard_tree");
  response.set_saplingtree("sapling_tree");

  // Correct input
  {
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::TreeState::New(
                    "network", 2, "hash", 1, "sapling_tree", "orchard_tree"))));
    decoder()->ParseTreeState(GetPrefixedProtobuf(response.SerializeAsString()),
                              callback.Get());
  }
  // Missed protobuf prefix is incorrect
  {
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::TreeStatePtr())));
    decoder()->ParseTreeState(response.SerializeAsString(), callback.Get());
  }
  // Protobuf prefix exists but data format is wrong
  {
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::TreeStatePtr())));
    decoder()->ParseTreeState(GetPrefixedProtobuf(""), callback.Get());
  }
  // Corrupted input
  {
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::TreeStatePtr())));
    decoder()->ParseTreeState(
        GetPrefixedProtobuf(response.SerializeAsString()).substr(0, 5),
        callback.Get());
  }
  // Random string as input
  {
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::TreeStatePtr())));
    decoder()->ParseTreeState("123", callback.Get());
  }
  // Empty input
  {
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::TreeStatePtr())));
    decoder()->ParseTreeState("", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseCompactBlock) {
  std::vector<::zcash::CompactBlock> blocks = GenerateBlocks();
  std::vector<zcash::mojom::CompactBlockPtr> expected_blocks =
      GenerateMojoBlocks();

  // Correct input
  {
    std::vector<std::string> input;
    for (const auto& block : blocks) {
      input.push_back(GetPrefixedProtobuf(block.SerializeAsString()));
    }

    base::MockCallback<ZCashDecoder::ParseCompactBlocksCallback> callback;
    EXPECT_CALL(callback, Run(testing::_))
        .WillOnce([&](std::optional<std::vector<zcash::mojom::CompactBlockPtr>>
                          blocks) {
          EXPECT_EQ(expected_blocks.size(), blocks->size());
          for (size_t i = 0; i < expected_blocks.size(); i++) {
            EXPECT_TRUE(mojo::Equals(expected_blocks[i], blocks.value()[i]));
          }
        });
    decoder()->ParseCompactBlocks(input, callback.Get());
  }
  // Missed protobuf prefix is incorrect
  {
    std::vector<std::string> input;
    for (size_t i = 0; i < blocks.size(); i++) {
      input.push_back(blocks[i].SerializeAsString());
    }

    base::MockCallback<ZCashDecoder::ParseCompactBlocksCallback> callback;
    EXPECT_CALL(callback, Run(testing::Eq(std::nullopt)));
    decoder()->ParseCompactBlocks(input, callback.Get());
  }
  // Protobuf prefix exists but data format is wrong
  {
    std::vector<std::string> input;
    for (size_t i = 0; i < blocks.size(); i++) {
      input.push_back(GetPrefixedProtobuf(""));
    }

    base::MockCallback<ZCashDecoder::ParseCompactBlocksCallback> callback;
    EXPECT_CALL(callback, Run(testing::Eq(std::nullopt)));
    decoder()->ParseCompactBlocks(input, callback.Get());
  }
  // Corrupted input
  {
    std::vector<std::string> input;
    for (const auto& block : blocks) {
      input.push_back(
          GetPrefixedProtobuf(block.SerializeAsString()).substr(0, 5));
    }

    base::MockCallback<ZCashDecoder::ParseCompactBlocksCallback> callback;
    EXPECT_CALL(callback, Run(testing::Eq(std::nullopt)));
    decoder()->ParseCompactBlocks(input, callback.Get());
  }
  // Random string as input
  {
    base::MockCallback<ZCashDecoder::ParseCompactBlocksCallback> callback;
    EXPECT_CALL(callback, Run(testing::Eq(std::nullopt)));
    decoder()->ParseCompactBlocks({"123"}, callback.Get());
  }
  // Empty input
  {
    base::MockCallback<ZCashDecoder::ParseCompactBlocksCallback> callback;
    EXPECT_CALL(callback, Run(testing::_))
        .WillOnce([&](std::optional<std::vector<zcash::mojom::CompactBlockPtr>>
                          blocks) { EXPECT_EQ(0u, blocks->size()); });
    decoder()->ParseCompactBlocks({}, callback.Get());
  }
}

}  // namespace brave_wallet
