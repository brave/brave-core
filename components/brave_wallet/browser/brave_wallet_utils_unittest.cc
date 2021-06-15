/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletUtilsUnitTest, ToHex) {
  ASSERT_EQ(ToHex(""), "0x0");
  ASSERT_EQ(ToHex("hello world"), "0x68656c6c6f20776f726c64");
}

TEST(BraveWalletUtilsUnitTest, KeccakHash) {
  ASSERT_EQ(
      KeccakHash(""),
      "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  ASSERT_EQ(
      KeccakHash("hello world"),
      "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");
}

TEST(BraveWalletUtilsUnitTest, GetFunctionHash) {
  ASSERT_EQ(GetFunctionHash("approve(address,uint256)"), "0x095ea7b3");
  ASSERT_EQ(GetFunctionHash("balanceOf(address)"), "0x70a08231");
}

TEST(BraveWalletUtilsUnitTest, PadHexEncodedParameter) {
  std::string out;
  // Pad an address
  ASSERT_TRUE(PadHexEncodedParameter(
      "0x4e02f254184E904300e0775E4b8eeCB14a1b29f0", &out));
  ASSERT_EQ(
      out,
      "0x0000000000000000000000004e02f254184E904300e0775E4b8eeCB14a1b29f0");

  // Corner case: 62
  ASSERT_TRUE(PadHexEncodedParameter(
      "0x11111111112222222222333333333344444444445555555555666666666600",
      &out));
  ASSERT_EQ(
      out,
      "0x0011111111112222222222333333333344444444445555555555666666666600");

  ASSERT_TRUE(PadHexEncodedParameter("0x0", &out));
  ASSERT_EQ(
      out,
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  // Invalid input
  ASSERT_FALSE(PadHexEncodedParameter("0x", &out));
  ASSERT_FALSE(PadHexEncodedParameter("0", &out));
  ASSERT_FALSE(PadHexEncodedParameter("", &out));
}

TEST(BraveWalletUtilsUnitTest, IsValidHexString) {
  ASSERT_TRUE(IsValidHexString("0x0"));
  ASSERT_TRUE(IsValidHexString("0x4e02f254184E904300e0775E4b8eeCB14a1b29f0"));
  ASSERT_FALSE(IsValidHexString("0x"));
  ASSERT_FALSE(IsValidHexString("0xZ"));
  ASSERT_FALSE(IsValidHexString("123"));
  ASSERT_FALSE(IsValidHexString("0"));
  ASSERT_FALSE(IsValidHexString(""));
  ASSERT_FALSE(IsValidHexString("0xBraVe"));
  ASSERT_FALSE(IsValidHexString("0x12$$"));
}

TEST(BraveWalletUtilsUnitTest, ConcatHexStrings) {
  std::string out;
  // Pad an address
  ASSERT_TRUE(ConcatHexStrings(
      "0x70a08231",
      "0x0000000000000000000000004e02f254184E904300e0775E4b8eeCB14a1b29f0",
      &out));
  ASSERT_EQ(out,
            "0x70a082310000000000000000000000004e02f254184E904300e0775E4b8eeCB1"
            "4a1b29f0");
  ASSERT_TRUE(ConcatHexStrings("0x0", "0x0", &out));
  ASSERT_EQ(out, "0x00");
  // Invalid input
  ASSERT_FALSE(ConcatHexStrings("0x", "0x0", &out));
  ASSERT_FALSE(ConcatHexStrings("0x0", "0", &out));
}

TEST(BraveWalletUtilsUnitTest, HexValueToUint256) {
  uint256_t out;
  ASSERT_TRUE(HexValueToUint256("0x1", &out));
  ASSERT_EQ(out, (uint256_t)1);
  ASSERT_TRUE(HexValueToUint256("0x1234", &out));
  ASSERT_EQ(out, (uint256_t)4660);
  ASSERT_TRUE(HexValueToUint256("0xB", &out));
  ASSERT_EQ(out, (uint256_t)11);
  uint256_t expected_val = 102400000000000;
  // "10240000000000000000000000"
  expected_val *= static_cast<uint256_t>(100000000000);
  ASSERT_TRUE(HexValueToUint256("0x878678326eac900000000", &out));
  ASSERT_TRUE(out == (uint256_t)expected_val);
  // Check padded values too
  ASSERT_TRUE(HexValueToUint256("0x00000000000000000000000F0", &out));
  ASSERT_EQ(out, (uint256_t)240);
}

TEST(BraveWalletUtilsUnitTest, Uint256ValueToHex) {
  ASSERT_EQ(Uint256ValueToHex(1), "0x1");
  ASSERT_EQ(Uint256ValueToHex(4660), "0x1234");
  ASSERT_EQ(Uint256ValueToHex(11), "0xb");
  // "10240000000000000000000000"
  uint256_t input_val = 102400000000000;
  input_val *= static_cast<uint256_t>(100000000000);
  ASSERT_EQ(Uint256ValueToHex(input_val), "0x878678326eac900000000");
  ASSERT_EQ(Uint256ValueToHex(3735928559), "0xdeadbeef");
}

TEST(BraveWalletUtilsUnitTest, Mnemonic) {
  const struct {
    const char* entropy;
    const char* mnemonic;
    const char* seed;
  } cases[] = {
      {"00000000000000000000000000000000",
       "abandon abandon abandon abandon abandon abandon abandon abandon "
       "abandon "
       "abandon abandon about",
       "c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531f09a69"
       "87"
       "599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04"},
      {"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
       "legal winner thank year wave sausage worth useful legal winner thank "
       "yellow",
       "2e8905819b8723fe2c1d161860e5ee1830318dbf49a83bd451cfb8440c28bd6fa457fe1"
       "29"
       "6106559a3c80937a1c1069be3a3a5bd381ee6260e8d9739fce1f607"},
      {"80808080808080808080808080808080",
       "letter advice cage absurd amount doctor acoustic avoid letter advice "
       "cage above",
       "d71de856f81a8acc65e6fc851a38d4d7ec216fd0796d0a6827a3ad6ed5511a30fa280f1"
       "2eb2e47ed2ac03b5c462a0358d18d69fe4f985ec81778c1b370b652a8"},
      {"ffffffffffffffffffffffffffffffff",
       "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
       "ac27495480225222079d7be181583751e86f571027b0497b5b5d11218e0a8a133325729"
       "17f0f8e5a589620c6f15b11c61dee327651a14c34e18231052e48c069"},
      {"000000000000000000000000000000000000000000000000",
       "abandon abandon abandon abandon abandon abandon abandon abandon "
       "abandon abandon abandon abandon abandon abandon abandon abandon "
       "abandon agent",
       "035895f2f481b1b0f01fcf8c289c794660b289981a78f8106447707fdd9666ca06da5a9"
       "a565181599b79f53b844d8a71dd9f439c52a3d7b3e8a79c906ac845fa"},
      {"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
       "legal winner thank year wave sausage worth useful legal winner thank "
       "year wave sausage worth useful legal will",
       "f2b94508732bcbacbcc020faefecfc89feafa6649a5491b8c952cede496c214a0c7b3c3"
       "92d168748f2d4a612bada0753b52a1c7ac53c1e93abd5c6320b9e95dd"},
      {"808080808080808080808080808080808080808080808080",
       "letter advice cage absurd amount doctor acoustic avoid letter advice "
       "cage absurd amount doctor acoustic avoid letter always",
       "107d7c02a5aa6f38c58083ff74f04c607c2d2c0ecc55501dadd72d025b751bc27fe913f"
       "fb796f841c49b1d33b610cf0e91d3aa239027f5e99fe4ce9e5088cd65"},
      {"ffffffffffffffffffffffffffffffffffffffffffffffff",
       "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo "
       "when",
       "0cd6e5d827bb62eb8fc1e262254223817fd068a74b5b449cc2f667c3f1f985a76379b43"
       "348d952e2265b4cd129090758b3e3c2c49103b5051aac2eaeb890a528"},
      {"0000000000000000000000000000000000000000000000000000000000000000",
       "abandon abandon abandon abandon abandon abandon abandon abandon "
       "abandon abandon abandon abandon abandon abandon abandon abandon "
       "abandon abandon abandon abandon abandon abandon abandon art",
       "bda85446c68413707090a52022edd26a1c9462295029f2e60cd7c4f2bbd3097170af7a4"
       "d73245cafa9c3cca8d561a7c3de6f5d4a10be8ed2a5e608d68f92fcc8"},
      {"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
       "legal winner thank year wave sausage worth useful legal winner thank "
       "year wave sausage worth useful legal winner thank year wave sausage "
       "worth title",
       "bc09fca1804f7e69da93c2f2028eb238c227f2e9dda30cd63699232578480a4021b146a"
       "d717fbb7e451ce9eb835f43620bf5c514db0f8add49f5d121449d3e87"},
      {"8080808080808080808080808080808080808080808080808080808080808080",
       "letter advice cage absurd amount doctor acoustic avoid letter advice "
       "cage absurd amount doctor acoustic avoid letter advice cage absurd "
       "amount doctor acoustic bless",
       "c0c519bd0e91a2ed54357d9d1ebef6f5af218a153624cf4f2da911a0ed8f7a09e2ef61a"
       "f0aca007096df430022f7a2b6fb91661a9589097069720d015e4e982f"},
      {"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
       "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo "
       "zoo zoo zoo zoo zoo zoo vote",
       "dd48c104698c30cfe2b6142103248622fb7bb0ff692eebb00089b32d22484e1613912f0"
       "a5b694407be899ffd31ed3992c456cdf60f5d4564b8ba3f05a69890ad"},
      {"77c2b00716cec7213839159e404db50d",
       "jelly better achieve collect unaware mountain thought cargo oxygen act "
       "hood bridge",
       "b5b6d0127db1a9d2226af0c3346031d77af31e918dba64287a1b44b8ebf63cdd52676f6"
       "72a290aae502472cf2d602c051f3e6f18055e84e4c43897fc4e51a6ff"},
      {"b63a9c59a6e641f288ebc103017f1da9f8290b3da6bdef7b",
       "renew stay biology evidence goat welcome casual join adapt armor "
       "shuffle fault little machine walk stumble urge swap",
       "9248d83e06f4cd98debf5b6f010542760df925ce46cf38a1bdb4e4de7d21f5c39366941"
       "c69e1bdbf2966e0f6e6dbece898a0e2f0a4c2b3e640953dfe8b7bbdc5"},
      {"3e141609b97933b66a060dcddc71fad1d91677db872031e85f4c015c5e7e8982",
       "dignity pass list indicate nasty swamp pool script soccer toe leaf "
       "photo multiply desk host tomato cradle drill spread actor shine "
       "dismiss champion exotic",
       "ff7f3184df8696d8bef94b6c03114dbee0ef89ff938712301d27ed8336ca89ef9635da2"
       "0af07d4175f2bf5f3de130f39c9d9e8dd0472489c19b1a020a940da67"},
      {"0460ef47585604c5660618db2e6a7e7f",
       "afford alter spike radar gate glance object seek swamp infant panel "
       "yellow",
       "65f93a9f36b6c85cbe634ffc1f99f2b82cbb10b31edc7f087b4f6cb9e976e9faf76ff41"
       "f8f27c99afdf38f7a303ba1136ee48a4c1e7fcd3dba7aa876113a36e4"},
      {"72f60ebac5dd8add8d2a25a797102c3ce21bc029c200076f",
       "indicate race push merry suffer human cruise dwarf pole review arch "
       "keep canvas theme poem divorce alter left",
       "3bbf9daa0dfad8229786ace5ddb4e00fa98a044ae4c4975ffd5e094dba9e0bb289349db"
       "e2091761f30f382d4e35c4a670ee8ab50758d2c55881be69e327117ba"},
      {"2c85efc7f24ee4573d2b81a6ec66cee209b2dcbd09d8eddc51e0215b0b68e416",
       "clutch control vehicle tonight unusual clog visa ice plunge glimpse "
       "recipe series open hour vintage deposit universe tip job dress radar "
       "refuse motion taste",
       "fe908f96f46668b2d5b37d82f558c77ed0d69dd0e7e043a5b0511c48c2f1064694a956f"
       "86360c93dd04052a8899497ce9e985ebe0c8c52b955e6ae86d4ff4449"},
      {"eaebabb2383351fd31d703840b32e9e2",
       "turtle front uncle idea crush write shrug there lottery flower risk "
       "shell",
       "bdfb76a0759f301b0b899a1e3985227e53b3f51e67e3f2a65363caedf3e32fde42a66c4"
       "04f18d7b05818c95ef3ca1e5146646856c461c073169467511680876c"},
      {"7ac45cfe7722ee6c7ba84fbc2d5bd61b45cb2fe5eb65aa78",
       "kiss carry display unusual confirm curtain upgrade antique rotate "
       "hello void custom frequent obey nut hole price segment",
       "ed56ff6c833c07982eb7119a8f48fd363c4a9b1601cd2de736b01045c5eb8ab4f57b079"
       "403485d1c4924f0790dc10a971763337cb9f9c62226f64fff26397c79"},
      {"4fa1a8bc3e6d80ee1316050e862c1812031493212b7ec3f3bb1b08f168cabeef",
       "exile ask congress lamp submit jacket era scheme attend cousin alcohol "
       "catch course end lucky hurt sentence oven short ball bird grab wing "
       "top",
       "095ee6f817b4c2cb30a5a797360a81a40ab0f9a4e25ecd672a3f58a0b5ba0687c096a6b"
       "14d2c0deb3bdefce4f61d01ae07417d502429352e27695163f7447a8c"},
      {"18ab19a9f54a9274f03e5209a2ac8a91",
       "board flee heavy tunnel powder denial science ski answer betray cargo "
       "cat",
       "6eff1bb21562918509c73cb990260db07c0ce34ff0e3cc4a8cb3276129fbcb300bddfe0"
       "05831350efd633909f476c45c88253276d9fd0df6ef48609e8bb7dca8"},
      {"18a2e1d81b8ecfb2a333adcb0c17a5b9eb76cc5d05db91a4",
       "board blade invite damage undo sun mimic interest slam gaze truly "
       "inherit resist great inject rocket museum chief",
       "f84521c777a13b61564234bf8f8b62b3afce27fc4062b51bb5e62bdfecb23864ee6ecf0"
       "7c1d5a97c0834307c5c852d8ceb88e7c97923c0a3b496bedd4e5f88a9"},
      {"15da872c95a13dd738fbf50e427583ad61f18fd99f628c417a61cf8343c90419",
       "beyond stage sleep clip because twist token leaf atom beauty genius "
       "food business side grid unable middle armed observe pair crouch "
       "tonight away coconut",
       "b15509eaa2d09d3efd3e006ef42151b30367dc6e3aa5e44caba3fe4d3e352e65101fbdb"
       "86a96776b91946ff06f8eac594dc6ee1d3e82a42dfe1b40fef6bcc3fd"},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    std::vector<uint8_t> bytes;
    EXPECT_TRUE(base::HexStringToBytes(cases[i].entropy, &bytes));

    EXPECT_EQ(GenerateMnemonicForTest(bytes), cases[i].mnemonic);
    std::unique_ptr<std::vector<uint8_t>> seed =
        MnemonicToSeed(cases[i].mnemonic, "TREZOR");
    EXPECT_EQ(base::ToLowerASCII(base::HexEncode(*seed)), cases[i].seed);
  }

  for (size_t i = 15; i <= 33; i += 2) {
    EXPECT_EQ(GenerateMnemonic(i), "");
  }
  for (size_t i = 16; i <= 32; i += 4) {
    std::string result = GenerateMnemonic(i);
    EXPECT_NE(result, "");
    std::istringstream buf(result);
    std::istream_iterator<std::string> begin(buf), end;
    std::vector<std::string> words(begin, end);
    // words count should be 12, 15, 18, 21, 24
    EXPECT_EQ(words.size(), (i / 4) * 3);
    // Random generated entropy
    EXPECT_NE(GenerateMnemonic(i), GenerateMnemonic(i));
  }
}

TEST(BraveWalletUtilsUnitTest, MnemonicToSeed) {
  EXPECT_NE(MnemonicToSeed("kingdom possible coast island six arrow fluid "
                           "spell chunk loud glue street",
                           ""),
            nullptr);
  EXPECT_EQ(MnemonicToSeed("lingdom possible coast island six arrow fluid "
                           "spell chunk loud glue street",
                           ""),
            nullptr);
  EXPECT_EQ(
      MnemonicToSeed(
          "kingdom possible coast island six arrow fluid spell chunk loud glue",
          ""),
      nullptr);
  EXPECT_EQ(MnemonicToSeed("", ""), nullptr);
}

TEST(BraveWalletUtilsUnitTest, EncodeString) {
  std::string output;
  EXPECT_TRUE(EncodeString("one", &output));
  EXPECT_EQ(output,
            // Count for input string.
            "0x0000000000000000000000000000000000000000000000000000000000000003"
            // Encoding for input string.
            "6f6e650000000000000000000000000000000000000000000000000000000000");

  output.clear();
  EXPECT_TRUE(EncodeString(
      "oneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneon"
      "e",
      &output));
  EXPECT_EQ(
      output,
      // Count for input string.
      "0x0000000000000000000000000000000000000000000000000000000000000048"
      // Encoding for input string.
      "6f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e"
      "656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f"
      "6e65000000000000000000000000000000000000000000000000");

  output.clear();
  EXPECT_TRUE(EncodeString("", &output));
  EXPECT_EQ(
      output,
      "0x0000000000000000000000000000000000000000000000000000000000000000");

  output.clear();
  std::string invalid_input = "\xF0\x8F\xBF\xBE";
  EXPECT_FALSE(base::IsStringUTF8(invalid_input));
  EXPECT_FALSE(EncodeString(invalid_input, &output));
}

TEST(BraveWalletUtilsUnitTest, EncodeStringArray) {
  std::vector<std::string> input({"one", "two", "three"});
  std::string output;
  EXPECT_TRUE(EncodeStringArray(input, &output));
  EXPECT_EQ(output,
            // count of elements in input array
            "0x0000000000000000000000000000000000000000000000000000000000000003"
            // offsets to array elements
            "0000000000000000000000000000000000000000000000000000000000000060"
            "00000000000000000000000000000000000000000000000000000000000000a0"
            "00000000000000000000000000000000000000000000000000000000000000e0"
            // count for "one"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "one"
            "6f6e650000000000000000000000000000000000000000000000000000000000"
            // count for "two"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "two"
            "74776f0000000000000000000000000000000000000000000000000000000000"
            // count for "three"
            "0000000000000000000000000000000000000000000000000000000000000005"
            // encoding for "three"
            "7468726565000000000000000000000000000000000000000000000000000000");

  input = {"one", "one two three four five six seven eight nine", "two",
           "one two three four five six seven eight nine ten", "three"};
  output.clear();
  EXPECT_TRUE(EncodeStringArray(input, &output));

  EXPECT_EQ(output,
            // count of elements in input array
            "0x0000000000000000000000000000000000000000000000000000000000000005"
            // offsets to array elements
            "00000000000000000000000000000000000000000000000000000000000000a0"
            "00000000000000000000000000000000000000000000000000000000000000e0"
            "0000000000000000000000000000000000000000000000000000000000000140"
            "0000000000000000000000000000000000000000000000000000000000000180"
            "00000000000000000000000000000000000000000000000000000000000001e0"
            // count for "one"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "one"
            "6f6e650000000000000000000000000000000000000000000000000000000000"
            // count for "one two three four five six seven eight nine"
            "000000000000000000000000000000000000000000000000000000000000002c"
            // encoding for "one two three four five six seven eight nine"
            "6f6e652074776f20746872656520666f75722066697665207369782073657665"
            "6e206569676874206e696e650000000000000000000000000000000000000000"
            // count for "two"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "two"
            "74776f0000000000000000000000000000000000000000000000000000000000"
            // count for "one two three four five six seven eight nine ten"
            "0000000000000000000000000000000000000000000000000000000000000030"
            // encoding for "one two three four five six seven eight nine ten"
            "6f6e652074776f20746872656520666f75722066697665207369782073657665"
            "6e206569676874206e696e652074656e00000000000000000000000000000000"
            // count for "three"
            "0000000000000000000000000000000000000000000000000000000000000005"
            // encoding for "three"
            "7468726565000000000000000000000000000000000000000000000000000000");

  input = {"", "one", "", "two", "", "three"};
  output.clear();
  EXPECT_TRUE(EncodeStringArray(input, &output));

  EXPECT_EQ(output,
            "0x0000000000000000000000000000000000000000000000000000000000000006"
            // offsets to array elements
            "00000000000000000000000000000000000000000000000000000000000000c0"
            "00000000000000000000000000000000000000000000000000000000000000e0"
            "0000000000000000000000000000000000000000000000000000000000000120"
            "0000000000000000000000000000000000000000000000000000000000000140"
            "0000000000000000000000000000000000000000000000000000000000000180"
            "00000000000000000000000000000000000000000000000000000000000001a0"
            // count for ""
            "0000000000000000000000000000000000000000000000000000000000000000"
            // count for "one"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "one"
            "6f6e650000000000000000000000000000000000000000000000000000000000"
            // count for ""
            "0000000000000000000000000000000000000000000000000000000000000000"
            // count for "two"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "two"
            "74776f0000000000000000000000000000000000000000000000000000000000"
            // count for ""
            "0000000000000000000000000000000000000000000000000000000000000000"
            // count for "three"
            "0000000000000000000000000000000000000000000000000000000000000005"
            // encoding for "three"
            "7468726565000000000000000000000000000000000000000000000000000000");

  input = {"one", "\xF0\x8F\xBF\xBE"};
  output.clear();
  EXPECT_FALSE(EncodeStringArray(input, &output));
}

TEST(BraveWalletUtilsUnitTest, DecodeString) {
  std::string output;
  EXPECT_TRUE(DecodeString(
      0,
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000",
      &output));
  EXPECT_EQ(output, "one");

  output.clear();
  EXPECT_TRUE(DecodeString(
      0,
      // count for "one two three four five six seven eight nine"
      "000000000000000000000000000000000000000000000000000000000000002c"
      // encoding for "one two three four five six seven eight nine"
      "6f6e652074776f20746872656520666f75722066697665207369782073657665"
      "6e206569676874206e696e650000000000000000000000000000000000000000",
      &output));
  EXPECT_EQ(output, "one two three four five six seven eight nine");

  output.clear();
  EXPECT_TRUE(DecodeString(
      0,
      // count for ""
      "0000000000000000000000000000000000000000000000000000000000000000",
      &output));
  EXPECT_EQ(output, "");

  // Test invalid inputs.
  output.clear();
  EXPECT_FALSE(DecodeString(0, "", &output));
  EXPECT_FALSE(DecodeString(0, "invalid string", &output));
  EXPECT_FALSE(DecodeString(
      0,
      // invalid count
      "6f6e650000000000000000000000000000000000000000000000000000000000",
      &output));

  EXPECT_FALSE(DecodeString(
      0,
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // invalid encoding for "one": len < expected len of encoding for "one"
      "6f6e",
      &output));

  EXPECT_FALSE(DecodeString(
      0,
      // count for "one" without encoding of string
      "0000000000000000000000000000000000000000000000000000000000000003",
      &output));

  EXPECT_FALSE(DecodeString(
      64,  // out-of-bound offset
      "0000000000000000000000000000000000000000000000000000000000000001",
      &output));

  EXPECT_FALSE(DecodeString(
      999999,  // out-of-bound invalid offset
      // count for "one two three four five six seven eight nine"
      "000000000000000000000000000000000000000000000000000000000000002c"
      // encoding for "one two three four five six seven eight nine"
      "6f6e652074776f20746872656520666f75722066697665207369782073657665"
      "6e206569676874206e696e650000000000000000000000000000000000000000",
      &output));
}

TEST(BraveWalletUtilsUnitTest, DecodeStringArray) {
  std::vector<std::string> output;
  EXPECT_TRUE(DecodeStringArray(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000003"
      // offsets to array elements
      "0000000000000000000000000000000000000000000000000000000000000060"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000"
      // count for "three"
      "0000000000000000000000000000000000000000000000000000000000000005"
      // encoding for "three"
      "7468726565000000000000000000000000000000000000000000000000000000",
      &output));
  std::vector<std::string> expected_output({"one", "two", "three"});
  EXPECT_EQ(output, expected_output);

  output.clear();
  EXPECT_TRUE(DecodeStringArray(
      "0000000000000000000000000000000000000000000000000000000000000005"
      // offsets to array elements
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      "0000000000000000000000000000000000000000000000000000000000000140"
      "0000000000000000000000000000000000000000000000000000000000000180"
      "00000000000000000000000000000000000000000000000000000000000001e0"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for "one two three four five six seven eight nine"
      "000000000000000000000000000000000000000000000000000000000000002c"
      // encoding for "one two three four five six seven eight nine"
      "6f6e652074776f20746872656520666f75722066697665207369782073657665"
      "6e206569676874206e696e650000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000"
      // count for "one two three four five six seven eight nine ten"
      "0000000000000000000000000000000000000000000000000000000000000030"
      // encoding for "one two three four five six seven eight nine ten"
      "6f6e652074776f20746872656520666f75722066697665207369782073657665"
      "6e206569676874206e696e652074656e00000000000000000000000000000000"
      // count for "three"
      "0000000000000000000000000000000000000000000000000000000000000005"
      // encoding for "three"
      "7468726565000000000000000000000000000000000000000000000000000000",
      &output));
  expected_output = {"one", "one two three four five six seven eight nine",
                     "two", "one two three four five six seven eight nine ten",
                     "three"};
  EXPECT_EQ(output, expected_output);

  output.clear();
  EXPECT_TRUE(DecodeStringArray(
      "0000000000000000000000000000000000000000000000000000000000000006"
      // offsets to array elements
      "00000000000000000000000000000000000000000000000000000000000000c0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000140"
      "0000000000000000000000000000000000000000000000000000000000000180"
      "00000000000000000000000000000000000000000000000000000000000001a0"
      // count for ""
      "0000000000000000000000000000000000000000000000000000000000000000"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for ""
      "0000000000000000000000000000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000"
      // count for ""
      "0000000000000000000000000000000000000000000000000000000000000000"
      // count for "three"
      "0000000000000000000000000000000000000000000000000000000000000005"
      // encoding for "three"
      "7468726565000000000000000000000000000000000000000000000000000000",
      &output));
  expected_output = {"", "one", "", "two", "", "three"};
  EXPECT_EQ(output, expected_output);

  // Test invalid input.
  output.clear();
  EXPECT_FALSE(DecodeStringArray("", &output));
  EXPECT_FALSE(DecodeStringArray("1", &output));
  EXPECT_FALSE(DecodeStringArray("z", &output));
  EXPECT_FALSE(DecodeStringArray("\xF0\x8F\xBF\xBE", &output));
  EXPECT_FALSE(DecodeStringArray(
      // count of array elements
      "0000000000000000000000000000000000000000000000000000000000000001"
      // invalid data offset to string element.
      "0000000000000000000000000000000000000000000000000000000000001",
      &output));
  EXPECT_FALSE(DecodeStringArray(
      // count of array elements
      "0000000000000000000000000000000000000000000000000000000000000002"
      // out-of-bound offset to array element
      "00000000000000000000000000000000000000000000000000000000000001e0",
      &output));

  EXPECT_FALSE(DecodeStringArray(
      // Mismatched count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000003"
      // offsets to array elements
      "0000000000000000000000000000000000000000000000000000000000000060"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000",
      &output));

  EXPECT_FALSE(DecodeStringArray(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000003"
      // offsets to array elements, last offset point to non-existed data
      "0000000000000000000000000000000000000000000000000000000000000060"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000",
      &output));

  // Missing data offset and data.
  EXPECT_FALSE(DecodeStringArray(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001",
      &output));

  // Missing data.
  EXPECT_FALSE(DecodeStringArray(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001"
      // offset for "one", data missing
      "0000000000000000000000000000000000000000000000000000000000000020",
      &output));

  // Missing count.
  EXPECT_FALSE(DecodeStringArray(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001"
      // offset for "one"
      "0000000000000000000000000000000000000000000000000000000000000020"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000",
      &output));

  // Missing encoding of string.
  EXPECT_FALSE(DecodeStringArray(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001"
      // offset for "one"
      "0000000000000000000000000000000000000000000000000000000000000020"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003",
      &output));
}

TEST(BraveWalletUtilsUnitTest, Namehash) {
  EXPECT_EQ(
      Namehash(""),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      Namehash("eth"),
      "0x93cdeb708b7545dc668eb9280176169d1c33cfd8ed6f04690a0bcc88a93fc4ae");
  EXPECT_EQ(
      Namehash("foo.eth"),
      "0xde9b09fd7c5f901e23a3f19fecc54828e9c848539801e86591bd9801b019f84f");
  EXPECT_EQ(
      Namehash("."),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      Namehash("crypto"),
      "0x0f4a10a4f46c288cea365fcf45cccf0e9d901b945b9829ccdb54c10dc3cb7a6f");
  EXPECT_EQ(
      Namehash("example.crypto"),
      "0xd584c5509c6788ad9d9491be8ba8b4422d05caf62674a98fbf8a9988eeadfb7e");
  EXPECT_EQ(
      Namehash("www.example.crypto"),
      "0x3ae54ac25ccd63401d817b6d79a4a56ae7f79a332fe77a98fa0c9d10adf9b2a1");
  EXPECT_EQ(
      Namehash("a.b.c.crypto"),
      "0x353ea3e0449067382e0ea7934767470170dcfa9c49b1be0fe708adc4b1f9cf13");
  EXPECT_EQ(
      Namehash("brave.crypto"),
      "0x77252571a99feee8f5e6b2f0c8b705407d395adc00b3c8ebcc7c19b2ea850013");
}

TEST(BraveWalletUtilsUnitTest, SecureZeroData) {
  int a = 123;
  SecureZeroData(&a, sizeof(a));
  EXPECT_EQ(a, 0);
  std::string b = "brave";
  SecureZeroData(&b, sizeof(b));
  EXPECT_TRUE(b.empty());
  std::vector<uint8_t> c = {0xde, 0xad, 0xbe, 0xef};
  SecureZeroData(&c, sizeof(c));
  for (const auto& byte : c) {
    EXPECT_EQ(byte, 0);
  }
}

}  // namespace brave_wallet
