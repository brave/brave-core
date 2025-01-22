/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kBip84TestMnemonic[] =
    "immense leader act drama someone renew become mention fragile wide "
    "cinnamon obtain wool window mirror";

// https://github.com/Electric-Coin-Company/zcash-android-wallet-sdk/blob/57f3b7ada6381dcf67b072c84d2f24a6d331045f/sdk-lib/src/androidTest/java/cash/z/ecc/android/sdk/jni/TransparentTest.kt#L42
constexpr char kDeputyVisaTestMnemonic[] =
    "deputy visa gentle among clean scout farm drive comfort patch skin salt "
    "ranch cool ramp warrior drink narrow normal lunch behind salt deal person";
}  // namespace

namespace brave_wallet {

using bip39::MnemonicToSeed;
using mojom::ZCashKeyId;

TEST(ZCashKeyringUnitTest, GetPubKey) {
  ZCashKeyring keyring(*MnemonicToSeed(kBip84TestMnemonic), false);

  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 0, 0))),
      "022C3E9812AA4DC6B93BAACAF54AFAA1FF6CCF65EA2493763AA66C8C8B18CD03CE");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 0, 1))),
      "02846F3C6216F046D0C69D22EDAEAE70C2C1887F3A81F63CFE604F645F68AD95D4");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 1, 0))),
      "02F464FA6B80AAC09328B3E1E09B5CA10C46AD1F404BCCB897200EC9A149C5DBB5");
}

TEST(ZCashKeyringUnitTest, GetTransparentAddress) {
  {
    ZCashKeyring keyring(*MnemonicToSeed(kBip84TestMnemonic), false);

    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 0))->address_string,
        "t1fhcesXQLT3U1t7caBYTpd59LiRV8tVfqj");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 1))->address_string,
        "t1WTZNzKCvU2GeM1ZWRyF7EvhMHhr7magiT");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 2))->address_string,
        "t1NPwPhNPHc4S8Xktzq4ygnLLLeuVKgCBZz");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 3))->address_string,
        "t1fe6a9otS98EdnucHLx6cT4am6Ae6THUgj");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 4))->address_string,
        "t1dA52xDndfEo9jJU7DBFSKuxKMqZMMLEGL");

    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 1, 0))->address_string,
        "t1RnTVUMzs1Hi2smgk6EJpVMA2upwTpESYP");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 1, 1))->address_string,
        "t1YRbQiMhwS7mui9r7E2aWGmkLC1xHQnFoc");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 1, 2))->address_string,
        "t1ZnyiWoaS9sceAQKjiK4ughBRQu9z2MXcB");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 1, 3))->address_string,
        "t1WwSQBqGUYf4AXKE6nNR3fp61awb6qsM8z");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 1, 4))->address_string,
        "t1WLKBRDxENW35SLrsW125Yt5tYJDYpBs1i");

    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 0, 0))->address_string,
        "t1LquzEnJVAqdRGeZJZbomrsKwPHuGhNJtm");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 0, 1))->address_string,
        "t1JgGisPzWWN1KMbN82bfKvk9Faa5eKqHUg");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 0, 2))->address_string,
        "t1fmqM2ud5FEgRTLo17QS4c2i575GD8QQCq");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 0, 3))->address_string,
        "t1SoBVfwQvRi7X8RuSxNvkm8NBrbbaUJo5c");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 0, 4))->address_string,
        "t1YuvWycJoxFVvuGHSJEkP5jtfe5VjXyGyj");

    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 1, 0))->address_string,
        "t1NFJ9Jn6su9bGH7mxjowBT6UsPTQdSpVF7");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 1, 1))->address_string,
        "t1SfWeR1cGcxv9G7sBMFP1JubyKBujSFu1C");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 1, 2))->address_string,
        "t1efWNVCp5sDvkRtrJxq6jrdJDbxwsYBxfb");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 1, 3))->address_string,
        "t1g4WDhMPb8e8GUWuV1JEAPa32bgVkeoSuJ");
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(1, 1, 4))->address_string,
        "t1ZTmbmys2n8UjVRb2tfE17A9V6ehZNqZ4W");
  }

  // https://github.com/zcash/zcash-android-wallet-sdk/blob/57f3b7ada6381dcf67b072c84d2f24a6d331045f/sdk-lib/src/androidTest/java/cash/z/ecc/android/sdk/jni/TransparentTest.kt#L49
  {
    ZCashKeyring keyring(*MnemonicToSeed(kDeputyVisaTestMnemonic), false);
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 0))->address_string,
        "t1PKtYdJJHhc3Pxowmznkg7vdTwnhEsCvR4");
  }

  {
    ZCashKeyring keyring(*MnemonicToSeed(kDeputyVisaTestMnemonic), true);
    EXPECT_EQ(
        keyring.GetTransparentAddress(ZCashKeyId(0, 0, 0))->address_string,
        "tm9v3KTsjXK8XWSqiwFjic6Vda6eHY9Mjjq");
  }
}

