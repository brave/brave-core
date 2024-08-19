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

TEST(ZCashUtilsUnitTest, GetOrchardRawBytes) {
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L832
  {
    auto orchard_raw_bytes = GetOrchardRawBytes(
        "u1a84vn0qes8q3jhk7zxs2whd2p922far8kztqdapergs5ej8rarn53v5ddnd6t7e3l5ef"
        "haefrhkptatnzq565nrpvf7kn2787gdvervmk08azp4qgehaew2zplkxkkyu36l3v7drg2"
        "v",
        false);
    ASSERT_TRUE(orchard_raw_bytes);
    constexpr std::array<uint8_t, kOrchardRawBytesSize> expected_raw_bytes = {
        0x3b, 0x68, 0xc2, 0x9b, 0x4a, 0x13, 0x8b, 0x28, 0x9f, 0xea, 0x8b,
        0x67, 0x95, 0xe6, 0x47, 0x59, 0xa7, 0xcd, 0x7c, 0x0a, 0xaf, 0x4b,
        0xb9, 0x8e, 0xd3, 0x07, 0x99, 0x59, 0xb0, 0xbb, 0xa9, 0xb7, 0x61,
        0x70, 0x4b, 0x6c, 0xfc, 0x14, 0x65, 0xad, 0x74, 0xbb, 0x05};
    EXPECT_EQ(orchard_raw_bytes.value(), expected_raw_bytes);
  }

  // Wrong
  {
    auto orchard_raw_bytes = GetOrchardRawBytes("u11", false);
    EXPECT_FALSE(orchard_raw_bytes.has_value());
  }

  // Unknown typecode
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L472
  {
    auto orchard_raw_bytes = GetOrchardRawBytes(
        "u1ln90fvpdtyjapnsqpa2xjsarmhu3k2qvdr6uc6upurnuvzh382jzmfyw40yu8avd2lj7"
        "arvq57n0qmryy0flp7tm0fw05h366587mzzwwrls85da6l2sr7tuazmv5s02avxaxrl4j7"
        "pau0u9xyp470y9hkca5m9g4735208w6957p82lxajzq4l2pqkam86y6jfx8cd8ecw2e05q"
        "nh0qq95dr09sgz9hqmflzac7hsxj47yvjd69ej06ewdg97wsu2x9wg3ahfh6s4nvk65elw"
        "cu5wl092ta38028p4lc2d6l7ea63s6uh4ek0ry9lg50acxuw2sdv02jh90tzh783d59gne"
        "u8ue3wqefjmtndyquwq9kkxaedhtqh2yyjew93ua38vp8uchug0q7kg7qvp4l65t9yqaz2"
        "w2p",
        false);
    EXPECT_FALSE(orchard_raw_bytes);
  }
}

TEST(ZCashUtilsUnitTest, GetTransparentRawBytes) {
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L472
  {
    auto transparent_raw_bytes = GetTransparentRawBytes(
        "u1n9znrl4zyuvds24rcapzglzapqdlax4r8rgkvek0y0xlzfjfvn7zexelrafkchea24w0"
        "30cr9jqsel7t8lvveaq7m7w4z0khmrlzc6748w9ldlccy02scd5xngtcv2yy4ctnyu9zn5"
        "m",
        false);
    EXPECT_EQ(std::vector<uint8_t>({0x65, 0x70, 0x4e, 0x3a, 0xb7, 0x67, 0xca,
                                    0x57, 0x8e, 0x5b, 0x09, 0x2f, 0xb4, 0x76,
                                    0x04, 0xf6, 0x59, 0x47, 0x5b, 0xae}),
              transparent_raw_bytes.value());
  }

  // Unknown typecode
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L400
  {
    auto transparent_raw_bytes = GetTransparentRawBytes(
        "u1ln90fvpdtyjapnsqpa2xjsarmhu3k2qvdr6uc6upurnuvzh382jzmfyw40yu8avd2lj7"
        "arvq57n0qmryy0flp7tm0fw05h366587mzzwwrls85da6l2sr7tuazmv5s02avxaxrl4j7"
        "pau0u9xyp470y9hkca5m9g4735208w6957p82lxajzq4l2pqkam86y6jfx8cd8ecw2e05q"
        "nh0qq95dr09sgz9hqmflzac7hsxj47yvjd69ej06ewdg97wsu2x9wg3ahfh6s4nvk65elw"
        "cu5wl092ta38028p4lc2d6l7ea63s6uh4ek0ry9lg50acxuw2sdv02jh90tzh783d59gne"
        "u8ue3wqefjmtndyquwq9kkxaedhtqh2yyjew93ua38vp8uchug0q7kg7qvp4l65t9yqaz2"
        "w2p",
        false);
    EXPECT_FALSE(transparent_raw_bytes);
  }

  // Wrong
  {
    auto transparent_raw_bytes = GetTransparentRawBytes("u11", false);
    EXPECT_FALSE(transparent_raw_bytes.has_value());
  }
}

