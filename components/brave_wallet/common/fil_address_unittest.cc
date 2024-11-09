/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

bool ValidatePayload(const std::string& payload_hex,
                     mojom::FilecoinAddressProtocol protocol,
                     const std::string& network,
                     const std::string& expected_address) {
  std::vector<uint8_t> payload;
  auto result = base::HexStringToBytes(payload_hex, &payload);
  EXPECT_TRUE(result);
  auto address = FilAddress::FromPayload(payload, protocol, network);
  EXPECT_FALSE(address.IsEmpty());
  EXPECT_EQ(address.EncodeAsString(), expected_address);
  return FilAddress::IsValidAddress(address.EncodeAsString());
}

}  // namespace

TEST(FilAddressUnitTest, From) {
  // Valid secp256k1 address Testnet
  std::string address = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  auto fil_address = FilAddress::FromAddress(address);
  EXPECT_EQ(fil_address.EncodeAsString(), address);
  EXPECT_FALSE(fil_address.IsEmpty());
  EXPECT_EQ(fil_address.protocol(), mojom::FilecoinAddressProtocol::SECP256K1);

  // Valid secp256k1 address Mainnet
  address = "f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  fil_address = FilAddress::FromAddress(address);
  EXPECT_EQ(fil_address.EncodeAsString(), address);
  EXPECT_FALSE(fil_address.IsEmpty());
  EXPECT_EQ(fil_address.protocol(), mojom::FilecoinAddressProtocol::SECP256K1);

  // Valid BLS address Testnet
  address =
      "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
      "oz4o4tpa4mvigcrayh4a";
  fil_address = FilAddress::FromAddress(address);
  EXPECT_EQ(fil_address.EncodeAsString(), address);
  EXPECT_EQ(fil_address.protocol(), mojom::FilecoinAddressProtocol::BLS);

  // Valid BLS address Mainnet
  address =
      "f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
      "oz4o4tpa4mvigcrayh4a";
  fil_address = FilAddress::FromAddress(address);
  EXPECT_EQ(fil_address.EncodeAsString(), address);
  EXPECT_EQ(fil_address.protocol(), mojom::FilecoinAddressProtocol::BLS);

  address =
      "t3yaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaby2smx7a";
  EXPECT_EQ(FilAddress::FromAddress(address).EncodeAsString(), address);

  // wrong size for SECP256K1 account
  address =
      "f1wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
      "oz4o4tpa4mvigcrayh4a";
  EXPECT_NE(FilAddress::FromAddress(address).EncodeAsString(), address);

  // wrong size for BLS account
  address = "t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  EXPECT_NE(FilAddress::FromAddress(address).EncodeAsString(), address);

  // broken key
  address = "t1h3n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  EXPECT_NE(FilAddress::FromAddress(address).EncodeAsString(), address);

  address = "";
  EXPECT_EQ(FilAddress::FromAddress(address).EncodeAsString(), address);
  EXPECT_TRUE(FilAddress::FromAddress(address).IsEmpty());

  address = "f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y";
  fil_address = FilAddress::FromAddress(address);
  EXPECT_EQ(fil_address.EncodeAsString(), address);
  EXPECT_EQ(fil_address.protocol(), mojom::FilecoinAddressProtocol::DELEGATED);

  address = "t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y";
  fil_address = FilAddress::FromAddress(address);
  EXPECT_EQ(fil_address.EncodeAsString(), address);
  EXPECT_EQ(fil_address.protocol(), mojom::FilecoinAddressProtocol::DELEGATED);

  address = "f420frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y";
  EXPECT_NE(FilAddress::FromAddress(address).EncodeAsString(), address);
}