TEST(ZCashKeyringUnitTest, GetPubkey) {
  {
    ZCashKeyring keyring(*MnemonicToSeed(kBip84TestMnemonic), false);

    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 0, 0))),
        "022C3E9812AA4DC6B93BAACAF54AFAA1FF6CCF65EA2493763AA66C8C8B18CD03CE");
    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 0, 1))),
        "02846F3C6216F046D0C69D22EDAEAE70C2C1887F3A81F63CFE604F645F68AD95D4");
    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 1, 0))),
        "02F464FA6B80AAC09328B3E1E09B5CA10C46AD1F404BCCB897200EC9A149C5DBB5");

    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(1, 0, 0))),
        "038B5E4C440B54B5C63C460E8AE5042157917A6DEDF5CE951E22B85BA3E125D2CE");
    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(1, 0, 1))),
        "0309093DA6A36773835DF2C83431889BC7889E34B6A17DDFBEF608E79D951BD4BA");
    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(1, 1, 0))),
        "03D21E1C5E65174FB6214D0C818F6B2C1EAD0CC0F0B37CD7B12C7517D6C0A7BF3D");
  }

  // https://github.com/zcash/zcash-android-wallet-sdk/blob/57f3b7ada6381dcf67b072c84d2f24a6d331045f/sdk-lib/src/androidTest/java/cash/z/ecc/android/sdk/jni/TransparentTest.kt#L49
  {
    ZCashKeyring keyring(*MnemonicToSeed(kDeputyVisaTestMnemonic), false);
    EXPECT_EQ(
        base::HexEncode(*keyring.GetPubkey(ZCashKeyId(0, 0, 0))),
        "03B1D7FB28D17C125B504D06B1530097E0A3C76ADA184237E3BC0925041230A5AF");
  }

  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L73
  {
    ZCashKeyring keyring(
        std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                              0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                              0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                              0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}),
        false);
    EXPECT_EQ(*keyring.GetPubkeyHash(ZCashKeyId(1, 0, 3)),
              std::vector<uint8_t>({0xca, 0xd2, 0x68, 0x75, 0x8c, 0x5e, 0x71,
                                    0x49, 0x30, 0x66, 0x44, 0x6b, 0x98, 0xe7,
                                    0x1d, 0xf9, 0xd1, 0xd6, 0xa5, 0xca}));
  }
}

#if BUILDFLAG(ENABLE_ORCHARD)

TEST(ZCashKeyringUnitTest, GetShieldedAddress) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  ZCashKeyring keyring(
      std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}),
      false);
  // Shielded address
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L524
  EXPECT_EQ(keyring.GetShieldedAddress(ZCashKeyId(9, 0, 0))->address_string,
            "u1ddnjsdcpm36r6aq79n3s68shjweksnmwtdltrh046s8m6xcws9ygyawalxx8n6"
            "hg6vegk0wh8zjnafxgh6msppjsljvyt0ynece3lvm0");

  // Diversifier index
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L540
  EXPECT_EQ(keyring.GetShieldedAddress(ZCashKeyId(9, 0, 1))->address_string,
            "u1nztelxna9h7w0vtpd2xjhxt4lpu8s9cmdl8n8vcr7actf2ny45nd07cy8cyuhu"
            "vw3axcp545y0ktq9cezuzx84jyhex8dk4tdvwhu4dl");
}

