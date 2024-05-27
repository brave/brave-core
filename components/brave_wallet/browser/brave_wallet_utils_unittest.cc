/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using base::test::ParseJsonDict;
using testing::Contains;
using testing::ElementsAreArray;
using testing::Eq;
using testing::Not;

namespace brave_wallet {

// DEPRECATED 01/2024. For migration only.
std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id);
std::string GetFilecoinSubdomainForKnownChainId(const std::string& chain_id);
std::string GetBitcoinSubdomainForKnownChainId(const std::string& chain_id);
std::string GetZCashSubdomainForKnownChainId(const std::string& chain_id);
std::string GetKnownEthNetworkId(const std::string& chain_id);
std::string GetKnownSolNetworkId(const std::string& chain_id);
std::string GetKnownFilNetworkId(const std::string& chain_id);
std::string GetKnownBtcNetworkId(const std::string& chain_id);
std::string GetKnownZecNetworkId(const std::string& chain_id);
std::string GetNetworkId(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id);

namespace {

void UpdateCustomNetworks(PrefService* prefs,
                          const std::vector<base::Value::Dict>& values,
                          brave_wallet::mojom::CoinType coin) {
  ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::List* list = update->EnsureList(GetPrefKeyForCoinType(coin));
  list->clear();
  for (auto& it : values) {
    list->Append(it.Clone());
  }
}

void UpdateCustomNetworks(PrefService* prefs,
                          const base::Value::Dict& value,
                          brave_wallet::mojom::CoinType coin) {
  std::vector<base::Value::Dict> values;
  values.push_back(value.Clone());
  UpdateCustomNetworks(prefs, values, coin);
}

std::vector<brave_wallet::mojom::NetworkInfoPtr> GetAllEthCustomChains(
    PrefService* prefs) {
  return GetAllCustomChains(prefs, brave_wallet::mojom::CoinType::ETH);
}

}  // namespace

TEST(BraveWalletUtilsUnitTest, Mnemonic) {
  const struct {
    const char* entropy;
    const char* mnemonic;
    const char* seed;
  } cases[] = {
      {"00000000000000000000000000000000", kMnemonicAbandonAbandon,
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

  for (const auto& entry : cases) {
    std::vector<uint8_t> bytes;
    EXPECT_TRUE(base::HexStringToBytes(entry.entropy, &bytes));
    std::unique_ptr<std::vector<uint8_t>> entropy =
        MnemonicToEntropy(entry.mnemonic);
    EXPECT_EQ(base::ToLowerASCII(base::HexEncode(*entropy)), entry.entropy);

    EXPECT_EQ(GenerateMnemonicForTest(bytes), entry.mnemonic);
    std::unique_ptr<std::vector<uint8_t>> seed =
        MnemonicToSeed(entry.mnemonic, "TREZOR");
    EXPECT_EQ(base::ToLowerASCII(base::HexEncode(*seed)), entry.seed);
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

TEST(BraveWalletUtilsUnitTest, MnemonicToSeedAndEntropy) {
  const char* valid_mnemonic =
      "kingdom possible coast island six arrow fluid spell chunk loud glue "
      "street";
  const char* invalid_mnemonic1 =
      "lingdom possible coast island six arrow fluid spell chunk loud glue "
      "street";
  const char* invalid_mnemonic2 =
      "kingdom possible coast island six arrow fluid spell chunk loud glue";
  EXPECT_NE(MnemonicToSeed(valid_mnemonic, ""), nullptr);
  EXPECT_NE(MnemonicToEntropy(valid_mnemonic), nullptr);
  EXPECT_EQ(MnemonicToSeed(invalid_mnemonic1, ""), nullptr);
  EXPECT_EQ(MnemonicToEntropy(invalid_mnemonic1), nullptr);
  EXPECT_EQ(MnemonicToSeed(invalid_mnemonic2, ""), nullptr);
  EXPECT_EQ(MnemonicToEntropy(invalid_mnemonic2), nullptr);
  EXPECT_EQ(MnemonicToSeed("", ""), nullptr);
  EXPECT_EQ(MnemonicToEntropy(""), nullptr);
}

TEST(BraveWalletUtilsUnitTest, IsValidMnemonic) {
  EXPECT_TRUE(
      IsValidMnemonic("kingdom possible coast island six arrow fluid "
                      "spell chunk loud glue street"));
  EXPECT_FALSE(
      IsValidMnemonic("lingdom possible coast island six arrow fluid "
                      "spell chunk loud glue street"));
  EXPECT_FALSE(IsValidMnemonic("kingdom possible coast island six arrow"));
  EXPECT_FALSE(IsValidMnemonic(""));
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

TEST(BraveWalletUtilsUnitTest, TransactionReceiptAndValue) {
  TransactionReceipt tx_receipt;
  tx_receipt.transaction_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";
  tx_receipt.transaction_index = 0x1;
  tx_receipt.block_number = 0xb;
  tx_receipt.block_hash =
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b";
  tx_receipt.cumulative_gas_used = 0x33bc;
  tx_receipt.gas_used = 0x4dc;
  tx_receipt.contract_address = "0xb60e8dd61c5d32be8058bb8eb970870f07233155";
  tx_receipt.status = true;

  base::Value::Dict tx_receipt_value = TransactionReceiptToValue(tx_receipt);
  auto tx_receipt_from_value = ValueToTransactionReceipt(tx_receipt_value);
  ASSERT_NE(tx_receipt_from_value, std::nullopt);
  EXPECT_EQ(tx_receipt, *tx_receipt_from_value);
}

TEST(BraveWalletUtilsUnitTest, GetAllCustomChainsTest) {
  for (auto coin : kAllCoins) {
    SCOPED_TRACE(coin);
    TestingPrefServiceSimple prefs;
    prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
    ASSERT_TRUE(GetAllCustomChains(&prefs, coin).empty());

    std::vector<base::Value::Dict> values;

    mojom::NetworkInfo chain1 = GetTestNetworkInfo1("chain1", coin);
    values.push_back(NetworkInfoToValue(chain1));

    mojom::NetworkInfo chain2 = GetTestNetworkInfo2("chain2", coin);
    if (coin != mojom::CoinType::ETH) {
      chain2.is_eip1559 = false;
    }
    values.push_back(NetworkInfoToValue(chain2));
    UpdateCustomNetworks(&prefs, std::move(values), coin);

    ASSERT_EQ(2u, GetAllCustomChains(&prefs, coin).size());
    EXPECT_EQ(chain1, *GetAllCustomChains(&prefs, coin)[0]);
    EXPECT_EQ(chain2, *GetAllCustomChains(&prefs, coin)[1]);
  }
}

TEST(BraveWalletUtilsUnitTest, KnownChainExists) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain));
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  auto known_chains = GetAllKnownChains(&prefs, mojom::CoinType::ETH);
  EXPECT_EQ(known_chains.size(), 12u);
  for (auto& known_chain : known_chains) {
    EXPECT_TRUE(KnownChainExists(known_chain->chain_id, mojom::CoinType::ETH));
    // Test that uppercase chain ID works too
    EXPECT_TRUE(KnownChainExists(base::ToUpperASCII(known_chain->chain_id),
                                 mojom::CoinType::ETH));
  }

  EXPECT_TRUE(CustomChainExists(&prefs, chain.chain_id, mojom::CoinType::ETH));
  // Test that uppercase chain ID works too
  EXPECT_TRUE(CustomChainExists(&prefs, base::ToUpperASCII(chain.chain_id),
                                mojom::CoinType::ETH));
  EXPECT_FALSE(KnownChainExists(chain.chain_id, mojom::CoinType::ETH));

  EXPECT_TRUE(KnownChainExists(mojom::kFilecoinMainnet, mojom::CoinType::FIL));
  EXPECT_TRUE(KnownChainExists(mojom::kFilecoinTestnet, mojom::CoinType::FIL));
  EXPECT_TRUE(KnownChainExists(mojom::kLocalhostChainId, mojom::CoinType::FIL));

  EXPECT_TRUE(KnownChainExists(mojom::kSolanaMainnet, mojom::CoinType::SOL));
  EXPECT_TRUE(KnownChainExists(mojom::kSolanaTestnet, mojom::CoinType::SOL));
  EXPECT_TRUE(KnownChainExists(mojom::kSolanaDevnet, mojom::CoinType::SOL));
  EXPECT_TRUE(KnownChainExists(mojom::kLocalhostChainId, mojom::CoinType::SOL));

  EXPECT_TRUE(KnownChainExists(mojom::kBitcoinMainnet, mojom::CoinType::BTC));
  EXPECT_TRUE(KnownChainExists(mojom::kBitcoinTestnet, mojom::CoinType::BTC));

  EXPECT_TRUE(KnownChainExists(mojom::kZCashMainnet, mojom::CoinType::ZEC));
  EXPECT_TRUE(KnownChainExists(mojom::kZCashTestnet, mojom::CoinType::ZEC));

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, CustomChainExists) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));

  EXPECT_FALSE(
      CustomChainExists(&prefs, chain1.chain_id, mojom::CoinType::ETH));
  EXPECT_FALSE(
      CustomChainExists(&prefs, chain2.chain_id, mojom::CoinType::ETH));
  EXPECT_EQ(GetAllEthCustomChains(&prefs).size(), 0u);
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  EXPECT_TRUE(CustomChainExists(&prefs, chain1.chain_id, mojom::CoinType::ETH));
  EXPECT_TRUE(CustomChainExists(&prefs, chain2.chain_id, mojom::CoinType::ETH));
  EXPECT_EQ(GetAllEthCustomChains(&prefs).size(), 2u);

  EXPECT_FALSE(
      CustomChainExists(&prefs, mojom::kFilecoinMainnet, mojom::CoinType::FIL));
  UpdateCustomNetworks(
      &prefs,
      NetworkInfoToValue(*GetAllKnownChains(&prefs, mojom::CoinType::FIL)[0]),
      mojom::CoinType::FIL);
  EXPECT_TRUE(
      CustomChainExists(&prefs, mojom::kFilecoinMainnet, mojom::CoinType::FIL));

  EXPECT_FALSE(
      CustomChainExists(&prefs, mojom::kSolanaMainnet, mojom::CoinType::SOL));
  UpdateCustomNetworks(
      &prefs,
      NetworkInfoToValue(*GetAllKnownChains(&prefs, mojom::CoinType::SOL)[0]),
      mojom::CoinType::SOL);
  EXPECT_TRUE(
      CustomChainExists(&prefs, mojom::kSolanaMainnet, mojom::CoinType::SOL));

  EXPECT_FALSE(
      CustomChainExists(&prefs, mojom::kBitcoinMainnet, mojom::CoinType::BTC));
  UpdateCustomNetworks(
      &prefs,
      NetworkInfoToValue(*GetAllKnownChains(&prefs, mojom::CoinType::BTC)[0]),
      mojom::CoinType::BTC);
  EXPECT_TRUE(
      CustomChainExists(&prefs, mojom::kBitcoinMainnet, mojom::CoinType::BTC));

  EXPECT_FALSE(
      CustomChainExists(&prefs, mojom::kZCashMainnet, mojom::CoinType::ZEC));
  UpdateCustomNetworks(
      &prefs,
      NetworkInfoToValue(*GetAllKnownChains(&prefs, mojom::CoinType::ZEC)[0]),
      mojom::CoinType::ZEC);
  EXPECT_TRUE(
      CustomChainExists(&prefs, mojom::kZCashMainnet, mojom::CoinType::ZEC));

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, CustomChainsExist) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));

  std::vector<std::string> chains{chain1.chain_id, chain2.chain_id,
                                  "unexpected_chain"};
  // Before updating custom networks, none of the chains should be returned
  EXPECT_EQ(CustomChainsExist(&prefs, chains, mojom::CoinType::ETH).size(), 0u);
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  // After updating custom networks the, only the added chains should be
  // returned
  std::vector<std::string> existing_chains =
      CustomChainsExist(&prefs, chains, mojom::CoinType::ETH);
  EXPECT_EQ(existing_chains.size(), 2u);
  EXPECT_EQ(existing_chains[0], chain1.chain_id);
  EXPECT_EQ(existing_chains[1], chain2.chain_id);
  EXPECT_EQ(std::find(existing_chains.begin(), existing_chains.end(),
                      "unexpected_chain"),
            existing_chains.end());
}