TEST(FilAddressUnitTest, IsValidAddress) {
  EXPECT_TRUE(
      FilAddress::IsValidAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  EXPECT_TRUE(FilAddress::IsValidAddress(
      "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
      "oz4o4tpa4mvigcrayh4a"));
  EXPECT_TRUE(
      FilAddress::IsValidAddress("f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  EXPECT_TRUE(FilAddress::IsValidAddress(
      "t3yaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaby2smx7a"));
  EXPECT_FALSE(FilAddress::IsValidAddress(
      "f1wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
      "oz4o4tpa4mvigcrayh4a"));
  EXPECT_FALSE(
      FilAddress::IsValidAddress("t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  EXPECT_FALSE(
      FilAddress::IsValidAddress("t1h3n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  EXPECT_FALSE(FilAddress::IsValidAddress(""));
  EXPECT_TRUE(FilAddress::IsValidAddress(
      "f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"));
  EXPECT_TRUE(FilAddress::IsValidAddress(
      "t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"));
  EXPECT_FALSE(FilAddress::IsValidAddress(
      "f420frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"));
}

TEST(FilAddressUnitTest, FromPayload) {
  EXPECT_TRUE(ValidatePayload("3F666D84EFEC7BEA64C90135BFEF2D9A3D834380",
                              mojom::FilecoinAddressProtocol::SECP256K1,
                              mojom::kFilecoinTestnet,
                              "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq"));

  EXPECT_TRUE(ValidatePayload("3F666D84EFEC7BEA64C90135BFEF2D9A3D834380",
                              mojom::FilecoinAddressProtocol::SECP256K1,
                              mojom::kFilecoinMainnet,
                              "f1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq"));

  EXPECT_TRUE(ValidatePayload(
      "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EBEAF1EF"
      "053F957A7678EE4DE0E32A83",
      mojom::FilecoinAddressProtocol::BLS, mojom::kFilecoinTestnet,
      "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4t"
      "pa4mvigcrayh4a"));

  EXPECT_TRUE(ValidatePayload(
      "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EBEAF1EF"
      "053F957A7678EE4DE0E32A83",
      mojom::FilecoinAddressProtocol::BLS, mojom::kFilecoinMainnet,
      "f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4t"
      "pa4mvigcrayh4a"));

  EXPECT_TRUE(ValidatePayload("8C60A3A9530DDD22F44C0B9C219E55E693335B51",
                              mojom::FilecoinAddressProtocol::DELEGATED,
                              mojom::kFilecoinMainnet,
                              "f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"));

  auto empty_address = FilAddress::FromPayload(
      {}, mojom::FilecoinAddressProtocol::SECP256K1, mojom::kFilecoinTestnet);
  EXPECT_EQ(empty_address.EncodeAsString(), "");
  EXPECT_TRUE(empty_address.IsEmpty());

  empty_address = FilAddress::FromPayload(
      {}, mojom::FilecoinAddressProtocol::SECP256K1, mojom::kFilecoinMainnet);
  EXPECT_EQ(empty_address.EncodeAsString(), "");
  EXPECT_TRUE(empty_address.IsEmpty());

  empty_address = FilAddress::FromPayload(
      {}, mojom::FilecoinAddressProtocol::BLS, mojom::kFilecoinMainnet);
  EXPECT_EQ(empty_address.EncodeAsString(), "");
  EXPECT_TRUE(empty_address.IsEmpty());
  empty_address = FilAddress::FromPayload(
      {}, mojom::FilecoinAddressProtocol::BLS, mojom::kFilecoinTestnet);
  EXPECT_EQ(empty_address.EncodeAsString(), "");
  EXPECT_TRUE(empty_address.IsEmpty());

  // wrong key/protocol pair bls
  std::vector<uint8_t> public_bls_key;
  auto result = base::HexStringToBytes(
      "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EBEAF1EF"
      "053F957A7678EE4DE0E32A83",
      &public_bls_key);
  EXPECT_TRUE(result);

  empty_address = FilAddress::FromPayload(
      public_bls_key, mojom::FilecoinAddressProtocol::SECP256K1,
      mojom::kFilecoinTestnet);
  EXPECT_EQ(empty_address.EncodeAsString(), "");
  EXPECT_TRUE(empty_address.IsEmpty());

  // wrong key/protocol pair secp
  std::vector<uint8_t> public_secp_key;
  result = base::HexStringToBytes("3F666D84EFEC7BEA64C90135BFEF2D9A3D834380",
                                  &public_secp_key);
  EXPECT_TRUE(result);

  empty_address = FilAddress::FromPayload(
      public_bls_key, mojom::FilecoinAddressProtocol::SECP256K1,
      mojom::kFilecoinTestnet);
  EXPECT_EQ(empty_address.EncodeAsString(), "");
  EXPECT_TRUE(empty_address.IsEmpty());
}

TEST(FilAddressUnitTest, FromUncompressedPublicKey) {
  std::string uncompressed_key_hex =
      "04E11EE1369349B470BD324C27F2BE177929B258B1B1302E15E6D0A0DCD4BB7C9219405B"
      "B1B40835A4249D28CD6B98F649034A7D5A5E8F1C9A0F557C532BE08EAA";
  std::vector<uint8_t> public_secp_key;
  auto result = base::HexStringToBytes(uncompressed_key_hex, &public_secp_key);
  EXPECT_TRUE(result);
  auto uncompressed_key = FilAddress::FromUncompressedPublicKey(
      public_secp_key, mojom::FilecoinAddressProtocol::SECP256K1,
      mojom::kFilecoinTestnet);
  EXPECT_EQ(uncompressed_key.EncodeAsString(),
            "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");

  uncompressed_key = FilAddress::FromUncompressedPublicKey(
      public_secp_key, mojom::FilecoinAddressProtocol::SECP256K1,
      mojom::kFilecoinMainnet);
  EXPECT_EQ(uncompressed_key.EncodeAsString(),
            "f1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");

  uncompressed_key = FilAddress::FromUncompressedPublicKey(
      public_secp_key, mojom::FilecoinAddressProtocol::BLS,
      mojom::kFilecoinTestnet);
  EXPECT_EQ(uncompressed_key.EncodeAsString(), "");
  EXPECT_TRUE(uncompressed_key.IsEmpty());
  uncompressed_key = FilAddress::FromUncompressedPublicKey(
      {}, mojom::FilecoinAddressProtocol::SECP256K1, mojom::kFilecoinTestnet);
  EXPECT_EQ(uncompressed_key.EncodeAsString(), "");
  EXPECT_TRUE(uncompressed_key.IsEmpty());
}

TEST(FilAddressUnitTest, Comparison) {
  EXPECT_EQ(
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));

  EXPECT_NE(
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
      FilAddress::FromAddress("f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  EXPECT_NE(
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
      FilAddress::FromAddress("t3h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"));
  EXPECT_NE(
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q"),
      FilAddress::FromAddress(
          "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"));

  EXPECT_NE(
      FilAddress::FromAddress(
          "t1wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"),
      FilAddress::FromAddress(
          "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"));
  EXPECT_NE(
      FilAddress::FromAddress(
          "f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"),
      FilAddress::FromAddress(
          "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"));
  EXPECT_EQ(
      FilAddress::FromAddress(
          "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"),
      FilAddress::FromAddress(
          "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"));
  EXPECT_EQ(
      FilAddress::FromAddress("f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"),
      FilAddress::FromAddress("f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"));
}

TEST(FilAddressUnitTest, GetBytes) {
  {
    auto fil_address = FilAddress::FromAddress(
        "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o"
        "4t"
        "pa4mvigcrayh4a");
    EXPECT_EQ(
        ToHex(fil_address.GetBytes()),
        base::ToLowerASCII("0x03B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587"
                           "697D3647798D9FE739167EBEAF1EF"
                           "053F957A7678EE4DE0E32A83"));
    EXPECT_TRUE(ValidatePayload(
        "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EBEAF1"
        "EF"
        "053F957A7678EE4DE0E32A83",
        mojom::FilecoinAddressProtocol::BLS, mojom::kFilecoinTestnet,
        "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o"
        "4t"
        "pa4mvigcrayh4a"));
  }
}

TEST(FilAddressUnitTest, FromBytes) {
  {
    auto fil_address = FilAddress::FromAddress(
        "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o"
        "4t"
        "pa4mvigcrayh4a");
    EXPECT_EQ(fil_address,
              FilAddress::FromBytes(
                  mojom::kFilecoinTestnet,
                  PrefixedHexStringToBytes(
                      "0x03B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587"
                      "697D3647798D9FE739167EBEAF1EF"
                      "053F957A7678EE4DE0E32A83")
                      .value()));
  }
  {
    auto fil_address = FilAddress::FromAddress(
        "f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o"
        "4t"
        "pa4mvigcrayh4a");
    EXPECT_EQ(fil_address,
              FilAddress::FromBytes(
                  mojom::kFilecoinMainnet,
                  PrefixedHexStringToBytes(
                      "0x03B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587"
                      "697D3647798D9FE739167EBEAF1EF"
                      "053F957A7678EE4DE0E32A83")
                      .value()));
  }
  {
    auto fil_address =
        FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
    EXPECT_EQ(fil_address,
              FilAddress::FromBytes(
                  mojom::kFilecoinTestnet,
                  PrefixedHexStringToBytes(
                      "0x013f1bf8bce258596c244ff262345965152c18baf8")
                      .value()));
  }
}

TEST(FilAddressUnitTest, ConvertFEVMtoFVM) {
  EXPECT_EQ("f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
            FilAddress::FromFEVMAddress(
                true, "0x8C60a3A9530dDD22F44C0B9c219E55E693335b51")
                .EncodeAsString());

  EXPECT_EQ("t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
            FilAddress::FromFEVMAddress(
                false, "0x8C60a3A9530dDD22F44C0B9c219E55E693335b51")
                .EncodeAsString());

  EXPECT_TRUE(FilAddress::FromFEVMAddress(
                  false, "8C60a3A9530dDD22F44C0B9c219E55E693335b51")
                  .IsEmpty());
}

TEST(FilAddressUnitTest, GetProtocolFromAddress) {
  EXPECT_EQ(
      mojom::FilecoinAddressProtocol::BLS,
      FilAddress::GetProtocolFromAddress(
          "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
          "oz4o4tpa4mvigcrayh4a"));

  EXPECT_EQ(mojom::FilecoinAddressProtocol::SECP256K1,
            FilAddress::GetProtocolFromAddress(
                "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq"));
  EXPECT_EQ(std::nullopt, FilAddress::GetProtocolFromAddress(
                              "t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y"));
}

}  // namespace brave_wallet
