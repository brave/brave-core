/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
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

TEST(ZCashKeyringUnitTest, GetAddress) {
  {
    ZCashKeyring keyring(*MnemonicToSeed(kBip84TestMnemonic), false);

    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 0))->address_string,
              "t1fhcesXQLT3U1t7caBYTpd59LiRV8tVfqj");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 1))->address_string,
              "t1WTZNzKCvU2GeM1ZWRyF7EvhMHhr7magiT");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 2))->address_string,
              "t1NPwPhNPHc4S8Xktzq4ygnLLLeuVKgCBZz");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 3))->address_string,
              "t1fe6a9otS98EdnucHLx6cT4am6Ae6THUgj");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 4))->address_string,
              "t1dA52xDndfEo9jJU7DBFSKuxKMqZMMLEGL");

    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 1, 0))->address_string,
              "t1RnTVUMzs1Hi2smgk6EJpVMA2upwTpESYP");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 1, 1))->address_string,
              "t1YRbQiMhwS7mui9r7E2aWGmkLC1xHQnFoc");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 1, 2))->address_string,
              "t1ZnyiWoaS9sceAQKjiK4ughBRQu9z2MXcB");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 1, 3))->address_string,
              "t1WwSQBqGUYf4AXKE6nNR3fp61awb6qsM8z");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 1, 4))->address_string,
              "t1WLKBRDxENW35SLrsW125Yt5tYJDYpBs1i");

    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 0, 0))->address_string,
              "t1LquzEnJVAqdRGeZJZbomrsKwPHuGhNJtm");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 0, 1))->address_string,
              "t1JgGisPzWWN1KMbN82bfKvk9Faa5eKqHUg");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 0, 2))->address_string,
              "t1fmqM2ud5FEgRTLo17QS4c2i575GD8QQCq");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 0, 3))->address_string,
              "t1SoBVfwQvRi7X8RuSxNvkm8NBrbbaUJo5c");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 0, 4))->address_string,
              "t1YuvWycJoxFVvuGHSJEkP5jtfe5VjXyGyj");

    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 1, 0))->address_string,
              "t1NFJ9Jn6su9bGH7mxjowBT6UsPTQdSpVF7");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 1, 1))->address_string,
              "t1SfWeR1cGcxv9G7sBMFP1JubyKBujSFu1C");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 1, 2))->address_string,
              "t1efWNVCp5sDvkRtrJxq6jrdJDbxwsYBxfb");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 1, 3))->address_string,
              "t1g4WDhMPb8e8GUWuV1JEAPa32bgVkeoSuJ");
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(1, 1, 4))->address_string,
              "t1ZTmbmys2n8UjVRb2tfE17A9V6ehZNqZ4W");
  }

  // https://github.com/zcash/zcash-android-wallet-sdk/blob/57f3b7ada6381dcf67b072c84d2f24a6d331045f/sdk-lib/src/androidTest/java/cash/z/ecc/android/sdk/jni/TransparentTest.kt#L49
  {
    ZCashKeyring keyring(*MnemonicToSeed(kDeputyVisaTestMnemonic), false);
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 0))->address_string,
              "t1PKtYdJJHhc3Pxowmznkg7vdTwnhEsCvR4");
  }

  {
    ZCashKeyring keyring(*MnemonicToSeed(kDeputyVisaTestMnemonic), true);
    EXPECT_EQ(keyring.GetAddress(ZCashKeyId(0, 0, 0))->address_string,
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
}

}  // namespace brave_wallet