TEST(BraveWalletUtilsUnitTest, GetAllChainsTest) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 =
      GetTestNetworkInfo1(mojom::kPolygonMainnetChainId);
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  auto known_chains = GetAllKnownChains(&prefs, mojom::CoinType::ETH);
  auto custom_chains = GetAllEthCustomChains(&prefs);
  EXPECT_EQ(*custom_chains[0], chain1);
  EXPECT_EQ(*custom_chains[1], chain2);

  // Custom Polygon chain takes place of known one.
  // Custom unknown chain becomes last.
  auto expected_chains = std::move(known_chains);
  EXPECT_EQ(expected_chains[2]->chain_id, mojom::kPolygonMainnetChainId);
  expected_chains[2] = chain1.Clone();
  expected_chains.push_back(chain2.Clone());

  auto all_chains = GetAllChains(&prefs, mojom::CoinType::ETH);

  EXPECT_EQ(expected_chains.size(), all_chains.size());
  for (size_t i = 0; i < all_chains.size(); i++) {
    ASSERT_TRUE(all_chains.at(i).Equals(expected_chains.at(i)));
    EXPECT_THAT(all_chains.at(i)->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kDefault}));
  }

  // Solana
  auto sol_main_custom = *GetAllKnownChains(&prefs, mojom::CoinType::SOL)[0];
  sol_main_custom.decimals = 123;
  UpdateCustomNetworks(&prefs, {NetworkInfoToValue(sol_main_custom)},
                       mojom::CoinType::SOL);

  auto sol_chains = GetAllChains(&prefs, mojom::CoinType::SOL);
  ASSERT_EQ(sol_chains.size(), 4u);
  EXPECT_EQ(sol_chains[0]->chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(sol_chains[0]->decimals, 123);
  EXPECT_EQ(sol_chains[1]->chain_id, mojom::kSolanaTestnet);
  EXPECT_EQ(sol_chains[2]->chain_id, mojom::kSolanaDevnet);
  EXPECT_EQ(sol_chains[3]->chain_id, mojom::kLocalhostChainId);

  for (auto& sol_chain : sol_chains) {
    EXPECT_THAT(sol_chain->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kSolana}));
  }

  // Filecoin
  auto fil_main_custom = *GetAllKnownChains(&prefs, mojom::CoinType::FIL)[0];
  fil_main_custom.decimals = 123;
  UpdateCustomNetworks(&prefs, NetworkInfoToValue(fil_main_custom),
                       mojom::CoinType::FIL);

  auto fil_chains = GetAllChains(&prefs, mojom::CoinType::FIL);
  ASSERT_EQ(fil_chains.size(), 3u);
  EXPECT_EQ(fil_chains[0]->chain_id, mojom::kFilecoinMainnet);
  EXPECT_EQ(fil_chains[0]->decimals, 123);
  EXPECT_EQ(fil_chains[1]->chain_id, mojom::kFilecoinTestnet);
  EXPECT_EQ(fil_chains[2]->chain_id, mojom::kLocalhostChainId);
  EXPECT_THAT(fil_chains[0]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kFilecoin}));
  EXPECT_THAT(fil_chains[1]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));
  EXPECT_THAT(fil_chains[2]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));

  // Bitcoin
  auto btc_main_custom = *GetAllKnownChains(&prefs, mojom::CoinType::BTC)[0];
  btc_main_custom.decimals = 123;
  UpdateCustomNetworks(&prefs, NetworkInfoToValue(btc_main_custom),
                       mojom::CoinType::BTC);

  auto btc_chains = GetAllChains(&prefs, mojom::CoinType::BTC);
  ASSERT_EQ(btc_chains.size(), 2u);
  EXPECT_EQ(btc_chains[0]->chain_id, mojom::kBitcoinMainnet);
  EXPECT_EQ(btc_chains[0]->decimals, 123);
  EXPECT_EQ(btc_chains[1]->chain_id, mojom::kBitcoinTestnet);
  EXPECT_THAT(btc_chains[0]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kBitcoin84,
                                mojom::KeyringId::kBitcoinImport}));
  EXPECT_THAT(btc_chains[1]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet,
                                mojom::KeyringId::kBitcoinImportTestnet}));

  // ZCash
  auto zec_main_custom = *GetAllKnownChains(&prefs, mojom::CoinType::ZEC)[0];
  zec_main_custom.decimals = 123;
  UpdateCustomNetworks(&prefs, NetworkInfoToValue(zec_main_custom),
                       mojom::CoinType::ZEC);

  auto zec_chains = GetAllChains(&prefs, mojom::CoinType::ZEC);
  ASSERT_EQ(zec_chains.size(), 2u);
  EXPECT_EQ(zec_chains[0]->chain_id, mojom::kZCashMainnet);
  EXPECT_EQ(zec_chains[0]->decimals, 123);
  EXPECT_EQ(zec_chains[1]->chain_id, mojom::kZCashTestnet);
  EXPECT_THAT(zec_chains[0]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kZCashMainnet}));
  EXPECT_THAT(zec_chains[1]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kZCashTestnet}));

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, GetNetworkURLTest) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);
  for (const auto& chain : GetAllKnownChains(&prefs, mojom::CoinType::ETH)) {
    // Brave proxies should have infura key added to path.
    GURL rpc_url(chain->rpc_endpoints.front());
    if (base::EndsWith(rpc_url.host(), "brave.com")) {
      rpc_url = AddInfuraProjectId(rpc_url);
    }

    EXPECT_EQ(rpc_url,
              GetNetworkURL(&prefs, chain->chain_id, mojom::CoinType::ETH));
  }
  EXPECT_EQ(chain1.rpc_endpoints.front(),
            GetNetworkURL(&prefs, chain1.chain_id, mojom::CoinType::ETH));
  EXPECT_EQ(chain2.rpc_endpoints.front(),
            GetNetworkURL(&prefs, chain2.chain_id, mojom::CoinType::ETH));

  EXPECT_EQ(GURL("https://mainnet-beta-solana.brave.com/rpc"),
            GetNetworkURL(&prefs, mojom::kSolanaMainnet, mojom::CoinType::SOL));
  auto custom_sol_network =
      GetKnownChain(&prefs, mojom::kSolanaMainnet, mojom::CoinType::SOL);
  custom_sol_network->rpc_endpoints.emplace_back("https://test-sol.com");
  custom_sol_network->active_rpc_endpoint_index = 1;
  UpdateCustomNetworks(&prefs, {NetworkInfoToValue(*custom_sol_network)},
                       mojom::CoinType::SOL);
  EXPECT_EQ(GURL("https://test-sol.com"),
            GetNetworkURL(&prefs, mojom::kSolanaMainnet, mojom::CoinType::SOL));

  EXPECT_EQ(
      GURL("https://api.node.glif.io/rpc/v0"),
      GetNetworkURL(&prefs, mojom::kFilecoinMainnet, mojom::CoinType::FIL));
  auto custom_fil_network =
      GetKnownChain(&prefs, mojom::kFilecoinMainnet, mojom::CoinType::FIL);
  custom_fil_network->rpc_endpoints.emplace_back("https://test-fil.com");
  custom_fil_network->active_rpc_endpoint_index = 1;
  UpdateCustomNetworks(&prefs, {NetworkInfoToValue(*custom_fil_network)},
                       mojom::CoinType::FIL);
  EXPECT_EQ(
      GURL("https://test-fil.com"),
      GetNetworkURL(&prefs, mojom::kFilecoinMainnet, mojom::CoinType::FIL));

  EXPECT_EQ(
      GURL("https://bitcoin-mainnet.wallet.brave.com/"),
      GetNetworkURL(&prefs, mojom::kBitcoinMainnet, mojom::CoinType::BTC));
  auto custom_btc_network =
      GetKnownChain(&prefs, mojom::kBitcoinMainnet, mojom::CoinType::BTC);
  custom_btc_network->rpc_endpoints.emplace_back("https://test-btc.com");
  custom_btc_network->active_rpc_endpoint_index = 1;
  UpdateCustomNetworks(&prefs, {NetworkInfoToValue(*custom_btc_network)},
                       mojom::CoinType::BTC);
  EXPECT_EQ(
      GURL("https://test-btc.com"),
      GetNetworkURL(&prefs, mojom::kBitcoinMainnet, mojom::CoinType::BTC));

  EXPECT_EQ(GURL("https://mainnet.lightwalletd.com:9067/"),
            GetNetworkURL(&prefs, mojom::kZCashMainnet, mojom::CoinType::ZEC));
  auto custom_zec_network =
      GetKnownChain(&prefs, mojom::kZCashMainnet, mojom::CoinType::ZEC);
  custom_zec_network->rpc_endpoints.emplace_back("https://test-zec.com");
  custom_zec_network->active_rpc_endpoint_index = 1;
  UpdateCustomNetworks(&prefs, {NetworkInfoToValue(*custom_zec_network)},
                       mojom::CoinType::ZEC);
  EXPECT_EQ(GURL("https://test-zec.com"),
            GetNetworkURL(&prefs, mojom::kZCashMainnet, mojom::CoinType::ZEC));

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, GetNetworkURLForKnownChains) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  // GetNetworkURL for these known chains should resolve to brave subdomain.
  base::flat_set<std::string> infura_chains = {
      brave_wallet::mojom::kMainnetChainId,
      brave_wallet::mojom::kPolygonMainnetChainId,
      brave_wallet::mojom::kOptimismMainnetChainId,
      brave_wallet::mojom::kAuroraMainnetChainId,
      brave_wallet::mojom::kAvalancheMainnetChainId,
      brave_wallet::mojom::kGoerliChainId,
      brave_wallet::mojom::kSepoliaChainId};

  for (const auto& chain : GetAllKnownChains(&prefs, mojom::CoinType::ETH)) {
    auto network_url =
        GetNetworkURL(&prefs, chain->chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(base::EndsWith(network_url.host(), ".brave.com"),
              infura_chains.contains(chain->chain_id));
  }
}

