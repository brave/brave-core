/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/key_backup.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "chrome/browser/os_crypt/app_bound_encryption_provider_win.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class OSCryptKeyBackupTest : public ::testing::Test {
 public:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

 protected:
  base::FilePath path() const {
    return temp_dir_.GetPath().Append(kOSCryptKeyBackupFileName);
  }

  std::string Contents() const {
    std::string contents;
    EXPECT_TRUE(base::ReadFileToString(path(), &contents));
    return contents;
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(OSCryptKeyBackupTest, WritesWhenThereIsNoBackup) {
  EXPECT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", "app-bound"));

  EXPECT_TRUE(base::PathExists(path()));
  EXPECT_NE(std::string::npos, Contents().find("wrapped-key"));
  EXPECT_NE(std::string::npos, Contents().find("app-bound"));
}

TEST_F(OSCryptKeyBackupTest, LeavesAMatchingBackupAlone) {
  ASSERT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", ""));
  const std::string original = Contents();

  EXPECT_EQ(OSCryptKeyBackupState::kMatchesLiveKey,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", ""));
  EXPECT_EQ(original, Contents());
}

// The rule the whole design rests on: if the live key has changed, the key
// already backed up is the one worth keeping.
TEST_F(OSCryptKeyBackupTest, NeverReplacesABackupWithADifferentKey) {
  ASSERT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "original-key", ""));

  EXPECT_EQ(OSCryptKeyBackupState::kDiffersFromLiveKey,
            WriteOSCryptKeyBackupIfAbsent(path(), "replacement-key", ""));

  EXPECT_NE(std::string::npos, Contents().find("original-key"));
  EXPECT_EQ(std::string::npos, Contents().find("replacement-key"));
}

TEST_F(OSCryptKeyBackupTest, ReplacesABackupItCannotRead) {
  ASSERT_TRUE(base::WriteFile(path(), "{ this is not json"));

  EXPECT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", ""));
  EXPECT_NE(std::string::npos, Contents().find("wrapped-key"));
}

TEST_F(OSCryptKeyBackupTest, LeavesABackupFromANewerVersionAlone) {
  ASSERT_TRUE(base::WriteFile(path(), R"({"version": 99})"));

  EXPECT_EQ(OSCryptKeyBackupState::kDiffersFromLiveKey,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", ""));
  EXPECT_EQ(std::string::npos, Contents().find("wrapped-key"));
}

TEST_F(OSCryptKeyBackupTest, OmitsAnAbsentAppBoundKey) {
  ASSERT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", ""));
  EXPECT_EQ(std::string::npos, Contents().find("app_bound_encrypted_key"));
}

// --- Restore ---

class OSCryptKeyRestoreTest : public OSCryptKeyBackupTest {
 public:
  void SetUp() override {
    OSCryptKeyBackupTest::SetUp();
    RegisterOSCryptKeyBackupLocalStatePrefs(local_state_.registry());
    // Registered by OSCrypt itself in production; stubbed here.
    local_state_.registry()->RegisterStringPref("os_crypt.encrypted_key", "");
    local_state_.registry()->RegisterStringPref(
        os_crypt_async::kAppBoundEncryptedKeyPrefName, "");
  }

 protected:
  std::string LiveKey() {
    return local_state_.GetString("os_crypt.encrypted_key");
  }

  std::string AppBoundKey() {
    return local_state_.GetString(
        os_crypt_async::kAppBoundEncryptedKeyPrefName);
  }

  // What a later launch, or support, would read back.
  OSCryptKeyRestoreResult RecordedResult() {
    return static_cast<OSCryptKeyRestoreResult>(
        local_state_.GetInteger("brave.os_crypt.key_restore_result"));
  }

  TestingPrefServiceSimple local_state_;
};

// The one case restore acts on: the key is gone and a backup has it.
TEST_F(OSCryptKeyRestoreTest, PutsTheKeyBackWhenItIsMissing) {
  ASSERT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", "app-bound"));

  EXPECT_EQ(OSCryptKeyRestoreResult::kRestored,
            MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_));
  EXPECT_EQ("wrapped-key", LiveKey());
  EXPECT_EQ("app-bound", AppBoundKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestored, RecordedResult());
}

// A key that is present but different cannot be told apart from a key the user
// legitimately has now, and replacing it would orphan everything encrypted
// since it arrived.
TEST_F(OSCryptKeyRestoreTest, LeavesAKeyThatIsAlreadyThereAlone) {
  ASSERT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "backed-up-key", ""));
  local_state_.SetString("os_crypt.encrypted_key", "live-key");

  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted,
            MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_));
  EXPECT_EQ("live-key", LiveKey());
}

TEST_F(OSCryptKeyRestoreTest, ReportsWhenThereIsNothingToRestoreFrom) {
  EXPECT_EQ(OSCryptKeyRestoreResult::kNoBackup,
            MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_));
  EXPECT_TRUE(LiveKey().empty());
}

TEST_F(OSCryptKeyRestoreTest, ReportsAnUnusableBackup) {
  ASSERT_TRUE(base::WriteFile(path(), "{ this is not json"));

  EXPECT_EQ(OSCryptKeyRestoreResult::kBackupUnusable,
            MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_));
  EXPECT_TRUE(LiveKey().empty());
}

TEST_F(OSCryptKeyRestoreTest, OmitsAnAppBoundKeyTheBackupDoesNotHave) {
  ASSERT_EQ(OSCryptKeyBackupState::kCreated,
            WriteOSCryptKeyBackupIfAbsent(path(), "wrapped-key", ""));

  EXPECT_EQ(OSCryptKeyRestoreResult::kRestored,
            MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_));
  EXPECT_EQ("wrapped-key", LiveKey());
  EXPECT_TRUE(AppBoundKey().empty());
}

}  // namespace brave
