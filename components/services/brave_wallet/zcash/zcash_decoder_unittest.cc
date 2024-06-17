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
  // Correct
  {
    ::zcash::BlockID block_id;
    block_id.set_height(64u);
    block_id.set_hash("abcd");
    zcash::mojom::BlockIDPtr result;
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(
        callback,
        Run(EqualsMojo(zcash::mojom::BlockID::New(64u, ToBytes("abcd")))));
    decoder()->ParseBlockID(GetPrefixedProtobuf(block_id.SerializeAsString()),
                            callback.Get());
  }
  // Incorrect
  {
    zcash::mojom::BlockIDPtr result;
    std::optional<std::string> error;
    base::MockCallback<ZCashDecoder::ParseBlockIDCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::BlockIDPtr())));
    decoder()->ParseBlockID("123", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseGetAddressUtxos) {
  // Correct
  {
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
  // Incorrect
  {
    zcash::mojom::GetAddressUtxosResponsePtr result;
    std::optional<std::string> error;
    base::MockCallback<ZCashDecoder::ParseGetAddressUtxosCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::GetAddressUtxosResponsePtr())));
    decoder()->ParseGetAddressUtxos("123", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseSendResponse) {
  // Correct
  {
    ::zcash::SendResponse response;
    response.set_errorcode(1);
    response.set_errormessage("123");
    zcash::mojom::SendResponsePtr result;
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::SendResponse::New(1, "123"))));
    decoder()->ParseSendResponse(
        GetPrefixedProtobuf(response.SerializeAsString()), callback.Get());
  }
  // Incorrect
  {
    zcash::mojom::SendResponsePtr result;
    std::optional<std::string> error;
    base::MockCallback<ZCashDecoder::ParseSendResponseCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::SendResponsePtr())));
    decoder()->ParseSendResponse("123", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseRawTransaction) {
  // Correct
  {
    ::zcash::RawTransaction response;
    response.set_height(2);
    response.set_data("data");

    zcash::mojom::RawTransactionPtr result;
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransaction::New(
                              ToBytes("data"), 2u))));
    decoder()->ParseRawTransaction(
        GetPrefixedProtobuf(response.SerializeAsString()), callback.Get());
  }
  // Incorrect
  {
    zcash::mojom::RawTransactionPtr result;
    std::optional<std::string> error;
    base::MockCallback<ZCashDecoder::ParseRawTransactionCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::RawTransactionPtr())));
    decoder()->ParseRawTransaction("123", callback.Get());
  }
}

TEST_F(ZCashDecoderUnitTest, ParseTreeState) {
  // Correct
  {
    ::zcash::TreeState response;
    response.set_hash("hash");
    response.set_network("network");
    response.set_height(2);
    response.set_time(1);
    response.set_orchardtree("orchard_tree");
    response.set_saplingtree("sapling_tree");

    zcash::mojom::TreeStatePtr result;
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback,
                Run(EqualsMojo(zcash::mojom::TreeState::New(
                    "network", 2, "hash", 1, "sapling_tree", "orchard_tree"))));
    decoder()->ParseTreeState(GetPrefixedProtobuf(response.SerializeAsString()),
                              callback.Get());
  }
  // Incorrect
  {
    zcash::mojom::TreeStatePtr result;
    std::optional<std::string> error;
    base::MockCallback<ZCashDecoder::ParseTreeStateCallback> callback;
    EXPECT_CALL(callback, Run(EqualsMojo(zcash::mojom::TreeStatePtr())));
    decoder()->ParseTreeState("123", callback.Get());
  }
}

}  // namespace brave_wallet
