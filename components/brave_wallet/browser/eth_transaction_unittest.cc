/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/hd_key.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthTransactionUnitTest, GetMessageToSign) {
  base::test::SingleThreadTaskEnvironment task_environment(
      base::test::SingleThreadTaskEnvironment::MainThreadType::UI);

  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes(
      "00000000000000000000000000000000000000000000000000000000000000ad0000"
      "00000000000000000000000000000000000000000000000000000000fafa00000000"
      "00000000000000000000000000000000000000000000000000000dfa000000000000"
      "0000000000000000000000000000000000000000000000000dfa0000000000000000"
      "0000000000000000000000000000000000000000000000ad00000000000000000000"
      "0000000000000000000000000000000000000000000f000000000000000000000000"
      "000000000000000000000000000000000000000a0000000000000000000000000000"
      "0000000000000000000000000000000000df00000000000000000000000000000000"
      "0000000000000000000000000000000a000000000000000000000000000000000000"
      "00000000000000000000000000df0000000000000000000000000000000000000000"
      "00000000000000000000000a00000000000000000000000000000000000000000000"
      "0000000000000000000d",
      &data));
  EthTransaction tx1(mojom::TxData::New(
      "0x6", "0x9184e72a000", "0x974",
      "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x16345785d8a0000", data));

  tx1.GetMessageToSign(
      "0x0",
      base::BindLambdaForTesting([](bool success,
                                    const std::vector<uint8_t>& message) {
        EXPECT_TRUE(success);
        EXPECT_EQ(
            base::ToLowerASCII(base::HexEncode(message)),
            "61e1ec33764304dddb55348e7883d4437426f44ab3ef65e6da1e025734c03ff0");
      }));
  base::RunLoop().RunUntilIdle();

  data.clear();
  EthTransaction tx2(mojom::TxData::New(
      "0xb", "0x51f4d5c00", "0x5208",
      "0x656e929d6fc0cac52d3d9526d288fe02dcd56fbd", "0x2386f26fc10000", data));

  // with chain id (mainnet)
  tx2.GetMessageToSign(
      "0x1",
      base::BindLambdaForTesting([](bool success,
                                    const std::vector<uint8_t>& message) {
        EXPECT_TRUE(success);
        EXPECT_EQ(
            base::ToLowerASCII(base::HexEncode(message)),
            "f97c73fdca079da7652dbc61a46cd5aeef804008e057be3e712c43eac389aaf0");
      }));

  // EIP 155 test vectors
  const struct {
    std::string nonce;
    std::string gas_price;
    std::string gas_limit;
    std::string to;
    std::string value;
    const char* hash;
  } cases[] = {
      {"0x0", "0x4a817c800", "0x5208",
       "0x3535353535353535353535353535353535353535", "0x0",
       "e0be81f8d506dbe3a5549e720b51eb79492378d6638087740824f168667e5239"},
      {"0x8", "0x4a817c808", "0x2e248",
       "0x3535353535353535353535353535353535353535", "0x200",
       "50b6e7b58320c885ab7b2ee0d0b5813a697268bd2494a06de792790b13668c08"},
      {"0x9", "0x4a817c809", "0x33450",
       "0x3535353535353535353535353535353535353535", "0x2d9",
       "24fd18c70146a2b002254810473fa26b744f7899258a1f32924cc73e7a8f4d4f"},
      {"0x1", "0x4a817c801", "0xa410",
       "0x3535353535353535353535353535353535353535", "0x1",
       "42973b488dbb3aa237db3d1a3bad18a8d2148af795fb6cdbbbeef5c736df97b9"},
      {"0x2", "0x4a817c802", "0xf618",
       "0x3535353535353535353535353535353535353535", "0x8",
       "e68afed5d359c7e60a0408093da23c57b63e84acb2e368ac7c47630518d6f4d9"},
      {"0x3", "0x4a817c803", "0x14820",
       "0x3535353535353535353535353535353535353535", "0x1b",
       "bcb6f653e06c276a080e9d68e5a967847a896cf52a6dc81917dc2c57ae0a31ef"},
      {"0x4", "0x4a817c804", "0x19a28",
       "0x3535353535353535353535353535353535353535", "0x40",
       "244e4b57522352c3e9f93ad8ac8a47d1b46c3dcda6da2522caedad009ac9afb7"},
      {"0x5", "0x4a817c805", "0x1ec30",
       "0x3535353535353535353535353535353535353535", "0x7d",
       "581c0b79498b1cf1b8fa4f69bc5f21c0c60371cd08d4682b15c4334aac1cccfd"},
      {"0x6", "0x4a817c806", "0x23e38",
       "0x3535353535353535353535353535353535353535", "0xd8",
       "678ae2053a840f5fe550a63d724d1c85420d2955a0ccc4f868dd59e27afdf279"},
      {"0x7", "0x4a817c807", "0x29040",
       "0x3535353535353535353535353535353535353535", "0x157",
       "81aa03ada1474ff3ca4b86afb8e8c0f8b22791e156e706231a695ef8c51515ab"},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    EthTransaction tx(mojom::TxData::New(
        cases[i].nonce, cases[i].gas_price, cases[i].gas_limit, cases[i].to,
        cases[i].value, std::vector<uint8_t>()));
    // with chain id (mainnet)
    tx.GetMessageToSign(
        "0x1", base::BindLambdaForTesting(
                   [&](bool success, const std::vector<uint8_t>& message) {
                     EXPECT_TRUE(success);
                     EXPECT_EQ(base::ToLowerASCII(base::HexEncode(message)),
                               cases[i].hash);
                   }));
  }
}

