/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_keyring.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace brave_wallet {

TEST(SolanaKeyringUnitTest, ConstructRootHDKey) {
  SolanaKeyring keyring(*MnemonicToSeed(kMnemonicScarePiece));

  EXPECT_EQ(keyring.root_->GetBase58EncodedKeypair(),
            "XUPar98T8X5HyvSw4pKk2cFi2zCMxzNcm8CJoQgDa3CjFpFKQic2cAFJhvaMgQCAQj"
            "Rs4sHHjiTqhAZ8F3tVR8D");
}

TEST(SolanaKeyringUnitTest, Accounts) {
  SolanaKeyring keyring(*MnemonicToSeed(kMnemonicScarePiece));

  keyring.AddNewHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu"));
  EXPECT_EQ(keyring.EncodePrivateKeyForExport(
                "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu"),
            "3WoEqkmeTX4BRTS3KNJCsqy7LktvEwbFSoqwMhC7xNgCG3zhwUptkT6KkJcbTpVJGX"
            "Rw9pd8CYVxZ8wLt8cUoVZb");

  keyring.AddNewHDAccount();
  keyring.AddNewHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu",
                          "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu",
                          "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY"));

  EXPECT_EQ(keyring.EncodePrivateKeyForExport(
                "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu"),
            "4pNHX6ATNXad3KZTb2PXTosW5ceaxqx45M9NH9pjcZCH9qoQKx6RMzUjuzm6J9Y2uw"
            "jCxJc5JsjL1TrGr1X3nPFP");

  // remove the last account
  keyring.RemoveLastHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu",
                          "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu"));

  keyring.AddNewHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu",
                          "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu",
                          "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY"));
  EXPECT_EQ(keyring.EncodePrivateKeyForExport(
                "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY"),
            "47rewUeufUCmtmes3uAGAo7AyM3bBYTvJdD1jQs9MGwB4eYn8SAyQUMNc9b5wFRhQy"
            "CP9WwmP7JMPAA9U9Q5E8xr");

  EXPECT_TRUE(keyring.EncodePrivateKeyForExport("brave").empty());
}

TEST(SolanaKeyringUnitTest, SignMessage) {
  SolanaKeyring keyring(*MnemonicToSeed(kMnemonicScarePiece));

  auto address = keyring.AddNewHDAccount()->address;
  EXPECT_EQ(address, "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu");

  // Message: Hello Brave
  const std::vector<uint8_t> message = {72, 101, 108, 108, 111, 32,
                                        66, 114, 97,  118, 101};
  const std::vector<uint8_t> expected_signature = {
      2,   179, 226, 40,  228, 8,   248, 176, 39,  21,  205, 26,  136,
      7,   92,  162, 178, 18,  181, 212, 58,  93,  159, 167, 207, 74,
      58,  102, 213, 60,  21,  217, 236, 188, 90,  75,  120, 116, 130,
      104, 20,  185, 45,  50,  115, 244, 223, 167, 114, 6,   225, 189,
      103, 51,  156, 215, 22,  207, 130, 197, 57,  39,  186, 12};
  auto signature = keyring.SignMessage(address, message);
  EXPECT_EQ(signature, expected_signature);
}

TEST(SolanaKeyringUnitTest, ImportAccount) {
  SolanaKeyring keyring(*MnemonicToSeed(kMnemonicScarePiece));

  std::vector<uint8_t> private_key;
  ASSERT_TRUE(base::HexStringToBytes(
      "2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e7"
      "a4b2856bfec510abab89753fac1ac0e1112364e7d250545963f135f2a33188ed",
      &private_key));
  keyring.ImportAccount(private_key);
  EXPECT_EQ(keyring.GetImportedAccountsForTesting().size(), 1u);
  EXPECT_EQ(keyring.EncodePrivateKeyForExport(
                "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ"),
            "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
            "YbQtaJQKLXET9jVjepWXe");

  private_key.clear();
  ASSERT_TRUE(base::HexStringToBytes(
      "bee602cc7dd4c1be27d8459892ab4e23f7a1d31ffde8cdd50542068ada52a201",
      &private_key));
  keyring.ImportAccount(private_key);
  EXPECT_EQ(keyring.GetImportedAccountsForTesting().size(), 2u);

  EXPECT_FALSE(keyring.RemoveImportedAccount("InvalidAddress"));
  EXPECT_TRUE(keyring.RemoveImportedAccount(
      "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ"));
  EXPECT_EQ(keyring.GetImportedAccountsForTesting().size(), 1u);
}