TEST(ZCashUtilsUnitTest, OrchardAddress) {
  // All addresses are present
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L84
  {
    std::string addr =
        "u1pg2aaph7jp8rpf6yhsza25722sg5fcn3vaca6ze27hqjw7jvvhhuxkpcg0ge9xh6drsg"
        "dk"
        "da8qjq5chpehkcpxf87rnjryjqwymdheptpvnljqqrjqzjwkc2ma6hcq666kgwfytxwac8"
        "ey"
        "ex6ndgr6ezte66706e3vaqrd25dzvzkc69kw0jgywtd0cmq52q5lkw6uh7hyvzjse8ksx";
    auto orchard_part = ExtractOrchardPart(addr, false);
    auto orchard_raw_bytes = std::array<uint8_t, kOrchardRawBytesSize>(
        {0xce, 0xcb, 0xe5, 0xe6, 0x89, 0xa4, 0x53, 0xa3, 0xfe, 0x10, 0xcc,
         0xf7, 0x61, 0x7e, 0x6c, 0x1f, 0xb3, 0x82, 0x81, 0x9d, 0x7f, 0xc9,
         0x20, 0x0a, 0x1f, 0x42, 0x09, 0x2a, 0xc8, 0x4a, 0x30, 0x37, 0x8f,
         0x8c, 0x1f, 0xb9, 0x0d, 0xff, 0x71, 0xa6, 0xd5, 0x04, 0x2d});
    auto transparent_raw_bytes = std::vector<uint8_t>(
        {0xca, 0xd2, 0x68, 0x75, 0x8c, 0x5e, 0x71, 0x49, 0x30, 0x66,
         0x44, 0x6b, 0x98, 0xe7, 0x1d, 0xf9, 0xd1, 0xd6, 0xa5, 0xca});
    EXPECT_EQ(GetOrchardRawBytes(addr, false), orchard_raw_bytes);
    EXPECT_EQ(GetTransparentRawBytes(addr, false), transparent_raw_bytes);
    EXPECT_EQ(orchard_part.value(),
              GetOrchardUnifiedAddress(orchard_raw_bytes, false).value());
  }

  // Orchard only
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L524
  {
    std::string addr =
        "u1ddnjsdcpm36r6aq79n3s68shjweksnmwtdltrh046s8m6xcws9ygyawalxx8n6hg6veg"
        "k0wh8zjnafxgh6msppjsljvyt0ynece3lvm0";
    auto orchard_part = ExtractOrchardPart(addr, false);
    auto orchard_raw_bytes = std::array<uint8_t, kOrchardRawBytesSize>(
        {0xe3, 0x40, 0x63, 0x65, 0x42, 0xec, 0xe1, 0xc8, 0x12, 0x85, 0xed,
         0x4e, 0xab, 0x44, 0x8a, 0xdb, 0xb5, 0xa8, 0xc0, 0xf4, 0xd3, 0x86,
         0xee, 0xff, 0x33, 0x7e, 0x88, 0xe6, 0x91, 0x5f, 0x6c, 0x3e, 0xc1,
         0xb6, 0xea, 0x83, 0x5a, 0x88, 0xd5, 0x66, 0x12, 0xd2, 0xbd});
    EXPECT_EQ(GetOrchardRawBytes(addr, false), orchard_raw_bytes);
    EXPECT_EQ(orchard_part.value(),
              GetOrchardUnifiedAddress(orchard_raw_bytes, false).value());
    EXPECT_EQ(addr, orchard_part.value());
  }

  // No orchard part
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L454
  {
    std::string addr =
        "u1mtnedjgkz5ln6zzs7nrcyt8mertjundexqdxx52n2x4ww3v52s0akf3qy6sqlze3nexc"
        "jsxtcajglxcdwg47dsrrva6g5t4nf8u3sjchhkmsqghelysrn0cl52c2m8uuv3nyfdv258"
        "jjqnvd4lgqtugc8aqvpmt05c49qv2yqlhxvnq9phdamm4xv89cc7tzvzgmwltxxdsvme44"
        "dgzt8prkcwcsma8cdr76m8n0xwj02tpr9086a237xakkdf8fumsj8u4r6qlf0d59x0mw83"
        "ar36vrcr94zsherapa0566vd22";
    auto orchard_part = ExtractOrchardPart(addr, false);
    EXPECT_FALSE(orchard_part.has_value());
  }

  // Wrong address
  {
    std::string addr =
        "u1ddnjsdcpm36r6aq79n3s68shjweksnmwtdltrh046s8m6xcws9ygyawalxx8n6hg6veg"
        "k0wh8zjnafxgh6msppjsljvyt0ynece3lvm0mm";
    auto orchard_part = ExtractOrchardPart(addr, false);
    EXPECT_FALSE(orchard_part.has_value());
  }
}

