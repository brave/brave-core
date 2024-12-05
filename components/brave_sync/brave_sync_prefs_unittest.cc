/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include <memory>

#include "base/base64.h"
#include "base/logging.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/sync/service/sync_prefs.h"
#endif

namespace brave_sync {

namespace {

constexpr char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";

}  // namespace

class BraveSyncPrefsTest : public testing::Test {
 protected:
  BraveSyncPrefsTest() {
    brave_sync::Prefs::RegisterProfilePrefs(pref_service_.registry());
    brave_sync_prefs_ = std::make_unique<brave_sync::Prefs>(&pref_service_);
#if BUILDFLAG(IS_ANDROID)
    syncer::SyncPrefs::RegisterProfilePrefs(pref_service_.registry());
    sync_prefs_ = std::make_unique<syncer::SyncPrefs>(&pref_service_);
#endif
  }

  brave_sync::Prefs* brave_sync_prefs() { return brave_sync_prefs_.get(); }

  PrefService* pref_service() { return &pref_service_; }

  base::test::SingleThreadTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<brave_sync::Prefs> brave_sync_prefs_;

#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<syncer::SyncPrefs> sync_prefs_;
#endif
};

#if BUILDFLAG(IS_APPLE)

// On macOS expected to see decryption failure when reading seed on
// locked keyring
TEST_F(BraveSyncPrefsTest, ValidPassphraseKeyringLocked) {
  OSCryptMocker::SetUp();

  brave_sync_prefs()->SetSeed(kValidSyncCode);

  bool failed_to_decrypt = false;
  OSCryptMocker::SetBackendLocked(true);
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), "");
  EXPECT_TRUE(failed_to_decrypt);

  OSCryptMocker::TearDown();
}

#endif  // BUILDFLAG(IS_APPLE)

TEST_F(BraveSyncPrefsTest, FailedToDecryptBraveSeedValue) {
  OSCryptMocker::SetUp();

  // Empty seed is expected as valid when sync is not turned on
  bool failed_to_decrypt = false;
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), "");
  EXPECT_FALSE(failed_to_decrypt);

  // Valid code does not set failed_to_decrypt
  brave_sync_prefs()->SetSeed(kValidSyncCode);
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), kValidSyncCode);
  EXPECT_FALSE(failed_to_decrypt);

  // Wrong base64-encoded seed must set failed_to_decrypt to true
  const char kWrongBase64String[] = "AA%BB";
  std::string base64_decoded;
  EXPECT_FALSE(base::Base64Decode(kWrongBase64String, &base64_decoded));
  pref_service()->SetString(brave_sync::Prefs::GetSeedPath(),
                            kWrongBase64String);
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), "");
  EXPECT_TRUE(failed_to_decrypt);

  // Valid base64 string but not valid encrypted string must set
  // failed_to_decrypt to true. Note: "v10" prefix is important to make
  // DecryptString fail. Also the remaining string must be 12 or more bytes.
  pref_service()->SetString(brave_sync::Prefs::GetSeedPath(),
                            base::Base64Encode("v10_AABBCCDDEEFF"));
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), "");
  EXPECT_TRUE(failed_to_decrypt);

  OSCryptMocker::TearDown();
}

using BraveSyncPrefsDeathTest = BraveSyncPrefsTest;

// Some tests are failing for Windows x86 CI,
// See https://github.com/brave/brave-browser/issues/22767
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_GetSeedOutNullptrCHECK DISABLED_GetSeedOutNullptrCHECK
#else
#define MAYBE_GetSeedOutNullptrCHECK GetSeedOutNullptrCHECK
#endif
TEST_F(BraveSyncPrefsDeathTest, MAYBE_GetSeedOutNullptrCHECK) {
  EXPECT_CHECK_DEATH(brave_sync_prefs()->GetSeed(nullptr));
}

TEST_F(BraveSyncPrefsTest, LeaveChainDetailsMaxLenIOS) {
  brave_sync_prefs()->SetAddLeaveChainDetailBehaviourForTests(
      brave_sync::Prefs::AddLeaveChainDetailBehaviour::kAdd);

  auto max_len = Prefs::GetLeaveChainDetailsMaxLenForTests();

  std::string details("a");
  brave_sync_prefs()->AddLeaveChainDetail("", 0, details.c_str());
  details = brave_sync_prefs()->GetLeaveChainDetails();
  EXPECT_LE(details.size(), max_len);
  EXPECT_GE(details.size(), 1u);

  details.assign(max_len + 1, 'a');
  brave_sync_prefs()->AddLeaveChainDetail(__FILE__, __LINE__, details.c_str());
  details = brave_sync_prefs()->GetLeaveChainDetails();
  EXPECT_EQ(details.size(), max_len);
}

#if BUILDFLAG(IS_ANDROID)
// This test is a modified upstream's
// SyncPrefsTest.PasswordSyncAllowed_ExplicitValue
TEST_F(BraveSyncPrefsTest, PasswordSyncAllowedExplicitValue) {
  using syncer::UserSelectableType;
  using syncer::UserSelectableTypeSet;

  // Make passwords explicitly enabled (no default value).
  sync_prefs_->SetSelectedTypesForSyncingUser(
      /*keep_everything_synced=*/false,
      /*registered_types=*/UserSelectableTypeSet::All(),
      /*selected_types=*/{UserSelectableType::kPasswords});

  sync_prefs_->SetPasswordSyncAllowed(false);

  EXPECT_TRUE(sync_prefs_->GetSelectedTypesForSyncingUser().Has(
      UserSelectableType::kPasswords));
}
#endif

}  // namespace brave_sync
