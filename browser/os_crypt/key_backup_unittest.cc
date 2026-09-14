/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/key_backup.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
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

}  // namespace brave
