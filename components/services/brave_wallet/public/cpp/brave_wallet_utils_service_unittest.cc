/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/public/cpp/brave_wallet_utils_service.h"

#include <vector>

#include "base/auto_reset.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/services/brave_wallet/public/cpp/utils/protobuf_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "brave/components/services/brave_wallet/public/proto/zcash_grpc_data.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::string GetBlockIdPayload(uint64_t height) {
  ::zcash::BlockID response;
  response.set_height(height);
  response.set_hash("abcd");

  return GetPrefixedProtobuf(response.SerializeAsString());
}

zcash::mojom::BlockIDPtr GetBlockIdResult(uint64_t height) {
  return zcash::mojom::BlockID(height, std::vector<uint8_t>{'a', 'b', 'c', 'd'})
      .Clone();
}

}  // namespace

class BraveWalletUtilsServiceUnitTest : public testing::Test {
 public:
  BraveWalletUtilsServiceUnitTest() = default;
  ~BraveWalletUtilsServiceUnitTest() override = default;

 private:
  base::test::TaskEnvironment task_environment_;
  base::AutoReset<bool> scoped_in_process_{
      BraveWalletUtilsService::ScopedInProcessServiceForTesting()};
};

TEST_F(BraveWalletUtilsServiceUnitTest, CreateZCashDecoder) {
  BraveWalletUtilsService service;

  mojo::Remote<zcash::mojom::ZCashDecoder> zcash_decoder;

  service.CreateZCashDecoder(zcash_decoder.BindNewPipeAndPassReceiver());

  base::test::TestFuture<zcash::mojom::BlockIDPtr> future;
  zcash_decoder->ParseBlockID(GetBlockIdPayload(123), future.GetCallback());
  EXPECT_EQ(GetBlockIdResult(123), future.Take());
}

TEST_F(BraveWalletUtilsServiceUnitTest, CreateTwoDecoders) {
  BraveWalletUtilsService service;

  mojo::Remote<zcash::mojom::ZCashDecoder> zcash_decoder1;
  mojo::Remote<zcash::mojom::ZCashDecoder> zcash_decoder2;

  service.CreateZCashDecoder(zcash_decoder1.BindNewPipeAndPassReceiver());
  service.CreateZCashDecoder(zcash_decoder2.BindNewPipeAndPassReceiver());

  base::test::TestFuture<zcash::mojom::BlockIDPtr> future1;
  zcash_decoder1->ParseBlockID(GetBlockIdPayload(123), future1.GetCallback());
  EXPECT_EQ(GetBlockIdResult(123), future1.Take());

  base::test::TestFuture<zcash::mojom::BlockIDPtr> future2;
  zcash_decoder2->ParseBlockID(GetBlockIdPayload(77), future2.GetCallback());
  EXPECT_EQ(GetBlockIdResult(77), future2.Take());
}

}  // namespace brave_wallet
