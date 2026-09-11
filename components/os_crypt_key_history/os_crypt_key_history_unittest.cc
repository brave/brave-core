/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/os_crypt_key_history/os_crypt_key_history.h"

#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

namespace {

constexpr char kProvider[] = "dpapi";

// Test keys are written as "<key id>:<wrapping>", standing in for the way a
// platform re-wraps the same key to different bytes each time. Two stored
// values are the same key when their id matches.
std::string_view KeyId(std::string_view wrapped_key) {
  const size_t separator = wrapped_key.find(':');
  return separator == std::string_view::npos ? wrapped_key
                                             : wrapped_key.substr(0, separator);
}

OSCryptKeyHistory::SameKeyPredicate SameKeyAs(std::string_view wrapped_key) {
  return base::BindRepeating(
      [](std::string id, std::string_view stored) {
        return KeyId(stored) == id;
      },
      std::string(KeyId(wrapped_key)));
}

base::Time At(int hours) {
  return base::Time::UnixEpoch() + base::Hours(hours);
}

}  // namespace

class OSCryptKeyHistoryTest : public ::testing::Test {
 public:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

 protected:
  base::FilePath path() const {
    return temp_dir_.GetPath().AppendASCII("OSCrypt Key History");
  }

  // Records `wrapped_key` through a freshly loaded store and saves it, the way
  // a browser session would.
  void RecordInNewSession(std::string_view wrapped_key, base::Time now) {
    OSCryptKeyHistory history(path());
    history.Load();
    history.RecordVerifiedKey(kProvider, wrapped_key, now,
                              SameKeyAs(wrapped_key));
    ASSERT_TRUE(history.Save());
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(OSCryptKeyHistoryTest, NoFileIsACleanSlate) {
  OSCryptKeyHistory history(path());
  EXPECT_EQ(OSCryptKeyHistory::LoadResult::kNoFile, history.Load());
  EXPECT_TRUE(history.GetRecords(kProvider).empty());
}

TEST_F(OSCryptKeyHistoryTest, RecordsSurviveARoundTrip) {
  RecordInNewSession("k1:wrapped", At(1));

  OSCryptKeyHistory history(path());
  ASSERT_EQ(OSCryptKeyHistory::LoadResult::kLoaded, history.Load());
  ASSERT_EQ(1u, history.GetRecords(kProvider).size());
  const OSCryptKeyRecord& record = history.GetRecords(kProvider)[0];
  EXPECT_EQ("k1:wrapped", record.wrapped_key);
  EXPECT_EQ(At(1), record.first_seen);
  EXPECT_EQ(At(1), record.last_verified);
}

// Re-wrapping produces different bytes for the same key, which must not be
// mistaken for the key having changed.
TEST_F(OSCryptKeyHistoryTest, ReWrappedSameKeyOnlyRefreshesTheTimestamp) {
  RecordInNewSession("k1:first", At(1));
  RecordInNewSession("k1:rewrapped", At(5));

  OSCryptKeyHistory history(path());
  ASSERT_EQ(OSCryptKeyHistory::LoadResult::kLoaded, history.Load());
  ASSERT_EQ(1u, history.GetRecords(kProvider).size());
  const OSCryptKeyRecord& record = history.GetRecords(kProvider)[0];
  // The stored bytes are left alone; only the verification time moves.
  EXPECT_EQ("k1:first", record.wrapped_key);
  EXPECT_EQ(At(1), record.first_seen);
  EXPECT_EQ(At(5), record.last_verified);
}

TEST_F(OSCryptKeyHistoryTest, ANewKeyIsAddedAndTheOldOneIsKept) {
  RecordInNewSession("k1:wrapped", At(1));
  RecordInNewSession("k2:wrapped", At(2));

  OSCryptKeyHistory history(path());
  ASSERT_EQ(OSCryptKeyHistory::LoadResult::kLoaded, history.Load());
  ASSERT_EQ(2u, history.GetRecords(kProvider).size());
  EXPECT_EQ("k2:wrapped", history.GetRecords(kProvider)[0].wrapped_key);
  EXPECT_EQ("k1:wrapped", history.GetRecords(kProvider)[1].wrapped_key);
}

TEST_F(OSCryptKeyHistoryTest, OnlyTheNewestKeysAreKept) {
  RecordInNewSession("k1:wrapped", At(1));
  RecordInNewSession("k2:wrapped", At(2));
  RecordInNewSession("k3:wrapped", At(3));
  RecordInNewSession("k4:wrapped", At(4));

  OSCryptKeyHistory history(path());
  ASSERT_EQ(OSCryptKeyHistory::LoadResult::kLoaded, history.Load());
  ASSERT_EQ(OSCryptKeyHistory::kMaxRecordsPerProvider,
            history.GetRecords(kProvider).size());
  EXPECT_EQ("k4:wrapped", history.GetRecords(kProvider)[0].wrapped_key);
  EXPECT_EQ("k3:wrapped", history.GetRecords(kProvider)[1].wrapped_key);
  EXPECT_EQ("k2:wrapped", history.GetRecords(kProvider)[2].wrapped_key);
}

TEST_F(OSCryptKeyHistoryTest, ProvidersAreKeptApart) {
  OSCryptKeyHistory history(path());
  history.Load();
  history.RecordVerifiedKey("dpapi", "k1:wrapped", At(1),
                            SameKeyAs("k1:wrapped"));
  history.RecordVerifiedKey("app_bound", "k2:wrapped", At(1),
                            SameKeyAs("k2:wrapped"));
  ASSERT_TRUE(history.Save());

  OSCryptKeyHistory reloaded(path());
  ASSERT_EQ(OSCryptKeyHistory::LoadResult::kLoaded, reloaded.Load());
  ASSERT_EQ(1u, reloaded.GetRecords("dpapi").size());
  ASSERT_EQ(1u, reloaded.GetRecords("app_bound").size());
  EXPECT_EQ("k1:wrapped", reloaded.GetRecords("dpapi")[0].wrapped_key);
  EXPECT_EQ("k2:wrapped", reloaded.GetRecords("app_bound")[0].wrapped_key);
}

// A file we cannot make sense of must be reported as such, so that callers
// don't replace records they failed to read.
TEST_F(OSCryptKeyHistoryTest, CorruptFileIsUnreadable) {
  ASSERT_TRUE(base::WriteFile(path(), "{ this is not json"));

  OSCryptKeyHistory history(path());
  EXPECT_EQ(OSCryptKeyHistory::LoadResult::kUnreadable, history.Load());
  EXPECT_TRUE(history.GetRecords(kProvider).empty());
}

TEST_F(OSCryptKeyHistoryTest, FileFromANewerVersionIsUnreadable) {
  ASSERT_TRUE(base::WriteFile(path(), R"({"version": 99, "providers": {}})"));

  OSCryptKeyHistory history(path());
  EXPECT_EQ(OSCryptKeyHistory::LoadResult::kUnreadable, history.Load());
}

TEST_F(OSCryptKeyHistoryTest, EntriesWithoutAKeyAreSkipped) {
  ASSERT_TRUE(base::WriteFile(path(), R"({
    "version": 1,
    "providers": {"dpapi": [{"first_seen": "1"}, {"wrapped_key": "k1:ok"}]}
  })"));

  OSCryptKeyHistory history(path());
  ASSERT_EQ(OSCryptKeyHistory::LoadResult::kLoaded, history.Load());
  ASSERT_EQ(1u, history.GetRecords(kProvider).size());
  EXPECT_EQ("k1:ok", history.GetRecords(kProvider)[0].wrapped_key);
}

TEST_F(OSCryptKeyHistoryTest, AnEmptyKeyIsNotRecorded) {
  OSCryptKeyHistory history(path());
  history.Load();
  history.RecordVerifiedKey(kProvider, "", At(1), SameKeyAs(""));
  EXPECT_TRUE(history.GetRecords(kProvider).empty());
}

}  // namespace brave