TEST(BraveWalletUtilsUnitTest, GetSolanaSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::SOL)) {
    auto subdomain = GetSolanaSubdomainForKnownChainId(chain->chain_id);
    bool expected = (chain->chain_id == brave_wallet::mojom::kLocalhostChainId);
    ASSERT_EQ(subdomain.empty(), expected);
  }
}

TEST(BraveWalletUtilsUnitTest, GetFilecoinSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::FIL)) {
    auto subdomain = GetFilecoinSubdomainForKnownChainId(chain->chain_id);
    bool expected = (chain->chain_id == brave_wallet::mojom::kLocalhostChainId);
    ASSERT_EQ(subdomain.empty(), expected);
  }
}

TEST(BraveWalletUtilsUnitTest, GetBitcoinSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::BTC)) {
    auto subdomain = GetBitcoinSubdomainForKnownChainId(chain->chain_id);
    ASSERT_FALSE(subdomain.empty());
  }
}

TEST(BraveWalletUtilsUnitTest, GetZCashSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::ZEC)) {
    auto subdomain = GetZCashSubdomainForKnownChainId(chain->chain_id);
    ASSERT_FALSE(subdomain.empty());
  }
}

TEST(BraveWalletUtilsUnitTest, GetKnownChain) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  const base::flat_set<std::string> non_eip1559_networks = {
      brave_wallet::mojom::kLocalhostChainId,
      brave_wallet::mojom::kBnbSmartChainMainnetChainId,
      brave_wallet::mojom::kAuroraMainnetChainId,
      brave_wallet::mojom::kNeonEVMMainnetChainId};

  auto known_chains = GetAllKnownChains(&prefs, mojom::CoinType::ETH);
  ASSERT_FALSE(known_chains.empty());
  for (const auto& chain : known_chains) {
    auto network = GetKnownChain(&prefs, chain->chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(network->chain_id, chain->chain_id);
    EXPECT_EQ(network->chain_name, chain->chain_name);
    EXPECT_TRUE(GetActiveEndpointUrl(*network).is_valid());
    EXPECT_EQ(network->icon_urls, chain->icon_urls);
    EXPECT_EQ(network->block_explorer_urls, chain->block_explorer_urls);
    EXPECT_EQ(network->symbol, chain->symbol);
    EXPECT_EQ(network->decimals, chain->decimals);
    EXPECT_EQ(network->symbol_name, chain->symbol_name);
    ASSERT_EQ(network->is_eip1559,
              !non_eip1559_networks.contains(chain->chain_id));
  }

  prefs.SetBoolean(kSupportEip1559OnLocalhostChain, true);
  auto network = GetKnownChain(&prefs, brave_wallet::mojom::kLocalhostChainId,
                               mojom::CoinType::ETH);
  EXPECT_EQ(network->is_eip1559, true);
}

