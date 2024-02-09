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

TEST(ZCashUtilsUnitTest, PubkeyHashToTransparentAddress) {
  EXPECT_FALSE(
      PubkeyHashToTransparentAddress(std::vector<uint8_t>(19, 0), false));
  EXPECT_FALSE(
      PubkeyHashToTransparentAddress(std::vector<uint8_t>(21, 0), false));
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/encoding.rs#L243
  EXPECT_EQ("t1Hsc1LR8yKnbbe3twRp88p6vFfC5t7DLbs",
            PubkeyHashToTransparentAddress(std::vector<uint8_t>(20, 0), false)
                .value());
}

TEST(ZCashUtilsUnitTest, ExtractTransparentPart) {
  // https://github.com/Electric-Coin-Company/zcash-android-wallet-sdk/blob/v2.0.6/sdk-incubator-lib/src/main/java/cash/z/ecc/android/sdk/fixture/WalletFixture.kt
  {
    auto transparent_part = ExtractTransparentPart(
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "0",
        false);
    EXPECT_EQ("t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ", transparent_part);
  }
  {
    auto transparent_part = ExtractTransparentPart(
        "u1czzc8jcl50svfezmfc9xsxnh63p374nptqplt0yw2uekr7v9wprp84y6esys6derp6uv"
        "dcq6x6ykjrkpdyhjzneq5ud78h6j68n63hewg7xp9fpneuh64wgzt3d7mh6zh3qpqapzlc"
        "4",
        false);
    EXPECT_EQ("t1duiEGg7b39nfQee3XaTY4f5McqfyJKhBi", transparent_part);
  }
  {
    auto transparent_part = ExtractTransparentPart(
        "utest16zd8zfx6n6few7mjsjpn6qtn8tlg6law7qnq33257855mdqekk7vru8lettx3vud"
        "4mh99elglddltmfjkduar69h7vy08h3xdq6zuls9pqq7quyuehjqwtthc3hfd8gshhw42d"
        "fr96e",
        true);
    EXPECT_EQ("tmCxJG72RWN66xwPtNgu4iKHpyysGrc7rEg", transparent_part);
  }

  // https://github.com/zcash/librustzcash/blob/zcash_primitives-0.13.0/components/zcash_address/src/kind/unified/address/test_vectors.rs#L17
  {
    auto transparent_part = ExtractTransparentPart(
        "u1l8xunezsvhq8fgzfl7404m450nwnd76zshscn6nfys7vyz2ywyh4cc5daaq0c7q2su5l"
        "qfh23sp7fkf3kt27ve5948mzpfdvckzaect2jtte308mkwlycj2u0eac077wu70vqcetkx"
        "f",
        false);
    EXPECT_EQ(
        PubkeyHashToTransparentAddress(
            std::vector<uint8_t>({0x7b, 0xb8, 0x35, 0x70, 0xb8, 0xfa, 0xe1,
                                  0x46, 0xe0, 0x3c, 0x53, 0x31, 0xa0, 0x20,
                                  0xb1, 0xe0, 0x89, 0x2f, 0x63, 0x1d}),
            false),
        transparent_part);
  }

  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L981
  {
    auto transparent_part = ExtractTransparentPart(
        "u1ap7zakdnuefrgdglr334cw62hnqjkhr65t7tketyym0amkhdvyedpucuyxwu9z2te5vp"
        "0jf75jgsm36d7r09h6z3qe5rkgd8y28er6fz8z5rckspevxnx4y9wfk49njpcujh5gle7m"
        "fan90m9tt9a2gltyh8hx27cwt7h6u8ndmzhtk8qrq8hjytnakjqm0n658llh4z0277cyl2"
        "rcu",
        false);
    EXPECT_EQ(
        PubkeyHashToTransparentAddress(
            std::vector<uint8_t>({0xf1, 0xbc, 0x3d, 0x72, 0x61, 0xbf, 0x77,
                                  0xfe, 0x80, 0x8e, 0x2b, 0x71, 0x78, 0x98,
                                  0x1c, 0x7c, 0xfe, 0x55, 0x70, 0xfd}),
            false),
        transparent_part);
  }

  // Multily transparent addresses
  {
    auto transparent_part = ExtractTransparentPart(
        "u1gfg995k4rre49al7x6m2u0t6rfhyjaecw9duhsmn4f7mdqeavfc3crny504e69yers2e"
        "7fzy8fwet0r0pt4wkdfs794ycqrvhe2hc97tkevpjr8rh3uenj3kz3rdqy78hmsmsx69dw"
        "raxwgy42xuhjh249uckn2elfwwhg36t7f9ms",
        false);
    EXPECT_EQ(PubkeyHashToTransparentAddress(
                  std::vector<uint8_t>({0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
                  false),
              transparent_part);
  }

  // No transparent addresses
  // https://github.com/zcash/librustzcash/blob/zcash_primitives-0.13.0/components/zcash_address/src/kind/unified/address/test_vectors.rs#L149
  {
    EXPECT_FALSE(
        ExtractTransparentPart(
            "u19a4vmx7ysmtavmnaz4d2dgl9pyshexw35rl5ezg5dkkxktg08p42lng7kf9hqtn2"
            "fhr63qzyhe8gtnvgtfl9yvne46x6zfzwgedx7c0chnrxty0k5r5qqph8k02zs8e3ke"
            "ul9vj8myju7rvqgjaysa9kt0fucxpzuky6kf0pjgy0a6hx",
            false)
            .has_value());
  }

  // Testnet
  {
    auto transparent_part = ExtractTransparentPart(
        "utest190jaxge023jmrqktsnae6et4pm7pmkyezzt8r674sd6a3seyrmxfry2spspl72aa"
        "na7kx9nfn0637np9k6tagzss48l6u9kcjf6gadlcnfusm42klsmmxnwj80q40cfwe8dnj7"
        "373w0",
        true);
    EXPECT_EQ(PubkeyHashToTransparentAddress(
                  std::vector<uint8_t>({0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
                  true),
              transparent_part);
  }

  // Wrong unified address
  {
    EXPECT_FALSE(ExtractTransparentPart("u1xxx", false).has_value());
    EXPECT_FALSE(ExtractTransparentPart("u0000", false).has_value());
    EXPECT_FALSE(ExtractTransparentPart("", false).has_value());
  }
}

}  // namespace brave_wallet
