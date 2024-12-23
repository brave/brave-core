/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthTransactionUnitTest, GetMessageToSign) {
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
  EthTransaction tx1 = *EthTransaction::FromTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data, false, std::nullopt));

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx1.GetMessageToSign(0))),
            "61e1ec33764304dddb55348e7883d4437426f44ab3ef65e6da1e025734c03ff0");

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx1.GetMessageToSign(1337))),
            "9ad82175b6921c5525fc52ebc08b97118cc9709952a16b2249a3f42d44614721");

  data.clear();
  EthTransaction tx2 = *EthTransaction::FromTxData(
      mojom::TxData::New("0x0b", "0x051f4d5c00", "0x5208",
                         "0x656e929d6fc0cac52d3d9526d288fe02dcd56fbd",
                         "0x2386f26fc10000", data, false, std::nullopt));

  // with chain id (mainnet)
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx2.GetMessageToSign(1))),
            "f97c73fdca079da7652dbc61a46cd5aeef804008e057be3e712c43eac389aaf0");

  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(tx2.GetMessageToSign(1, false))),
      "eb0b85051f4d5c0082520894656e929d6fc0cac52d3d9526d288fe02dcd56fbd872386f"
      "26fc1000080018080");

  // EIP 155 test vectors
  const struct {
    const char* nonce;
    const char* gas_price;
    const char* gas_limit;
    const char* to;
    const char* value;
    const char* hash;
  } cases[] = {
      {"0x00", "0x04a817c800", "0x5208",
       "0x3535353535353535353535353535353535353535", "0x00",
       "e0be81f8d506dbe3a5549e720b51eb79492378d6638087740824f168667e5239"},
      {"0x08", "0x04a817c808", "0x02e248",
       "0x3535353535353535353535353535353535353535", "0x0200",
       "50b6e7b58320c885ab7b2ee0d0b5813a697268bd2494a06de792790b13668c08"},
      {"0x09", "0x04a817c809", "0x033450",
       "0x3535353535353535353535353535353535353535", "0x02d9",
       "24fd18c70146a2b002254810473fa26b744f7899258a1f32924cc73e7a8f4d4f"},
      {"0x01", "0x04a817c801", "0xa410",
       "0x3535353535353535353535353535353535353535", "0x01",
       "42973b488dbb3aa237db3d1a3bad18a8d2148af795fb6cdbbbeef5c736df97b9"},
      {"0x02", "0x04a817c802", "0xf618",
       "0x3535353535353535353535353535353535353535", "0x08",
       "e68afed5d359c7e60a0408093da23c57b63e84acb2e368ac7c47630518d6f4d9"},
      {"0x03", "0x04a817c803", "0x014820",
       "0x3535353535353535353535353535353535353535", "0x1b",
       "bcb6f653e06c276a080e9d68e5a967847a896cf52a6dc81917dc2c57ae0a31ef"},
      {"0x04", "0x04a817c804", "0x019a28",
       "0x3535353535353535353535353535353535353535", "0x40",
       "244e4b57522352c3e9f93ad8ac8a47d1b46c3dcda6da2522caedad009ac9afb7"},
      {"0x05", "0x04a817c805", "0x01ec30",
       "0x3535353535353535353535353535353535353535", "0x7d",
       "581c0b79498b1cf1b8fa4f69bc5f21c0c60371cd08d4682b15c4334aac1cccfd"},
      {"0x06", "0x04a817c806", "0x023e38",
       "0x3535353535353535353535353535353535353535", "0xd8",
       "678ae2053a840f5fe550a63d724d1c85420d2955a0ccc4f868dd59e27afdf279"},
      {"0x07", "0x04a817c807", "0x029040",
       "0x3535353535353535353535353535353535353535", "0x0157",
       "81aa03ada1474ff3ca4b86afb8e8c0f8b22791e156e706231a695ef8c51515ab"},
  };

  for (const auto& entry : cases) {
    EthTransaction tx = *EthTransaction::FromTxData(mojom::TxData::New(
        entry.nonce, entry.gas_price, entry.gas_limit, entry.to, entry.value,
        std::vector<uint8_t>(), false, std::nullopt));
    // with chain id (mainnet)
    EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.GetMessageToSign(1))),
              entry.hash);
  }
}