TEST(BraveWalletUtilsUnitTest, GetCustomChain) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  EXPECT_FALSE(GetCustomChain(&prefs, "chain_id", mojom::CoinType::ETH));

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain));
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  auto network = GetCustomChain(&prefs, chain.chain_id, mojom::CoinType::ETH);
  ASSERT_TRUE(network);
  EXPECT_EQ(*network, chain);

  // Test that uppercase chain ID works too
  network = GetCustomChain(&prefs, base::ToUpperASCII(chain.chain_id),
                           mojom::CoinType::ETH);
  ASSERT_TRUE(network);
  EXPECT_EQ(*network, chain);
}

TEST(BraveWalletUtilsUnitTest, GetChain) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1("0x5566");
  values.push_back(NetworkInfoToValue(chain1));
  mojom::NetworkInfo chain2 = GetTestNetworkInfo1("0x89");
  values.push_back(NetworkInfoToValue(chain2));
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  EXPECT_FALSE(GetChain(&prefs, "", mojom::CoinType::ETH));

  EXPECT_FALSE(GetChain(&prefs, "0x123", mojom::CoinType::ETH));
  EXPECT_EQ(GetChain(&prefs, "0x5566", mojom::CoinType::ETH), chain1.Clone());
  EXPECT_EQ(GetChain(&prefs, "0x1", mojom::CoinType::ETH),
            GetKnownChain(&prefs, "0x1", mojom::CoinType::ETH));
  EXPECT_EQ(GetChain(&prefs, "0x539", mojom::CoinType::ETH),
            GetKnownChain(&prefs, "0x539", mojom::CoinType::ETH));

  EXPECT_EQ(*GetChain(&prefs, "0x89", mojom::CoinType::ETH), chain2);

  // Solana
  mojom::NetworkInfo sol_mainnet(
      brave_wallet::mojom::kSolanaMainnet, "Solana Mainnet Beta",
      {"https://explorer.solana.com/"}, {}, 0,
      {GURL("https://mainnet-beta-solana.brave.com/rpc")}, "SOL", "Solana", 9,
      brave_wallet::mojom::CoinType::SOL, {mojom::KeyringId::kSolana}, false);
  EXPECT_FALSE(GetChain(&prefs, "0x123", mojom::CoinType::SOL));
  EXPECT_EQ(GetChain(&prefs, "0x65", mojom::CoinType::SOL),
            sol_mainnet.Clone());

  // Filecoin
  mojom::NetworkInfo fil_mainnet(
      brave_wallet::mojom::kFilecoinMainnet, "Filecoin Mainnet",
      {"https://filscan.io/tipset/message-detail"}, {}, 0,
      {GURL("https://api.node.glif.io/rpc/v0")}, "FIL", "Filecoin", 18,
      brave_wallet::mojom::CoinType::FIL, {mojom::KeyringId::kFilecoin}, false);
  EXPECT_FALSE(GetChain(&prefs, "0x123", mojom::CoinType::FIL));
  EXPECT_EQ(GetChain(&prefs, "f", mojom::CoinType::FIL), fil_mainnet.Clone());

  // Bitcoin
  mojom::NetworkInfo btc_mainnet(
      mojom::kBitcoinMainnet, "Bitcoin Mainnet",
      {"https://www.blockchain.com/explorer"}, {}, 0,
      {GURL("https://bitcoin-mainnet.wallet.brave.com/")}, "BTC", "Bitcoin", 8,
      mojom::CoinType::BTC,
      {mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoinImport}, false);
  EXPECT_FALSE(GetChain(&prefs, "0x123", mojom::CoinType::BTC));
  EXPECT_EQ(GetChain(&prefs, "bitcoin_mainnet", mojom::CoinType::BTC),
            btc_mainnet.Clone());

  // Zcash
  mojom::NetworkInfo zec_mainnet(
      mojom::kZCashMainnet, "Zcash Mainnet",
      {"https://zcashblockexplorer.com/transactions"}, {}, 0,
      {GURL("https://mainnet.lightwalletd.com:9067/")}, "ZEC", "Zcash", 8,
      mojom::CoinType::ZEC, {mojom::KeyringId::kZCashMainnet}, false);
  EXPECT_FALSE(GetChain(&prefs, "0x123", mojom::CoinType::ZEC));
  EXPECT_EQ(GetChain(&prefs, "zcash_mainnet", mojom::CoinType::ZEC),
            zec_mainnet.Clone());

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, GetKnownEthNetworkId) {
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kLocalhostChainId),
            "http://localhost:7545/");
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kMainnetChainId), "mainnet");
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kGoerliChainId), "goerli");
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kSepoliaChainId), "sepolia");
}

TEST(BraveWalletUtilsUnitTest, GetKnownSolNetworkId) {
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kLocalhostChainId),
            "http://localhost:8899/");
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kSolanaMainnet), "mainnet");
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kSolanaTestnet), "testnet");
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kSolanaDevnet), "devnet");
}