TEST(EthTransactionUnitTest, GetSignedTransaction) {
  base::test::SingleThreadTaskEnvironment task_environment(
      base::test::SingleThreadTaskEnvironment::MainThreadType::UI);
  std::vector<uint8_t> private_key;
  EXPECT_TRUE(base::HexStringToBytes(
      "4646464646464646464646464646464646464646464646464646464646464646",
      &private_key));

  HDKey key;
  key.SetPrivateKey(private_key);
  EthTransaction tx(
      mojom::TxData::New("0x9", "0x4a817c800", "0x5208",
                         "0x3535353535353535353535353535353535353535",
                         "0xde0b6b3a7640000", std::vector<uint8_t>()));

  tx.GetMessageToSign(
      "0x1",
      base::BindLambdaForTesting([&](bool success,
                                     const std::vector<uint8_t>& message) {
        EXPECT_TRUE(success);
        EXPECT_EQ(
            base::ToLowerASCII(base::HexEncode(message)),
            "daf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53");

        int recid;
        const std::vector<uint8_t> signature = key.Sign(message, &recid);

        // invalid
        tx.ProcessSignature(std::vector<uint8_t>(63), recid, "0x1");
        EXPECT_EQ(tx.v_, 0);
        EXPECT_TRUE(tx.r_.empty());
        EXPECT_TRUE(tx.s_.empty());
        EXPECT_FALSE(tx.IsSigned());
        tx.ProcessSignature(std::vector<uint8_t>(65), recid, "0x1");
        EXPECT_FALSE(tx.IsSigned());
        tx.ProcessSignature(signature, -1, "0x1");
        EXPECT_FALSE(tx.IsSigned());
        tx.ProcessSignature(signature, 4, "0x1");
        EXPECT_FALSE(tx.IsSigned());

        tx.ProcessSignature(signature, recid, "0x1");
        tx.GetSignedTransaction(base::BindLambdaForTesting(
            [&](bool status, const std::string& signed_transaction) {
              EXPECT_TRUE(status);
              EXPECT_EQ(signed_transaction,
                        "0xf86c098504a817c8008252089435353535353535353535353535"
                        "353535353535"
                        "35880de0b6b3a76400008025a028ef61340bd939bc2195fe537567"
                        "866003e1a15d"
                        "3c71ff63e1590620aa636276a067cbe9d8997f761aecb703304b38"
                        "00ccf555c9f3"
                        "dc64214b297fb1966a3b6d83");
            }));
        base::RunLoop().RunUntilIdle();

        EXPECT_TRUE(tx.IsSigned());
        EXPECT_EQ(tx.v_, 37);
        // 18515461264373351373200002665853028612451056578545711640558177340181847433846
        EXPECT_EQ(
            base::HexEncode(tx.r_),
            "28EF61340BD939BC2195FE537567866003E1A15D3C71FF63E1590620AA636276");
        // 46948507304638947509940763649030358759909902576025900602547168820602576006531
        EXPECT_EQ(
            base::HexEncode(tx.s_),
            "67CBE9D8997F761AECB703304B3800CCF555C9F3DC64214B297FB1966A3B6D83");
      }));
  base::RunLoop().RunUntilIdle();
}

TEST(EthTransactionUnitTest, TransactionAndValue) {
  EthTransaction tx(
      mojom::TxData::New("0x9", "0x4a817c800", "0x5208",
                         "0x3535353535353535353535353535353535353535",
                         "0xde0b6b3a7640000", std::vector<uint8_t>()));
  base::Value tx_value = tx.ToValue();
  auto tx_from_value = EthTransaction::FromValue(tx_value);
  ASSERT_NE(tx_from_value, nullptr);
  EXPECT_EQ(*tx_from_value, tx);
}

TEST(EthTransactionUnitTest, GetBaseFee) {
  EthTransaction tx;
  EXPECT_EQ(tx.GetBaseFee(), uint256_t(53000));
}

TEST(EthTransactionUnitTest, GetDataFee) {
  EthTransaction tx1;
  EXPECT_EQ(tx1.GetDataFee(), uint256_t(0));

  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes(
      "00000000000000000000000000000000000000000000000000000000000000ad000000"
      "00"
      "0000000000000000000000000000000000000000000000000000fafa0000000000000000"
      "000000000000000000000000000000000000000000000dfa000000000000000000000000"
      "0000000000000000000000000000000000000dfa00000000000000000000000000000000"
      "000000000000000000000000000000ad0000000000000000000000000000000000000000"
      "00000000000000000000000f000000000000000000000000000000000000000000000000"
      "000000000000000a00000000000000000000000000000000000000000000000000000000"
      "000000df000000000000000000000000000000000000000000000000000000000000000a"
      "00000000000000000000000000000000000000000000000000000000000000df00000000"
      "0000000000000000000000000000000000000000000000000000000a0000000000000000"
      "00000000000000000000000000000000000000000000000d",
      &data));
  EthTransaction tx2(
      mojom::TxData::New("0x6", "0x9184e72a000", "0x974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data));
  EXPECT_EQ(tx2.GetDataFee(), uint256_t(1716));
}

TEST(EthTransactionUnitTest, GetUpFrontCost) {
  EthTransaction tx(mojom::TxData::New(
      "0x0", "0x3E8", "0x989680", "0x3535353535353535353535353535353535353535",
      "0x2A", std::vector<uint8_t>()));
  EXPECT_EQ(tx.GetUpfrontCost(), uint256_t(10000000042));
}

}  // namespace brave_wallet