TEST(EthTransactionUnitTest, GetSignedTransactionAndHash) {
  std::array<uint8_t, 32> private_key;
  EXPECT_TRUE(base::HexStringToSpan(
      "4646464646464646464646464646464646464646464646464646464646464646",
      private_key));

  HDKey key;
  key.SetPrivateKey(private_key);
  EthTransaction tx = *EthTransaction::FromTxData(mojom::TxData::New(
      "0x09", "0x4a817c800", "0x5208",
      "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
      std::vector<uint8_t>(), false, std::nullopt));

  const std::vector<uint8_t> message = tx.GetMessageToSign(1);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(message)),
            "daf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53");

  int recid;
  const std::vector<uint8_t> signature = key.SignCompact(message, &recid);

  // invalid
  tx.ProcessSignature(std::vector<uint8_t>(63), recid, 1);
  EXPECT_EQ(tx.v_, (uint256_t)0);
  EXPECT_TRUE(tx.r_.empty());
  EXPECT_TRUE(tx.s_.empty());
  EXPECT_FALSE(tx.IsSigned());
  tx.ProcessSignature(std::vector<uint8_t>(65), recid, 1);
  EXPECT_FALSE(tx.IsSigned());
  tx.ProcessSignature(signature, -1, 1);
  EXPECT_FALSE(tx.IsSigned());
  tx.ProcessSignature(signature, 4, 1);
  EXPECT_FALSE(tx.IsSigned());

  tx.ProcessSignature(signature, recid, 1);
  EXPECT_EQ(tx.GetSignedTransaction(),
            "0xf86c098504a817c8008252089435353535353535353535353535353535353535"
            "35880de0b6b3a76400008025a028ef61340bd939bc2195fe537567866003e1a15d"
            "3c71ff63e1590620aa636276a067cbe9d8997f761aecb703304b3800ccf555c9f3"
            "dc64214b297fb1966a3b6d83");
  EXPECT_EQ(
      tx.GetTransactionHash(),
      "0x33469b22e9f636356c4160a87eb19df52b7412e8eac32a4a55ffe88ea8350788");

  EXPECT_TRUE(tx.IsSigned());
  EXPECT_EQ(tx.v_, (uint256_t)37);
  // 18515461264373351373200002665853028612451056578545711640558177340181847433846
  EXPECT_EQ(base::HexEncode(tx.r_),
            "28EF61340BD939BC2195FE537567866003E1A15D3C71FF63E1590620AA636276");
  // 46948507304638947509940763649030358759909902576025900602547168820602576006531
  EXPECT_EQ(base::HexEncode(tx.s_),
            "67CBE9D8997F761AECB703304B3800CCF555C9F3DC64214B297FB1966A3B6D83");

  // Bigger chain_id
  const std::vector<uint8_t> message1337 = tx.GetMessageToSign(1337);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(message1337)),
            "9df81edc908cd622cbbab86525a4588fdcbaf6c88757f39b42b1f8f58fd617c2");
  recid = 0;
  const std::vector<uint8_t> signature1337 =
      key.SignCompact(message1337, &recid);
  tx.ProcessSignature(signature1337, recid, 1337);
  EXPECT_EQ(tx.GetSignedTransaction(),
            "0xf86e098504a817c8008252089435353535353535353535353535353535353535"
            "35880de0b6b3a764000080820a96a011d1f0b9de554ad9e690bb8355507007731b"
            "741e232ecb0dc183154c10c77875a03a4b32607c8c2287e82ae8c2a334d8412baf"
            "15e52ee25c531762dc34252a1365");
  EXPECT_EQ(
      tx.GetTransactionHash(),
      "0x3874c51841f3290a1b3e23152c474d361cc34e5b58f4adfcf4ff04bd77ed6b7a");
  EXPECT_TRUE(tx.IsSigned());
  EXPECT_EQ(tx.v_, (uint256_t)2710);
  EXPECT_EQ(base::HexEncode(tx.r_),
            "11D1F0B9DE554AD9E690BB8355507007731B741E232ECB0DC183154C10C77875");
  EXPECT_EQ(base::HexEncode(tx.s_),
            "3A4B32607C8C2287E82AE8C2A334D8412BAF15E52EE25C531762DC34252A1365");
}

TEST(EthTransactionUnitTest, TransactionAndValue) {
  EthTransaction tx = *EthTransaction::FromTxData(mojom::TxData::New(
      "0x09", "0x4a817c800", "0x5208",
      "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
      std::vector<uint8_t>(), false, std::nullopt));
  base::Value::Dict tx_value = tx.ToValue();
  auto tx_from_value = EthTransaction::FromValue(tx_value);
  ASSERT_NE(tx_from_value, std::nullopt);
  EXPECT_EQ(tx_from_value, tx);
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
      "00000000000000000000000000000000000000000000000000000000000000ad00000000"
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
  EthTransaction tx2 = *EthTransaction::FromTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data, false, std::nullopt));
  EXPECT_EQ(tx2.GetDataFee(), uint256_t(1716));
}

TEST(EthTransactionUnitTest, GetUpFrontCost) {
  EthTransaction tx = *EthTransaction::FromTxData(mojom::TxData::New(
      "0x00", "0x3E8", "0x989680", "0x3535353535353535353535353535353535353535",
      "0x2A", std::vector<uint8_t>(), false, std::nullopt));
  EXPECT_EQ(tx.GetUpfrontCost(), uint256_t(10000000042));
}