TEST(BraveWalletUtilsUnitTest, GetKnownFilNetworkId) {
  EXPECT_EQ(GetKnownFilNetworkId(mojom::kLocalhostChainId),
            "http://localhost:1234/rpc/v0");
  EXPECT_EQ(GetKnownFilNetworkId(mojom::kFilecoinMainnet), "mainnet");
  EXPECT_EQ(GetKnownFilNetworkId(mojom::kFilecoinTestnet), "testnet");
}

TEST(BraveWalletUtilsUnitTest, GetNetworkId) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
  ASSERT_TRUE(GetAllEthCustomChains(&prefs).empty());

  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  values.push_back(NetworkInfoToValue(chain1));

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  values.push_back(NetworkInfoToValue(chain2));
  UpdateCustomNetworks(&prefs, std::move(values), mojom::CoinType::ETH);

  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::ETH, mojom::kMainnetChainId),
            "mainnet");
  EXPECT_EQ(
      GetNetworkId(&prefs, mojom::CoinType::ETH, mojom::kLocalhostChainId),
      "http://localhost:7545/");
  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::ETH, "chain_id"), "chain_id");
  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::ETH, "chain_id2"),
            "chain_id2");
  EXPECT_EQ(
      GetNetworkId(&prefs, mojom::CoinType::ETH, mojom::kPolygonMainnetChainId),
      mojom::kPolygonMainnetChainId);
  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::ETH,
                         mojom::kBnbSmartChainMainnetChainId),
            mojom::kBnbSmartChainMainnetChainId);

  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::SOL, mojom::kSolanaMainnet),
            "mainnet");
  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::SOL, mojom::kSolanaTestnet),
            "testnet");
  EXPECT_EQ(GetNetworkId(&prefs, mojom::CoinType::SOL, mojom::kSolanaDevnet),
            "devnet");
}

TEST(BraveWalletUtilsUnitTest, AddCustomNetwork) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
  prefs.registry()->RegisterListPref(kBraveWalletUserAssetsList);

  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  chain2.icon_urls.clear();

  AddCustomNetwork(&prefs, chain1);
  AddCustomNetwork(&prefs, chain2);

  // kBraveWalletCustomNetworks should be updated with new chains.
  ASSERT_EQ(2u, GetAllEthCustomChains(&prefs).size());
  EXPECT_EQ(chain1, *GetAllEthCustomChains(&prefs)[0]);
  EXPECT_EQ(chain2, *GetAllEthCustomChains(&prefs)[1]);

  // Asset list of new custom chains should have native asset in
  // kBraveWalletUserAssets.
  const auto& asset_list = prefs.GetList(kBraveWalletUserAssetsList);
  ASSERT_EQ(asset_list.size(), 2u);

  EXPECT_EQ(*asset_list[0].GetDict().FindInt("coin"),
            static_cast<int>(mojom::CoinType::ETH));
  EXPECT_EQ(*asset_list[0].GetDict().FindString("chain_id"), "chain_id");
  EXPECT_EQ(*asset_list[0].GetDict().FindString("address"), "");
  EXPECT_EQ(*asset_list[0].GetDict().FindString("name"), "symbol_name");
  EXPECT_EQ(*asset_list[0].GetDict().FindString("symbol"), "symbol");
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("is_erc20"), false);
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("is_erc721"), false);
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("is_erc1155"), false);
  EXPECT_EQ(*asset_list[0].GetDict().FindInt("decimals"), 11);
  EXPECT_EQ(*asset_list[0].GetDict().FindString("logo"), "https://url1.com");
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("visible"), true);

  EXPECT_EQ(*asset_list[1].GetDict().FindInt("coin"),
            static_cast<int>(mojom::CoinType::ETH));
  EXPECT_EQ(*asset_list[1].GetDict().FindString("chain_id"), "chain_id2");
  EXPECT_EQ(*asset_list[1].GetDict().FindString("address"), "");
  EXPECT_EQ(*asset_list[1].GetDict().FindString("name"), "symbol_name2");
  EXPECT_EQ(*asset_list[1].GetDict().FindString("symbol"), "symbol2");
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("is_erc20"), false);
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("is_erc721"), false);
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("is_erc1155"), false);
  EXPECT_EQ(*asset_list[1].GetDict().FindInt("decimals"), 22);
  EXPECT_EQ(*asset_list[1].GetDict().FindString("logo"), "");
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("visible"), true);

  {
    mojom::NetworkInfo chain_fil =
        GetTestNetworkInfo1(mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    AddCustomNetwork(&prefs, chain_fil);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::FIL).size());
    EXPECT_EQ(chain_fil, *GetAllCustomChains(&prefs, mojom::CoinType::FIL)[0]);
  }

  {
    mojom::NetworkInfo chain_sol =
        GetTestNetworkInfo1(mojom::kSolanaMainnet, mojom::CoinType::SOL);
    AddCustomNetwork(&prefs, chain_sol);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::SOL).size());
    EXPECT_EQ(chain_sol, *GetAllCustomChains(&prefs, mojom::CoinType::SOL)[0]);
  }

  {
    mojom::NetworkInfo chain_btc =
        GetTestNetworkInfo1(mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    AddCustomNetwork(&prefs, chain_btc);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::BTC).size());
    EXPECT_EQ(chain_btc, *GetAllCustomChains(&prefs, mojom::CoinType::BTC)[0]);
  }

  {
    mojom::NetworkInfo chain_zec =
        GetTestNetworkInfo1(mojom::kZCashMainnet, mojom::CoinType::ZEC);
    AddCustomNetwork(&prefs, chain_zec);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::ZEC).size());
    EXPECT_EQ(chain_zec, *GetAllCustomChains(&prefs, mojom::CoinType::ZEC)[0]);
  }

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, AddCustomNetworkTwice) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();

  auto assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(24u, assets.size());

  AddCustomNetwork(&prefs, chain1);
  assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(25u, assets.size());

  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_TRUE(assets.back()->visible);

  SetUserAssetVisible(&prefs, assets.back(), false);
  assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_FALSE(assets.back()->visible);

  RemoveCustomNetwork(&prefs, chain1.chain_id, chain1.coin);
  // TODO(apaymyshev): Maybe we should remove such assets.
  assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(25u, assets.size());
  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_FALSE(assets.back()->visible);

  // Network added again. No duplicate assets.
  AddCustomNetwork(&prefs, chain1);
  assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(25u, assets.size());
  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_TRUE(assets.back()->visible);
}

TEST(BraveWalletUtilsUnitTest, CustomNetworkMatchesKnownNetwork) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
  prefs.registry()->RegisterListPref(kBraveWalletUserAssetsList);

  auto get_polygon_from_all = [&] {
    for (const auto& chain : GetAllChains(&prefs, mojom::CoinType::ETH)) {
      if (chain->chain_id == mojom::kPolygonMainnetChainId) {
        return chain.Clone();
      }
    }
    return mojom::NetworkInfoPtr();
  };

  // Known network by default.
  EXPECT_EQ(get_polygon_from_all()->chain_name, "Polygon Mainnet");
  EXPECT_EQ(
      GetNetworkURL(&prefs, mojom::kPolygonMainnetChainId, mojom::CoinType::ETH)
          .GetWithoutFilename(),
      GURL("https://mainnet-polygon.brave.com/"));

  mojom::NetworkInfo chain1 =
      GetTestNetworkInfo1(mojom::kPolygonMainnetChainId);

  AddCustomNetwork(&prefs, chain1);

  // Custom network overrides known one.
  EXPECT_EQ(get_polygon_from_all()->chain_name, "chain_name");
  EXPECT_EQ(
      GetNetworkURL(&prefs, mojom::kPolygonMainnetChainId, mojom::CoinType::ETH)
          .GetWithoutFilename(),
      GURL("https://url1.com/"));

  RemoveCustomNetwork(&prefs, mojom::kPolygonMainnetChainId,
                      mojom::CoinType::ETH);

  // Back to known when custom is removed.
  EXPECT_EQ(get_polygon_from_all()->chain_name, "Polygon Mainnet");
  EXPECT_EQ(
      GetNetworkURL(&prefs, mojom::kPolygonMainnetChainId, mojom::CoinType::ETH)
          .GetWithoutFilename(),
      GURL("https://mainnet-polygon.brave.com/"));
}

