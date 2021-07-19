/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"

#include <array>

#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveWalletUnitTest : public testing::Test {
 public:
  BraveWalletUnitTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveWalletUnitTest() override {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(testing_profile_manager_.SetUp(temp_dir_.GetPath()));
  }

  PrefService* GetPrefs() {
    return ProfileManager::GetActiveUserProfile()->GetPrefs();
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager testing_profile_manager_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(BraveWalletUnitTest, TestGetRandomNonce) {
  std::string nonce = EthereumRemoteClientService::GetRandomNonce();
  ASSERT_EQ(nonce.size(), EthereumRemoteClientService::kNonceByteLength);
}

TEST_F(BraveWalletUnitTest, TestGetRandomSeed) {
  std::string seed = EthereumRemoteClientService::GetRandomSeed();
  ASSERT_EQ(seed.size(), EthereumRemoteClientService::kSeedByteLength);
}

TEST_F(BraveWalletUnitTest, TestGetEthereumRemoteClientSeedFromRootSeed) {
  constexpr std::array<uint8_t, 32> seed = {
      48, 196, 56,  174, 243, 75,  120, 235, 37,  174, 254,
      97, 37,  205, 101, 93,  181, 23,  190, 82,  53,  180,
      51, 198, 232, 187, 188, 220, 160, 187, 212, 28};
  constexpr std::array<uint8_t, 32> expected_derived_seed = {
      142, 147, 10, 180, 36,  89,  142, 110, 52,  85,  216,
      222, 83,  56, 38,  206, 104, 133, 77,  246, 219, 90,
      105, 35,  52, 76,  223, 24,  183, 138, 244, 72};
  std::string derived =
      EthereumRemoteClientService::GetEthereumRemoteClientSeedFromRootSeed(
          std::string(seed.begin(), seed.end()));
  ASSERT_EQ(derived, std::string(expected_derived_seed.begin(),
                                 expected_derived_seed.end()));
}

TEST_F(BraveWalletUnitTest, TestBitGoSeedFromRootSeed) {
  constexpr std::array<uint8_t, 32> seed = {
      48, 196, 56,  174, 243, 75,  120, 235, 37,  174, 254,
      97, 37,  205, 101, 93,  181, 23,  190, 82,  53,  180,
      51, 198, 232, 187, 188, 220, 160, 187, 212, 28};
  constexpr std::array<uint8_t, 32> expected_derived_seed = {
      101, 6,   89,  61,  129, 81,  104, 13,  48,  59, 117,
      46,  73,  177, 168, 248, 91,  84,  145, 54,  61, 157,
      27,  254, 45,  203, 71,  123, 188, 29,  224, 203};
  std::string derived = EthereumRemoteClientService::GetBitGoSeedFromRootSeed(
      std::string(seed.begin(), seed.end()));
  ASSERT_EQ(derived, std::string(expected_derived_seed.begin(),
                                 expected_derived_seed.end()));
}

TEST_F(BraveWalletUnitTest, TestSealSeed) {
  constexpr std::array<uint8_t, 32> seed = {
      48, 196, 56,  174, 243, 75,  120, 235, 37,  174, 254,
      97, 37,  205, 101, 93,  181, 23,  190, 82,  53,  180,
      51, 198, 232, 187, 188, 220, 160, 187, 212, 28};
  constexpr std::array<uint8_t, 32> key = {
      196, 34,  104, 152, 91, 63,  78,  171, 234, 163, 25,
      221, 80,  73,  158, 89, 52,  53,  227, 231, 152, 214,
      61,  210, 33,  54,  68, 171, 140, 239, 3,   158};
  constexpr std::array<uint8_t, 12> nonce = {200, 153, 224, 40,  58,  249,
                                             156, 33,  152, 207, 177, 12};
  constexpr std::array<uint8_t, 48> expected_cipher_seed = {
      33,  11,  185, 125, 67,  27,  92,  110, 132, 238, 255, 8,
      79,  7,   8,   40,  189, 211, 35,  122, 236, 183, 66,  212,
      213, 68,  187, 103, 16,  138, 166, 0,   6,   128, 179, 64,
      55,  160, 219, 8,   222, 231, 48,  93,  132, 131, 178, 177};
  std::string cipher_seed;
  ASSERT_TRUE(EthereumRemoteClientService::SealSeed(
      std::string(seed.begin(), seed.end()),
      std::string(key.begin(), key.end()),
      std::string(nonce.begin(), nonce.end()), &cipher_seed));
  ASSERT_EQ(cipher_seed, std::string(expected_cipher_seed.begin(),
                                     expected_cipher_seed.end()));
}

TEST_F(BraveWalletUnitTest, TestOpenSeed) {
  constexpr std::array<uint8_t, 48> cipher_seed = {
      33,  11,  185, 125, 67,  27,  92,  110, 132, 238, 255, 8,
      79,  7,   8,   40,  189, 211, 35,  122, 236, 183, 66,  212,
      213, 68,  187, 103, 16,  138, 166, 0,   6,   128, 179, 64,
      55,  160, 219, 8,   222, 231, 48,  93,  132, 131, 178, 177};
  constexpr std::array<uint8_t, 32> key = {
      196, 34,  104, 152, 91, 63,  78,  171, 234, 163, 25,
      221, 80,  73,  158, 89, 52,  53,  227, 231, 152, 214,
      61,  210, 33,  54,  68, 171, 140, 239, 3,   158};
  constexpr std::array<uint8_t, 12> nonce = {200, 153, 224, 40,  58,  249,
                                             156, 33,  152, 207, 177, 12};
  constexpr std::array<uint8_t, 32> expected_seed = {
      48, 196, 56,  174, 243, 75,  120, 235, 37,  174, 254,
      97, 37,  205, 101, 93,  181, 23,  190, 82,  53,  180,
      51, 198, 232, 187, 188, 220, 160, 187, 212, 28};
  std::string seed;
  ASSERT_TRUE(EthereumRemoteClientService::OpenSeed(
      std::string(cipher_seed.begin(), cipher_seed.end()),
      std::string(key.begin(), key.end()),
      std::string(nonce.begin(), nonce.end()), &seed));
  ASSERT_EQ(seed, std::string(expected_seed.begin(), expected_seed.end()));
}

TEST_F(BraveWalletUnitTest, TestLoadFromPrefs) {
  GetPrefs()->SetString(kERCAES256GCMSivNonce, "yJngKDr5nCGYz7EM");
  GetPrefs()->SetString(
      kERCEncryptedSeed,
      "IQu5fUMbXG6E7v8ITwcIKL3TI3rst0LU1US7ZxCKpgAGgLNAN6DbCN7nMF2Eg7Kx");

  std::string cipher_seed;
  std::string nonce;
  ASSERT_TRUE(EthereumRemoteClientService::LoadFromPrefs(
      ProfileManager::GetActiveUserProfile()->GetPrefs(), &cipher_seed,
      &nonce));

  constexpr std::array<uint8_t, 12> expected_nonce = {
      200, 153, 224, 40, 58, 249, 156, 33, 152, 207, 177, 12};
  constexpr std::array<uint8_t, 48> expected_cipher_seed = {
      33,  11,  185, 125, 67,  27,  92,  110, 132, 238, 255, 8,
      79,  7,   8,   40,  189, 211, 35,  122, 236, 183, 66,  212,
      213, 68,  187, 103, 16,  138, 166, 0,   6,   128, 179, 64,
      55,  160, 219, 8,   222, 231, 48,  93,  132, 131, 178, 177};
  ASSERT_EQ(nonce, std::string(expected_nonce.begin(), expected_nonce.end()));
  ASSERT_EQ(
      std::string(expected_cipher_seed.begin(), expected_cipher_seed.end()),
      cipher_seed);
}

TEST_F(BraveWalletUnitTest, TestSaveToPrefs) {
  constexpr std::array<uint8_t, 12> nonce = {200, 153, 224, 40,  58,  249,
                                             156, 33,  152, 207, 177, 12};
  constexpr std::array<uint8_t, 48> cipher_seed = {
      33,  11,  185, 125, 67,  27,  92,  110, 132, 238, 255, 8,
      79,  7,   8,   40,  189, 211, 35,  122, 236, 183, 66,  212,
      213, 68,  187, 103, 16,  138, 166, 0,   6,   128, 179, 64,
      55,  160, 219, 8,   222, 231, 48,  93,  132, 131, 178, 177};
  EthereumRemoteClientService::SaveToPrefs(
      ProfileManager::GetActiveUserProfile()->GetPrefs(),
      std::string(cipher_seed.begin(), cipher_seed.end()),
      std::string(nonce.begin(), nonce.end()));

  ASSERT_EQ(GetPrefs()->GetString(kERCAES256GCMSivNonce), "yJngKDr5nCGYz7EM");
  ASSERT_EQ(GetPrefs()->GetString(kERCEncryptedSeed),
            "IQu5fUMbXG6E7v8ITwcIKL3TI3rst0LU1US7ZxCKpgAGgLNAN6DbCN7nMF2Eg7Kx");
}
