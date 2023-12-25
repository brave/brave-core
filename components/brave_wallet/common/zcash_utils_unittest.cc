/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/zcash_utils.h"

#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(ZCashUtilsUnitTest, PubkeyToTransparentAddress) {
  EXPECT_EQ("t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS",
            PubkeyToTransparentAddress(
                PrefixedHexStringToBytes("0x0248e93db0ad3e748f02688fdbbad77aafd"
                                         "f0cf2dfb9c7d6db42808cc7e5603d9a")
                    .value(),
                false));

  EXPECT_EQ("t1PycYmkim4L4PQyipovyHyAmiCS8z7Xwxq",
            PubkeyToTransparentAddress(
                PrefixedHexStringToBytes("0x035df6f58fd110b989eaa28659ea5c4cab3"
                                         "3545d313061f713a9d726c24eb83430")
                    .value(),
                false));

  EXPECT_EQ("t1WU75sSfiPbK5ez33uuhEbd9ZD3XNCxMRj",
            PubkeyToTransparentAddress(
                PrefixedHexStringToBytes("0x02c45d998fedfdad735c064f860d99c79b8"
                                         "0620c04d7d3dbf0307c7cfe24567f84")
                    .value(),
                false));

  EXPECT_EQ("t1g2srkA6K4M6oVAaeuJdZMi4JRYev25G1J",
            PubkeyToTransparentAddress(
                PrefixedHexStringToBytes("0x03b3d315d9599c38d91265ac67c60200dc5"
                                         "d87878217c6007c74d18459f43c64a2")
                    .value(),
                false));

  // https://github.com/zcash/zcash-android-wallet-sdk/blob/57f3b7ada6381dcf67b072c84d2f24a6d331045f/sdk-lib/src/androidTest/java/cash/z/ecc/android/sdk/jni/TransparentTest.kt#L49
  EXPECT_EQ("t1PKtYdJJHhc3Pxowmznkg7vdTwnhEsCvR4",
            PubkeyToTransparentAddress(
                PrefixedHexStringToBytes("0x03b1d7fb28d17c125b504d06b1530097e0a"
                                         "3c76ada184237e3bc0925041230a5af")
                    .value(),
                false));
}

TEST(ZCashUtilsUnitTest, ExtractTransparentPart) {
  auto transparent_past = ExtractTransparentPart(
      "u14nl3xm252a24u4tr9ml453ptz4uv4lam2ytau94nmnf9sf3cx5vdrh9ksf3f24d7jk8l35"
      "le9ntldzh3zl0xusyqlhs2t6j55xsrngulcv2p0del8ecrv9s0xyy3uawkh8clsquj7c89s9"
      "r9thxhvsd3h0eqcd3lj5grmnj3uf9l8u07s8n28wece35y0qvzj8q6sxa32fndc86h8dt",
      false);
  EXPECT_EQ("t1LdJpgcni2jYpkadiPtBGYv5maCD5Wu8zq", transparent_past);
}

}  // namespace brave_wallet