TEST(BraveWalletUtilsUnitTest, RemoveCustomNetwork) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
  prefs.registry()->RegisterListPref(kBraveWalletUserAssetsList);

  mojom::NetworkInfo chain = GetTestNetworkInfo1();

  AddCustomNetwork(&prefs, chain);
  EXPECT_TRUE(CustomChainExists(&prefs, chain.chain_id, mojom::CoinType::ETH));

  RemoveCustomNetwork(&prefs, chain.chain_id, mojom::CoinType::ETH);
  EXPECT_FALSE(CustomChainExists(&prefs, chain.chain_id, mojom::CoinType::ETH));

  // Should not crash.
  RemoveCustomNetwork(&prefs, "unknown network", mojom::CoinType::ETH);

  {
    mojom::NetworkInfo chain_fil =
        GetTestNetworkInfo1(mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    AddCustomNetwork(&prefs, chain_fil);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::FIL).size());
    RemoveCustomNetwork(&prefs, mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    ASSERT_EQ(0u, GetAllCustomChains(&prefs, mojom::CoinType::FIL).size());
  }

  {
    mojom::NetworkInfo chain_sol =
        GetTestNetworkInfo1(mojom::kSolanaMainnet, mojom::CoinType::SOL);
    AddCustomNetwork(&prefs, chain_sol);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::SOL).size());
    RemoveCustomNetwork(&prefs, mojom::kSolanaMainnet, mojom::CoinType::SOL);
    ASSERT_EQ(0u, GetAllCustomChains(&prefs, mojom::CoinType::SOL).size());
  }

  {
    mojom::NetworkInfo chain_btc =
        GetTestNetworkInfo1(mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    AddCustomNetwork(&prefs, chain_btc);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::BTC).size());
    RemoveCustomNetwork(&prefs, mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    ASSERT_EQ(0u, GetAllCustomChains(&prefs, mojom::CoinType::BTC).size());
  }

  {
    mojom::NetworkInfo chain_zec =
        GetTestNetworkInfo1(mojom::kZCashMainnet, mojom::CoinType::ZEC);
    AddCustomNetwork(&prefs, chain_zec);
    ASSERT_EQ(1u, GetAllCustomChains(&prefs, mojom::CoinType::ZEC).size());
    RemoveCustomNetwork(&prefs, mojom::kZCashMainnet, mojom::CoinType::ZEC);
    ASSERT_EQ(0u, GetAllCustomChains(&prefs, mojom::CoinType::ZEC).size());
  }

  EXPECT_TRUE(AllCoinsTested());
}

TEST(BraveWalletUtilsUnitTest, HiddenNetworks) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_THAT(
      GetHiddenNetworks(&prefs, mojom::CoinType::ETH),
      ElementsAreArray<std::string>(
          {mojom::kGoerliChainId, mojom::kSepoliaChainId,
           mojom::kLocalhostChainId, mojom::kFilecoinEthereumTestnetChainId}));
  EXPECT_THAT(GetHiddenNetworks(&prefs, mojom::CoinType::FIL),
              ElementsAreArray<std::string>(
                  {mojom::kFilecoinTestnet, mojom::kLocalhostChainId}));
  EXPECT_THAT(GetHiddenNetworks(&prefs, mojom::CoinType::SOL),
              ElementsAreArray<std::string>({mojom::kSolanaDevnet,
                                             mojom::kSolanaTestnet,
                                             mojom::kLocalhostChainId}));
  EXPECT_THAT(GetHiddenNetworks(&prefs, mojom::CoinType::BTC),
              ElementsAreArray<std::string>({mojom::kBitcoinTestnet}));
  EXPECT_THAT(GetHiddenNetworks(&prefs, mojom::CoinType::ZEC),
              ElementsAreArray<std::string>({mojom::kZCashTestnet}));
  EXPECT_TRUE(AllCoinsTested());

  for (auto coin : kAllCoins) {
    for (auto& default_hidden : GetHiddenNetworks(&prefs, coin)) {
      RemoveHiddenNetwork(&prefs, coin, default_hidden);
    }

    EXPECT_THAT(GetHiddenNetworks(&prefs, coin),
                ElementsAreArray<std::string>({}));

    AddHiddenNetwork(&prefs, coin, "0x123");
    EXPECT_THAT(GetHiddenNetworks(&prefs, coin), ElementsAreArray({"0x123"}));
    AddHiddenNetwork(&prefs, coin, "0x123");
    EXPECT_THAT(GetHiddenNetworks(&prefs, coin), ElementsAreArray({"0x123"}));

    RemoveHiddenNetwork(&prefs, coin, "0x555");
    EXPECT_THAT(GetHiddenNetworks(&prefs, coin), ElementsAreArray({"0x123"}));

    AddHiddenNetwork(&prefs, coin, "0x7");
    EXPECT_THAT(GetHiddenNetworks(&prefs, coin),
                ElementsAreArray({"0x123", "0x7"}));

    RemoveHiddenNetwork(&prefs, coin, "0x123");
    EXPECT_THAT(GetHiddenNetworks(&prefs, coin), ElementsAreArray({"0x7"}));

    RemoveHiddenNetwork(&prefs, coin, "0x7");
    EXPECT_THAT(GetHiddenNetworks(&prefs, coin),
                ElementsAreArray<std::string>({}));
  }
}

TEST(BraveWalletUtilsUnitTest, GetPrefKeyForCoinType) {
  auto key = GetPrefKeyForCoinType(mojom::CoinType::ETH);
  EXPECT_EQ(key, kEthereumPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::FIL);
  EXPECT_EQ(key, kFilecoinPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::SOL);
  EXPECT_EQ(key, kSolanaPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::BTC);
  EXPECT_EQ(key, kBitcoinPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::ZEC);
  EXPECT_EQ(key, kZCashPrefKey);

  EXPECT_TRUE(AllCoinsTested());

  EXPECT_DCHECK_DEATH(
      GetPrefKeyForCoinType(static_cast<mojom::CoinType>(2016)));
}