TEST(ZCashKeyringUnitTest, GetUnifiedAddress) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  ZCashKeyring keyring(
      std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}),
      false);

  // Merged address
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L472
  {
    auto addr =
        keyring.GetUnifiedAddress(ZCashKeyId(8, 0, 0), ZCashKeyId(8, 0, 0));
    constexpr OrchardAddrRawPart expected_raw_bytes = {
        0x5f, 0x09, 0xa9, 0x80, 0x7a, 0x56, 0x32, 0x3b, 0x26, 0x3b, 0x05,
        0xdf, 0x36, 0x8d, 0xc2, 0x83, 0x91, 0xb2, 0x1a, 0x64, 0xa0, 0xe1,
        0xb4, 0x0f, 0x9a, 0x68, 0x03, 0xb7, 0xe6, 0x8f, 0x39, 0x05, 0x92,
        0x3f, 0x35, 0xcb, 0x01, 0xf1, 0x19, 0xb2, 0x23, 0xf4, 0x93};

    EXPECT_EQ(GetOrchardRawBytes(addr.value(), false).value(),
              expected_raw_bytes);
    EXPECT_EQ(GetTransparentRawBytes(addr.value(), false).value(),
              std::vector<uint8_t>({0x65, 0x70, 0x4e, 0x3a, 0xb7, 0x67, 0xca,
                                    0x57, 0x8e, 0x5b, 0x09, 0x2f, 0xb4, 0x76,
                                    0x04, 0xf6, 0x59, 0x47, 0x5b, 0xae}));
    EXPECT_EQ(
        addr.value(),
        "u1n9znrl4zyuvds24rcapzglzapqdlax4r8rgkvek0y0xlzfjfvn7zexelrafkchea24w0"
        "30cr"
        "9jqsel7t8lvveaq7m7w4z0khmrlzc6748w9ldlccy02scd5xngtcv2yy4ctnyu9zn5m");
  }

  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L196
  {
    auto addr =
        keyring.GetUnifiedAddress(ZCashKeyId(3, 0, 0), ZCashKeyId(3, 0, 0));
    constexpr OrchardAddrRawPart expected_raw_bytes = {
        0x31, 0x84, 0x46, 0x83, 0xa0, 0x7b, 0xf8, 0xe3, 0x00, 0x57, 0x90,
        0x2b, 0x0d, 0x23, 0xe2, 0xb2, 0xce, 0x9c, 0xad, 0x0b, 0x22, 0x19,
        0x02, 0x38, 0xca, 0x4f, 0x32, 0x9d, 0xa9, 0x2c, 0x79, 0x79, 0x05,
        0x2b, 0x00, 0xf7, 0x35, 0xcb, 0x21, 0x06, 0x71, 0xbd, 0xb0};
    EXPECT_EQ(GetOrchardRawBytes(addr.value(), false).value(),
              expected_raw_bytes);
    EXPECT_EQ(GetTransparentRawBytes(addr.value(), false).value(),
              std::vector<uint8_t>({0x87, 0x1a, 0x08, 0x9d, 0x44, 0x62, 0x68,
                                    0xaa, 0x7a, 0xc0, 0x3d, 0x2a, 0x6f, 0x60,
                                    0xae, 0x70, 0x80, 0x8f, 0x39, 0x74}));
    EXPECT_EQ(addr.value(),
              "u1snf9yr883aj2hm8pksp9aymnqdwzy42rpzuffevj35hhxeckays5pcpeq7vy2"
              "mtgzlcuc4mnh9443qnuyje0yx6h59angywka4v2ap6kchh2j96ezf9w0c0auyz3w"
              "wts2lx5gmk2sk9");
  }

  // Diversifier used
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L232
  {
    auto addr =
        keyring.GetUnifiedAddress(ZCashKeyId(3, 0, 2), ZCashKeyId(3, 0, 2));
    constexpr OrchardAddrRawPart expected_raw_bytes = {
        0x55, 0x1a, 0x16, 0xfb, 0x00, 0xd5, 0x48, 0x2a, 0x2a, 0xb2, 0x51,
        0x82, 0x56, 0x06, 0x61, 0xcf, 0xd7, 0x4a, 0x60, 0xfe, 0x77, 0xa0,
        0xf1, 0xc9, 0x34, 0x7f, 0x16, 0xba, 0x52, 0x49, 0x88, 0x9f, 0x3a,
        0xe3, 0x46, 0xed, 0x69, 0x38, 0xc3, 0x0a, 0xbf, 0xaf, 0x80};
    EXPECT_EQ(GetOrchardRawBytes(addr.value(), false).value(),
              expected_raw_bytes);
    EXPECT_EQ(GetTransparentRawBytes(addr.value(), false).value(),
              std::vector<uint8_t>({0x3e, 0x02, 0xe0, 0x8b, 0x59, 0x65, 0xfc,
                                    0xe9, 0xc2, 0x0c, 0xe6, 0xde, 0x6f, 0x94,
                                    0x07, 0x67, 0x4d, 0x01, 0xba, 0x02}));
    EXPECT_EQ(addr.value(),
              "u1glq6lzrxc7n7r4c922qht20zmpxyl0asfuldrjcaddagfspxpc3040fdfwdf5c"
              "rw4j6j6wkx4r038s0w24w7enpyfmmdfu9t9p2amxazgvasms8l03l3j5yhrrfqy6"
              "xzue5uggef4p8");
  }
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