// Test cases from test_create_program_address in solana_program::pubkey module.
// https://docs.rs/solana-program/latest/src/solana_program/pubkey.rs.html
TEST(SolanaKeyringUnitTest, CreateProgramDerivedAddress) {
  const std::string program_id = "BPFLoaderUpgradeab1e11111111111111111111111";

  // Max seed length is 32 bytes for each seed.
  std::vector<uint8_t> exceeded_max_seed_len(32 + 1, 127);
  std::vector<uint8_t> max_seed(32, 0);

  // Max size of seeds array is 16.
  std::vector<std::vector<uint8_t>> exceeded_max_seeds = {
      {1},  {2},  {3},  {4},  {5},  {6},  {7},  {8}, {9},
      {10}, {11}, {12}, {13}, {14}, {15}, {16}, {17}};
  std::vector<std::vector<uint8_t>> max_seeds = {
      {1}, {2},  {3},  {4},  {5},  {6},  {7},  {8},
      {9}, {10}, {11}, {12}, {13}, {14}, {15}, {16}};

  EXPECT_FALSE(SolanaKeyring::CreateProgramDerivedAddress(
      {exceeded_max_seed_len}, program_id));
  EXPECT_FALSE(SolanaKeyring::CreateProgramDerivedAddress(exceeded_max_seeds,
                                                          program_id));

  auto addr = SolanaKeyring::CreateProgramDerivedAddress({{}, {1}}, program_id);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "BwqrghZA2htAcqq8dzP1WDAhTXYTYWj7CHxF5j7TDBAe");

  std::string test_string = "☉";
  addr = SolanaKeyring::CreateProgramDerivedAddress(
      {std::vector<uint8_t>(test_string.begin(), test_string.end()), {0}},
      program_id);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "13yWmRpaTR4r5nAktwLqMpRNr28tnVUZw26rTvPSSB19");

  std::vector<uint8_t> public_key;
  ASSERT_TRUE(Base58Decode("SeedPubey1111111111111111111111111111111111",
                           &public_key, kSolanaPubkeySize));
  addr =
      SolanaKeyring::CreateProgramDerivedAddress({public_key, {1}}, program_id);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "976ymqVnfE32QFe6NfGDctSvVa36LWnvYxhU6G2232YL");

  std::string talking = "Talking";
  std::string squirrels = "Squirrels";
  std::vector<uint8_t> talking_bytes =
      std::vector<uint8_t>(talking.begin(), talking.end());
  std::vector<uint8_t> squirrels_bytes =
      std::vector<uint8_t>(squirrels.begin(), squirrels.end());

  addr = SolanaKeyring::CreateProgramDerivedAddress(
      {talking_bytes, squirrels_bytes}, program_id);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "2fnQrngrQT4SeLcdToJAD96phoEjNL2man2kfRLCASVk");

  auto addr2 =
      SolanaKeyring::CreateProgramDerivedAddress({talking_bytes}, program_id);
  ASSERT_TRUE(addr2);
  EXPECT_NE(*addr, *addr2);
}

// Test cases from test_find_program_address in solana_program::pubkey module.
// https://docs.rs/solana-program/latest/src/solana_program/pubkey.rs.html
TEST(SolanaKeyringUnitTest, FindProgramDerivedAddress) {
  std::string lil = "Lil";
  std::string bits = "Bits";
  std::vector<uint8_t> lil_bytes = std::vector<uint8_t>(lil.begin(), lil.end());
  std::vector<uint8_t> bits_bytes =
      std::vector<uint8_t>(bits.begin(), bits.end());

  for (size_t i = 0; i < 1000; ++i) {
    uint8_t bump_seed = 0;
    auto addr1 = SolanaKeyring::FindProgramDerivedAddress(
        {lil_bytes, bits_bytes}, mojom::kSolanaAssociatedTokenProgramId,
        &bump_seed);
    auto addr2 = SolanaKeyring::CreateProgramDerivedAddress(
        {lil_bytes, bits_bytes, {bump_seed}},
        mojom::kSolanaAssociatedTokenProgramId);
    ASSERT_TRUE(addr1 && addr2);
    EXPECT_EQ(*addr1, *addr2);
  }
}

TEST(SolanaKeyringUnitTest, GetAssociatedTokenAccount) {
  auto addr = SolanaKeyring::GetAssociatedTokenAccount(
      "D3tynVS3dHGoShEZQcSbsJ69DnoWunhcgya35r5Dtn4p",
      "8ZETgHajbpwRr6wMjuytNvziM4VUVxfaJWhhhQoYot5T",
      mojom::SPLTokenProgram::kToken);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "5EHQ5fBsMdN3mESRhTJeEjNb3g33YWzkPBGDjoVtAGkN");

  addr = SolanaKeyring::GetAssociatedTokenAccount(
      "D3tynVS3dHGoShEZQcSbsJ69DnoWunhcgya35r5Dtn4p",
      "5ofLtZax45EhkNSkoBrDPdWNonKmijMTsW41ckzPs2r5",
      mojom::SPLTokenProgram::kToken);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "3bHK4cYoW94angdFWJeDBQcAuSq3mtYEdVaqkm1xXKcy");

  addr = SolanaKeyring::GetAssociatedTokenAccount(
      "D3tynVS3dHGoShEZQcSbsJ69DnoWunhcgya35r5Dtn4p",
      "5ofLtZax45EhkNSkoBrDPdWNonKmijMTsW41ckzPs2r5",
      mojom::SPLTokenProgram::kToken2022);
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "4h5w4Yn8egf1w2GgaR5LhC3RgZTL3rMyuCVFtb4dGyVE");

  for (auto program : {mojom::SPLTokenProgram::kUnknown,
                       mojom::SPLTokenProgram::kUnsupported}) {
    addr = SolanaKeyring::GetAssociatedTokenAccount(
        "D3tynVS3dHGoShEZQcSbsJ69DnoWunhcgya35r5Dtn4p",
        "5ofLtZax45EhkNSkoBrDPdWNonKmijMTsW41ckzPs2r5", program);
    EXPECT_FALSE(addr);
  }
}

TEST(SolanaKeyringUnitTest, GetAssociatedMetadataAccount) {
  auto addr = SolanaKeyring::GetAssociatedMetadataAccount(
      "5ZXToo7froykjvjnpHtTLYr9u2tW3USMwPg3sNkiaQVh");
  ASSERT_TRUE(addr);
  EXPECT_EQ(*addr, "6L255rMB19d544HLNumpvbdTKkTgiQ3fgMszzX6F9VAL");

  addr = SolanaKeyring::GetAssociatedMetadataAccount(
      "8q5qbP8xu1TgDWYXokwFjgTqoSNe6W3Ljj3phwqhDKqe");
  ASSERT_TRUE(addr);

  EXPECT_EQ(*addr, "586XgHr69ZhbUkkGJsQqGt16mf7jpFS6uhnvCAwb68Qq");
}

}  // namespace brave_wallet