TEST(BraveWalletUtilsUnitTest, GetAndSetCurrentChainId) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());
  const base::flat_map<mojom::CoinType, std::string> default_chain_ids = {
      {mojom::CoinType::ETH, mojom::kMainnetChainId},
      {mojom::CoinType::SOL, mojom::kSolanaMainnet},
      {mojom::CoinType::FIL, mojom::kFilecoinMainnet},
  };
  const base::flat_map<mojom::CoinType, std::string> new_default_chain_ids = {
      {mojom::CoinType::ETH, mojom::kGoerliChainId},
      {mojom::CoinType::SOL, mojom::kSolanaTestnet},
      {mojom::CoinType::FIL, mojom::kFilecoinTestnet},
  };

  EXPECT_TRUE(AllCoinsTested());

  for (const auto coin_type : kAllCoins) {
    // TODO(apaymyshev): make this test working for BTC which has no localhost
    if (coin_type == mojom::CoinType::BTC ||
        coin_type == mojom::CoinType::ZEC) {
      continue;
    }

    // default value
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, std::nullopt),
              default_chain_ids.at(coin_type));

    // fallback to default
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type,
                                url::Origin::Create(GURL("https://a.com"))),
              default_chain_ids.at(coin_type));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, url::Origin()),
              default_chain_ids.at(coin_type));
    EXPECT_EQ(
        GetCurrentChainId(&prefs, coin_type,
                          url::Origin::Create(GURL("file:///etc/passwd"))),
        default_chain_ids.at(coin_type));

    // unknown chain_id
    EXPECT_FALSE(SetCurrentChainId(&prefs, coin_type,
                                   url::Origin::Create(GURL("https://a.com")),
                                   "0x5566"));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type,
                                url::Origin::Create(GURL("https://a.com"))),
              default_chain_ids.at(coin_type));
    EXPECT_FALSE(SetCurrentChainId(&prefs, coin_type, std::nullopt, "0x5566"));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, std::nullopt),
              default_chain_ids.at(coin_type));

    EXPECT_TRUE(SetCurrentChainId(&prefs, coin_type,
                                  url::Origin::Create(GURL("https://a.com")),
                                  mojom::kLocalhostChainId));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type,
                                url::Origin::Create(GURL("https://a.com"))),
              mojom::kLocalhostChainId);
    // other origin still use default
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type,
                                url::Origin::Create(GURL("https://b.com"))),
              default_chain_ids.at(coin_type));

    // opaque cannot change the default
    EXPECT_FALSE(SetCurrentChainId(&prefs, coin_type, url::Origin(),
                                   mojom::kLocalhostChainId));
    EXPECT_FALSE(SetCurrentChainId(&prefs, coin_type,
                                   url::Origin::Create(GURL("about:blank")),
                                   mojom::kLocalhostChainId));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, std::nullopt),
              default_chain_ids.at(coin_type));

    // now we change the default
    EXPECT_TRUE(SetCurrentChainId(&prefs, coin_type, std::nullopt,
                                  new_default_chain_ids.at(coin_type)));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, std::nullopt),
              new_default_chain_ids.at(coin_type));
    // should not affect per origin pref
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type,
                                url::Origin::Create(GURL("https://a.com"))),
              mojom::kLocalhostChainId);

    // non http/https scheme will change the default
    EXPECT_TRUE(SetCurrentChainId(
        &prefs, coin_type, url::Origin::Create(GURL("file:///etc/passwd")),
        default_chain_ids.at(coin_type)));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, std::nullopt),
              default_chain_ids.at(coin_type));
    EXPECT_TRUE(SetCurrentChainId(&prefs, coin_type,
                                  url::Origin::Create(GURL("chrome://wallet")),
                                  new_default_chain_ids.at(coin_type)));
    EXPECT_EQ(GetCurrentChainId(&prefs, coin_type, std::nullopt),
              new_default_chain_ids.at(coin_type));
  }
}

TEST(BraveWalletUtilsUnitTest, eTLDPlusOne) {
  EXPECT_EQ("", eTLDPlusOne(url::Origin()));
  EXPECT_EQ("brave.com",
            eTLDPlusOne(url::Origin::Create(GURL("https://blog.brave.com"))));
  EXPECT_EQ("brave.com",
            eTLDPlusOne(url::Origin::Create(GURL("https://...brave.com"))));
  EXPECT_EQ(
      "brave.com",
      eTLDPlusOne(url::Origin::Create(GURL("https://a.b.c.d.brave.com/1"))));
  EXPECT_EQ("brave.github.io", eTLDPlusOne(url::Origin::Create(GURL(
                                   "https://a.b.brave.github.io/example"))));
  EXPECT_EQ("", eTLDPlusOne(url::Origin::Create(GURL("https://github.io"))));
}

TEST(BraveWalletUtilsUnitTest, MakeOriginInfo) {
  auto origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://blog.brave.com:443")));
  EXPECT_EQ("https://blog.brave.com", origin_info->origin_spec);
  EXPECT_EQ("brave.com", origin_info->e_tld_plus_one);

  url::Origin empty_origin;
  auto empty_origin_info = MakeOriginInfo(empty_origin);
  EXPECT_EQ("null", empty_origin_info->origin_spec);
  EXPECT_EQ("", empty_origin_info->e_tld_plus_one);
}

TEST(BraveWalletUtilsUnitTest, GenerateRandomHexString) {
  std::set<std::string> strings;
  for (int i = 0; i < 1000; ++i) {
    auto random_string = GenerateRandomHexString();
    EXPECT_EQ(64u, random_string.size());
    EXPECT_TRUE(base::ranges::all_of(random_string, base::IsHexDigit<char>));
    EXPECT_FALSE(base::Contains(strings, random_string));
    strings.insert(random_string);
  }
}

TEST(BraveWalletUtilsUnitTest, GetUnstoppableDomainsRpcUrl) {
  EXPECT_EQ(AddInfuraProjectId(GURL("https://mainnet-infura.brave.com")),
            GetUnstoppableDomainsRpcUrl(mojom::kMainnetChainId));
  EXPECT_EQ(AddInfuraProjectId(GURL("https://mainnet-polygon.brave.com")),
            GetUnstoppableDomainsRpcUrl(mojom::kPolygonMainnetChainId));
}

TEST(BraveWalletUtilsUnitTest, GetEnsRpcUrl) {
  EXPECT_EQ(AddInfuraProjectId(GURL("https://mainnet-infura.brave.com")),
            GetEnsRpcUrl());
}

TEST(BraveWalletUtilsUnitTest, GetSnsRpcUrl) {
  EXPECT_EQ(GURL("https://mainnet-beta-solana.brave.com/rpc"), GetSnsRpcUrl());
}

TEST(BraveWalletUtilsUnitTest, GetChainIdByNetworkId) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  prefs.registry()->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);

  for (auto coin : kAllCoins) {
    ASSERT_TRUE(GetAllCustomChains(&prefs, coin).empty());
    std::vector<base::Value::Dict> values;

    mojom::NetworkInfo chain1 = GetTestNetworkInfo1("chain_id", coin);
    values.push_back(NetworkInfoToValue(chain1));

    mojom::NetworkInfo chain2 = GetTestNetworkInfo2("chain_id", coin);
    if (coin != mojom::CoinType::ETH) {
      chain2.is_eip1559 = false;
    }
    values.push_back(NetworkInfoToValue(chain2));
    UpdateCustomNetworks(&prefs, std::move(values), coin);
  }

  auto getChainIdByNetworkIdCheck = [&](const mojom::CoinType& coin_type) {
    for (const auto& chain : GetAllChains(&prefs, coin_type)) {
      std::string nid;
      if (chain->coin == mojom::CoinType::ETH) {
        nid = GetKnownEthNetworkId(chain->chain_id);
      }
      if (chain->coin == mojom::CoinType::SOL) {
        nid = GetKnownSolNetworkId(chain->chain_id);
      }
      if (chain->coin == mojom::CoinType::FIL) {
        nid = GetKnownFilNetworkId(chain->chain_id);
      }
      if (chain->coin == mojom::CoinType::BTC) {
        nid = GetKnownBtcNetworkId(chain->chain_id);
      }
      if (chain->coin == mojom::CoinType::ZEC) {
        nid = GetKnownZecNetworkId(chain->chain_id);
      }
      if (nid.empty()) {
        nid = chain->chain_id;
        // GetNetworkId supports only ETH for custom networks atm.
        if (chain->coin != mojom::CoinType::ETH) {
          ASSERT_FALSE(
              GetChainIdByNetworkId_DEPRECATED(&prefs, coin_type, nid));
          continue;
        }
      }
      auto chain_id = GetChainIdByNetworkId_DEPRECATED(&prefs, coin_type, nid);
      ASSERT_TRUE(chain_id.has_value());
      EXPECT_EQ(chain->chain_id, chain_id.value());
    }
    ASSERT_FALSE(GetChainIdByNetworkId_DEPRECATED(&prefs, coin_type, ""));
  };

  for (auto coin : kAllCoins) {
    getChainIdByNetworkIdCheck(coin);
  }
}