TEST(EthTransactionUnitTest, FromTxData) {
  auto tx = EthTransaction::FromTxData(mojom::TxData::New(
      "0x01", "0x3E8", "0x989680", "0x3535353535353535353535353535353535353535",
      "0x2A", std::vector<uint8_t>{1}, false, std::nullopt));
  ASSERT_TRUE(tx);
  EXPECT_EQ(tx->nonce(), uint256_t(1));
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));
  EXPECT_EQ(tx->gas_limit(), uint256_t(10000000));
  EXPECT_EQ(tx->to(),
            EthAddress::FromHex("0x3535353535353535353535353535353535353535"));
  EXPECT_EQ(tx->value(), uint256_t(42));
  EXPECT_EQ(tx->data(), std::vector<uint8_t>{1});

  // Empty nonce
  tx = EthTransaction::FromTxData(mojom::TxData::New(
      "", "0x3E8", "0x989680", "0x3535353535353535353535353535353535353535",
      "0x2A", std::vector<uint8_t>{1}, false, std::nullopt));
  ASSERT_TRUE(tx);
  EXPECT_FALSE(tx->nonce());

  // Missing values should not parse correctly
  EXPECT_FALSE(EthTransaction::FromTxData(mojom::TxData::New(
      "0x01", "", "0x989680", "0x3535353535353535353535353535353535353535",
      "0x2A", std::vector<uint8_t>{1}, false, std::nullopt)));
  EXPECT_FALSE(EthTransaction::FromTxData(mojom::TxData::New(
      "0x01", "0x3E8", "", "0x3535353535353535353535353535353535353535", "0x2A",
      std::vector<uint8_t>{1}, false, std::nullopt)));
  EXPECT_FALSE(EthTransaction::FromTxData(mojom::TxData::New(
      "0x01", "0x3E8", "0x989680", "0x3535353535353535353535353535353535353535",
      "", std::vector<uint8_t>{1}, false, std::nullopt)));

  // But missing data is allowed when strict is false
  tx = EthTransaction::FromTxData(
      mojom::TxData::New("", "0x3E8", "",
                         "0x3535353535353535353535353535353535353535", "",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      false);
  ASSERT_TRUE(tx);
  // Empty nonce should be std::nullopt
  EXPECT_FALSE(tx->nonce());
  // Unspecified value defaults to 0
  EXPECT_EQ(tx->gas_limit(), uint256_t(0));
  EXPECT_EQ(tx->value(), uint256_t(0));
  // you can still get at other data that is specified
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));

  // And try for missing gas_price too since the last test didn't try that
  tx = EthTransaction::FromTxData(
      mojom::TxData::New("0x1", "", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      false);
  ASSERT_TRUE(tx);
  // Unspecified value defaults to 0
  EXPECT_EQ(tx->gas_price(), uint256_t(0));
  // you can still get at other data that is specified
  EXPECT_EQ(tx->nonce(), uint256_t(1));
  EXPECT_EQ(tx->value(), uint256_t(42));
  EXPECT_EQ(tx->gas_limit(), uint256_t(10000000));
}

TEST(EthTransactionUnitTest, ProcessVRS) {
  EthTransaction tx;
  ASSERT_FALSE(tx.ProcessVRS({}, {}, {}));
  ASSERT_FALSE(tx.ProcessVRS({0}, {}, {}));
  EXPECT_EQ(tx.v(), (uint256_t)0);
  ASSERT_TRUE(tx.r().empty());
  ASSERT_TRUE(tx.s().empty());
  tx.set_nonce(0u);

  std::string r =
      "0x93b9121e82df014428924df439ff044f89c205dd76a194f8b11f50d2eade744e";
  std::string s =
      "0x7aa705c9144742836b7fbbd0745c57f67b60df7b8d1790fe59f91ed8d2bfc11d";
  ASSERT_TRUE(tx.ProcessVRS(*PrefixedHexStringToBytes("0x00"),
                            *PrefixedHexStringToBytes(r),
                            *PrefixedHexStringToBytes(s)));
  EXPECT_EQ(tx.v(), (uint256_t)0);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.r())), r.substr(2));
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.s())), s.substr(2));

  EXPECT_EQ(
      tx.GetSignedTransaction(),
      "0xf84980808080808080a093b9121e82df014428924df439ff044f89c205dd76a194f8b1"
      "1f50d2eade744ea07aa705c9144742836b7fbbd0745c57f67b60df7b8d1790fe59f91ed8"
      "d2bfc11d");
}

TEST(EthTransactionUnitTest, ProcessVRSFail) {
  EthTransaction tx;
  ASSERT_FALSE(tx.ProcessVRS({}, {}, {}));
  ASSERT_FALSE(tx.ProcessVRS({0}, {0}, {}));
  ASSERT_FALSE(tx.ProcessVRS({0}, {}, {0}));
  ASSERT_FALSE(tx.ProcessVRS({}, {}, {0}));
  EXPECT_EQ(tx.v(), (uint256_t)0);
  ASSERT_TRUE(tx.r().empty());
  ASSERT_TRUE(tx.s().empty());
}

}  // namespace brave_wallet