TEST(ZCashUtilsUnitTest, GetMergedUnifiedAddress) {
  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L472
  {
    auto addr = GetMergedUnifiedAddress(
        {ParsedAddress(
             ZCashAddrType::kP2PKH,
             std::vector<uint8_t>({0x65, 0x70, 0x4e, 0x3a, 0xb7, 0x67, 0xca,
                                   0x57, 0x8e, 0x5b, 0x09, 0x2f, 0xb4, 0x76,
                                   0x04, 0xf6, 0x59, 0x47, 0x5b, 0xae})),
         ParsedAddress(
             ZCashAddrType::kOrchard,
             std::vector<uint8_t>(
                 {0x5f, 0x09, 0xa9, 0x80, 0x7a, 0x56, 0x32, 0x3b, 0x26,
                  0x3b, 0x05, 0xdf, 0x36, 0x8d, 0xc2, 0x83, 0x91, 0xb2,
                  0x1a, 0x64, 0xa0, 0xe1, 0xb4, 0x0f, 0x9a, 0x68, 0x03,
                  0xb7, 0xe6, 0x8f, 0x39, 0x05, 0x92, 0x3f, 0x35, 0xcb,
                  0x01, 0xf1, 0x19, 0xb2, 0x23, 0xf4, 0x93}))},
        false);
    EXPECT_EQ(addr.value(),
              "u1n9znrl4zyuvds24rcapzglzapqdlax4r8rgkvek0y0xlzfjfvn7zexe"
              "lrafkchea24w030cr9jqsel7t8lvveaq7m7w4z0khmrlzc6748w9ldlcc"
              "y02scd5xngtcv2yy4ctnyu9zn5m");
  }

  // https://github.com/zcash/librustzcash/blob/zcash_address-0.3.1/components/zcash_address/src/kind/unified/address/test_vectors.rs#L540
  {
    auto addr = GetMergedUnifiedAddress(
        {ParsedAddress(
            ZCashAddrType::kOrchard,
            std::vector<uint8_t>(
                {0x3f, 0xad, 0xf8, 0xed, 0xb2, 0x0a, 0x33, 0x01, 0xe8,
                 0x26, 0x0a, 0xa3, 0x11, 0xf4, 0xcb, 0xd5, 0x4d, 0x7d,
                 0x6a, 0x76, 0xba, 0xac, 0x88, 0xc2, 0x44, 0xb0, 0xb1,
                 0x21, 0xc6, 0xdc, 0x22, 0xa8, 0xbc, 0xce, 0x15, 0x89,
                 0x8e, 0x26, 0x78, 0x29, 0xfc, 0x1e, 0x01}))},
        false);
    EXPECT_EQ(addr.value(),
              "u1nztelxna9h7w0vtpd2xjhxt4lpu8s9cmdl8n8vcr7actf2ny45nd07cy8cyuhu"
              "vw3axcp545y0ktq9cezuzx84jyhex8dk4tdvwhu4dl");
  }
}

TEST(ZCashUtilsUnitTest, CalculateZCashTxFee) {
  EXPECT_EQ(25000u, CalculateZCashTxFee(5, 0));
  EXPECT_EQ(25000u, CalculateZCashTxFee(0, 5));
  EXPECT_EQ(10000u, CalculateZCashTxFee(1, 0));
  EXPECT_EQ(10000u, CalculateZCashTxFee(0, 1));
  EXPECT_EQ(50000u, CalculateZCashTxFee(5, 5));
  EXPECT_EQ(10000u, CalculateZCashTxFee(1, 1));
}

TEST(ZCashUtilsUnitTest, OutputZCashAddressSupported) {
  EXPECT_FALSE(OutputZCashAddressSupported(
      "t1Hsc1LR8yKnbbe3twRp88p6vFfC5t7DLbs__", false));
  EXPECT_TRUE(OutputZCashAddressSupported("t1Hsc1LR8yKnbbe3twRp88p6vFfC5t7DLbs",
                                          false));
  EXPECT_FALSE(
      OutputZCashAddressSupported("t1Hsc1LR8yKnbbe3twRp88p6vFfC5t7DLbs", true));
  EXPECT_TRUE(
      OutputZCashAddressSupported("tm9iMLAuYMzJ6jtFLcA7rzUmfreGuKvr7Ma", true));
  EXPECT_FALSE(OutputZCashAddressSupported(
      "tm9iMLAuYMzJ6jtFLcA7rzUmfreGuKvr7Ma", false));
}

TEST(ZCashUtilsUnitTest, RevertHex) {
  EXPECT_FALSE(RevertHex("0x"));
  EXPECT_FALSE(RevertHex("0xYY"));
  EXPECT_FALSE(RevertHex("0xa"));
  EXPECT_FALSE(RevertHex(""));
  EXPECT_EQ(
      RevertHex(
          "000000000061ef6a26dcf7597a3ffddd087c23df5f44398d070dbd26a9699ae9")
          .value(),
      "0xe99a69a926bd0d078d39445fdf237c08ddfd3f7a59f7dc266aef610000000000");
  EXPECT_EQ(
      RevertHex(
          "000000000049900203ce1cba81a36d29390ea40fc78cf4799e8139b96f3a8114")
          .value(),
      "0x14813a6fb939819e79f48cc70fa40e39296da381ba1cce030290490000000000");
}

}  // namespace brave_wallet