TEST(BraveWalletUtilsUnitTest, BitcoinNativeAssets) {
  EXPECT_EQ(
      BlockchainTokenToValue(GetBitcoinNativeToken(mojom::kBitcoinMainnet)),
      ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "bitcoin_mainnet",
        "coin": 0,
        "coingecko_id": "btc",
        "decimals": 8,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "is_nft": false,
        "is_spam": false,
        "logo": "btc.png",
        "name": "Bitcoin",
        "symbol": "BTC",
        "token_id": "",
        "visible": true
      }
      )"));

  EXPECT_EQ(
      BlockchainTokenToValue(GetBitcoinNativeToken(mojom::kBitcoinTestnet)),
      ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "bitcoin_testnet",
        "coin": 0,
        "coingecko_id": "",
        "decimals": 8,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "is_nft": false,
        "is_spam": false,
        "logo": "btc.png",
        "name": "Bitcoin",
        "symbol": "BTC",
        "token_id": "",
        "visible": true
      }
      )"));
}

TEST(BraveWalletUtilsUnitTest, ZcashNativeAssets) {
  EXPECT_EQ(BlockchainTokenToValue(GetZcashNativeToken(mojom::kZCashMainnet)),
            ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "zcash_mainnet",
        "coin": 133,
        "coingecko_id": "zec",
        "decimals": 8,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "is_nft": false,
        "is_spam": false,
        "logo": "zec.png",
        "name": "Zcash",
        "symbol": "ZEC",
        "token_id": "",
        "visible": true
      }
      )"));

  EXPECT_EQ(BlockchainTokenToValue(GetZcashNativeToken(mojom::kZCashTestnet)),
            ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "zcash_testnet",
        "coin": 133,
        "coingecko_id": "zec",
        "decimals": 8,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "is_nft": false,
        "is_spam": false,
        "logo": "zec.png",
        "name": "Zcash",
        "symbol": "ZEC",
        "token_id": "",
        "visible": true
      }
      )"));
}

TEST(BraveWalletUtilsUnitTest, GetAllUserAssets) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(24u, assets.size());
  for (auto& asset : assets) {
    EXPECT_NE(asset->name, "");
    if (asset->symbol == "BAT") {
      EXPECT_EQ(asset->contract_address,
                "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
      EXPECT_TRUE(asset->is_erc20);
    } else {
      EXPECT_EQ(asset->contract_address, "");
      EXPECT_FALSE(asset->is_erc20);
    }
    EXPECT_FALSE(asset->is_erc721);
    EXPECT_FALSE(asset->is_erc1155);
    EXPECT_FALSE(asset->is_nft);
    EXPECT_FALSE(asset->is_spam);
    EXPECT_NE(asset->symbol, "");
    EXPECT_GT(asset->decimals, 0);
    EXPECT_TRUE(asset->visible);
    EXPECT_EQ(asset->token_id, "");
    EXPECT_NE(asset->chain_id, "");
    EXPECT_TRUE(mojom::IsKnownEnumValue(asset->coin));
  }
}

TEST(BraveWalletUtilsUnitTest, AddUserAsset) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_EQ(24u, GetAllUserAssets(&prefs).size());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();
  asset->chain_id = "0x98765";
  asset->contract_address = "0x5AAEB6053F3E94C9B9A09F33669435E7EF1BEAED";

  EXPECT_FALSE(AddUserAsset(&prefs, asset->Clone()));

  AddCustomNetwork(&prefs,
                   GetTestNetworkInfo1("0x98765", mojom::CoinType::ETH));

  EXPECT_EQ(25u, GetAllUserAssets(&prefs).size());

  ASSERT_TRUE(AddUserAsset(&prefs, asset->Clone()));

  // Address gets checksum format.
  asset->contract_address = "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed";

  EXPECT_EQ(26u, GetAllUserAssets(&prefs).size());
  EXPECT_THAT(GetAllUserAssets(&prefs), Contains(Eq(std::ref(asset))));

  // Adding same asset again fails.
  ASSERT_FALSE(AddUserAsset(&prefs, asset->Clone()));

  ASSERT_TRUE(RemoveUserAsset(&prefs, asset));

  EXPECT_EQ(25u, GetAllUserAssets(&prefs).size());
  EXPECT_THAT(GetAllUserAssets(&prefs), Not(Contains(Eq(std::ref(asset)))));

  // Invalid contract address is rejected.
  asset->coin = mojom::CoinType::ETH;
  asset->chain_id = mojom::kMainnetChainId;
  asset->contract_address = "not_eth_address";
  ASSERT_FALSE(AddUserAsset(&prefs, asset->Clone()));

  // Invalid contract address is rejected.
  asset->coin = mojom::CoinType::SOL;
  asset->chain_id = mojom::kSolanaMainnet;
  asset->contract_address = "not_base58_encoded_string";
  ASSERT_FALSE(AddUserAsset(&prefs, asset->Clone()));
}

TEST(BraveWalletUtilsUnitTest, RemoveUserAsset) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_EQ(24u, GetAllUserAssets(&prefs).size());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();

  EXPECT_TRUE(RemoveUserAsset(&prefs, asset));
  EXPECT_EQ(23u, GetAllUserAssets(&prefs).size());
  EXPECT_THAT(GetAllUserAssets(&prefs), Not(Contains(Eq(std::ref(asset)))));

  asset->chain_id = "0x98765";
  EXPECT_FALSE(RemoveUserAsset(&prefs, asset));
}

TEST(BraveWalletUtilsUnitTest, SetUserAssetVisible) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();

  EXPECT_TRUE(asset->visible);

  EXPECT_TRUE(SetUserAssetVisible(&prefs, asset, false));
  asset->visible = false;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  EXPECT_TRUE(SetUserAssetVisible(&prefs, asset, true));
  asset->visible = true;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  asset->chain_id = "0x98765";
  EXPECT_FALSE(SetUserAssetVisible(&prefs, asset, false));
}

TEST(BraveWalletUtilsUnitTest, SetAssetSpamStatus) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();

  EXPECT_FALSE(asset->is_spam);
  EXPECT_TRUE(asset->visible);

  EXPECT_TRUE(SetAssetSpamStatus(&prefs, asset, true));
  asset->is_spam = true;
  asset->visible = false;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  EXPECT_TRUE(SetAssetSpamStatus(&prefs, asset, false));
  asset->is_spam = false;
  asset->visible = true;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  asset->chain_id = "0x98765";
  EXPECT_FALSE(SetAssetSpamStatus(&prefs, asset, true));
}

}  // namespace brave_wallet
