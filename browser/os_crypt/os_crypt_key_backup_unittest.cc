/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/os_crypt_key_backup.h"

#include <windows.h>

#include <wincrypt.h>

#include <string>
#include <string_view>
#include <vector>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "base/win/scoped_localalloc.h"
#include "chrome/browser/os_crypt/app_bound_encryption_provider_win.h"
#include "chrome/browser/os_crypt/app_bound_encryption_win.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave {

namespace {

// --- DPAPI test fixtures: real CryptProtectData round-trip. ---
//
// Matches components/os_crypt/async/browser/dpapi_key_provider_unittest.cc's
// own local `EncryptStringWithDPAPI` helper - DPAPI has no mocking seam, and
// Chromium's own tests call the real Win32 API directly, confirming it works
// fine unelevated in a normal test process.

constexpr uint8_t kDPAPIKeyPrefix[] = {'D', 'P', 'A', 'P', 'I'};

bool EncryptStringWithDPAPI(const std::string& plaintext,
                            std::string& ciphertext) {
  DATA_BLOB input = {};
  input.pbData =
      const_cast<BYTE*>(reinterpret_cast<const BYTE*>(plaintext.data()));
  input.cbData = static_cast<DWORD>(plaintext.length());

  DATA_BLOB output = {};
  if (!::CryptProtectData(&input, /*szDataDescr=*/L"",
                          /*pOptionalEntropy=*/nullptr, /*pvReserved=*/nullptr,
                          /*pPromptStruct=*/nullptr, /*dwFlags=*/0, &output)) {
    return false;
  }

  auto local_alloc = base::win::TakeLocalAlloc(output.pbData);
  ciphertext.assign(reinterpret_cast<char*>(local_alloc.get()), output.cbData);
  return true;
}

// Wraps `plaintext` the same way `os_crypt_win.cc` does: a "DPAPI" header
// followed by the real DPAPI ciphertext, base64-encoded as a whole - i.e.
// exactly the string form stored in the `os_crypt.encrypted_key` pref and in
// this backup file.
std::string WrapWithDPAPI(std::string_view plaintext) {
  std::string ciphertext;
  EXPECT_TRUE(EncryptStringWithDPAPI(std::string(plaintext), ciphertext));
  std::vector<uint8_t> wrapped(std::begin(kDPAPIKeyPrefix),
                               std::end(kDPAPIKeyPrefix));
  wrapped.insert(wrapped.end(), ciphertext.begin(), ciphertext.end());
  return base::Base64Encode(wrapped);
}

// A wrapped-looking value (correct header) whose body is not real DPAPI
// ciphertext, so `CryptUnprotectData` fails on it - simulates a corrupted
// history entry.
std::string CorruptDPAPIWrappedKey() {
  std::vector<uint8_t> wrapped(std::begin(kDPAPIKeyPrefix),
                               std::end(kDPAPIKeyPrefix));
  const std::string garbage = "not-real-dpapi-ciphertext";
  wrapped.insert(wrapped.end(), garbage.begin(), garbage.end());
  return base::Base64Encode(wrapped);
}

// --- App-bound test fixtures: mocked elevation service. ---
//
// Mirrors chrome/browser/os_crypt/app_bound_encryption_provider_win_unittest.cc
// and app_bound_encryption_win_unittest.cc's `MockAppBoundEncryptionOverrides`
// / `ScopedOverridesForTesting` shape exactly, so the seam Brave's code goes
// through (`os_crypt::DecryptAppBoundString`) is exercised the same way
// those tests exercise it.

class MockAppBoundEncryptionOverrides
    : public os_crypt::AppBoundEncryptionOverridesForTesting {
 public:
  MOCK_METHOD(HRESULT,
              EncryptAppBoundString,
              (ProtectionLevel level,
               const std::string& plaintext,
               std::string& ciphertext,
               DWORD& last_error,
               elevation_service::EncryptFlags* flags),
              (override));

  MOCK_METHOD(HRESULT,
              DecryptAppBoundString,
              (const std::string& ciphertext,
               std::string& plaintext,
               ProtectionLevel protection_level,
               std::optional<std::string>& new_ciphertext,
               DWORD& last_error,
               elevation_service::EncryptFlags* flags),
              (override));

  MOCK_METHOD(os_crypt::SupportLevel,
              GetAppBoundEncryptionSupportLevel,
              (PrefService * local_state),
              (override));
};

class ScopedOverridesForTesting {
 public:
  explicit ScopedOverridesForTesting(
      os_crypt::AppBoundEncryptionOverridesForTesting& overrides) {
    os_crypt::SetOverridesForTesting(&overrides);
  }
  ~ScopedOverridesForTesting() { os_crypt::SetOverridesForTesting(nullptr); }
};

constexpr std::string_view kAppBoundCiphertextPrefix = "SECRET";
constexpr std::string_view kAppBoundCiphertextSuffix = "DATA";

// Deterministic, non-cryptographic transform standing in for the real
// elevation-service round trip - only "does this specific wrapped value
// round-trip" matters for these tests, not real crypto semantics.
HRESULT DecryptAppBoundStringImpl(const std::string& ciphertext,
                                  std::string& plaintext,
                                  ProtectionLevel protection_level,
                                  std::optional<std::string>& new_ciphertext,
                                  DWORD& last_error,
                                  elevation_service::EncryptFlags* flags) {
  if (ciphertext.size() <
          kAppBoundCiphertextPrefix.size() + kAppBoundCiphertextSuffix.size() ||
      !ciphertext.starts_with(kAppBoundCiphertextPrefix) ||
      !ciphertext.ends_with(kAppBoundCiphertextSuffix)) {
    last_error = ERROR_INVALID_DATA;
    return E_FAIL;
  }
  plaintext =
      ciphertext.substr(kAppBoundCiphertextPrefix.size(),
                        ciphertext.size() - kAppBoundCiphertextPrefix.size() -
                            kAppBoundCiphertextSuffix.size());
  last_error = 0;
  return S_OK;
}

// Wraps `plaintext` the same way `AppBoundEncryptionProviderWin` does: the
// "APPB" header followed by ciphertext, base64-encoded as a whole.
std::string WrapAppBound(std::string_view plaintext) {
  std::string ciphertext = base::StrCat(
      {kAppBoundCiphertextPrefix, plaintext, kAppBoundCiphertextSuffix});
  std::vector<uint8_t> wrapped(
      std::begin(os_crypt_async::kCryptAppBoundKeyPrefix),
      std::end(os_crypt_async::kCryptAppBoundKeyPrefix));
  wrapped.insert(wrapped.end(), ciphertext.begin(), ciphertext.end());
  return base::Base64Encode(wrapped);
}

// A wrapped-looking value (correct header) whose body doesn't match the
// SECRET.../DATA pattern, so the mocked decrypt fails on it - simulates a
// corrupted history entry.
std::string CorruptAppBoundWrappedKey() {
  const std::string ciphertext = "garbage";
  std::vector<uint8_t> wrapped(
      std::begin(os_crypt_async::kCryptAppBoundKeyPrefix),
      std::end(os_crypt_async::kCryptAppBoundKeyPrefix));
  wrapped.insert(wrapped.end(), ciphertext.begin(), ciphertext.end());
  return base::Base64Encode(wrapped);
}

}  // namespace

class OSCryptKeyBackupTest : public ::testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ON_CALL(mock_app_bound_, DecryptAppBoundString)
        .WillByDefault(DecryptAppBoundStringImpl);
  }

 protected:
  base::FilePath path() const {
    return temp_dir_.GetPath().Append(kOSCryptKeyBackupFileName);
  }

  std::string Contents() const {
    std::string contents;
    EXPECT_TRUE(base::ReadFileToString(path(), &contents));
    return contents;
  }

  size_t EncryptedKeyHistorySize() const {
    return HistorySize("encrypted_key_history");
  }

  size_t AppBoundKeyHistorySize() const {
    return HistorySize("app_bound_encrypted_key_history");
  }

  size_t HistorySize(std::string_view list_key) const {
    std::optional<base::DictValue> root =
        base::JSONReader::ReadDict(Contents(), base::JSON_PARSE_RFC);
    if (!root) {
      return 0;
    }
    const base::DictValue* os_crypt = root->FindDict("os_crypt");
    if (!os_crypt) {
      return 0;
    }
    const base::ListValue* list = os_crypt->FindList(list_key);
    return list ? list->size() : 0;
  }

  base::ScopedTempDir temp_dir_;
  ::testing::NiceMock<MockAppBoundEncryptionOverrides> mock_app_bound_;
  ScopedOverridesForTesting overrides_{mock_app_bound_};
};

// --- Append/dedup, DPAPI side. ---

TEST_F(OSCryptKeyBackupTest, WritesWhenThereIsNoBackup) {
  const std::string wrapped = WrapWithDPAPI("key-one");
  EXPECT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), wrapped, ""));

  EXPECT_TRUE(base::PathExists(path()));
  EXPECT_EQ(1u, EncryptedKeyHistorySize());
  EXPECT_NE(std::string::npos, Contents().find(wrapped));
}

TEST_F(OSCryptKeyBackupTest, DoesNotAppendARewrappedButIdenticalKey) {
  const std::string first_wrap = WrapWithDPAPI("same-plaintext-key");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), first_wrap, ""));
  const std::string original = Contents();

  // Re-wrap the SAME plaintext key. DPAPI is non-deterministic, so this
  // produces different ciphertext bytes, but the unwrapped value is
  // identical - this is the case that proves dedup happens on the unwrapped
  // value, not the wrapped bytes.
  const std::string second_wrap = WrapWithDPAPI("same-plaintext-key");
  ASSERT_NE(first_wrap, second_wrap);

  EXPECT_EQ(OSCryptKeyBackupResult::kUpToDate,
            AppendOSCryptKeyBackupIfNew(path(), second_wrap, ""));
  EXPECT_EQ(original, Contents());
  EXPECT_EQ(1u, EncryptedKeyHistorySize());
}

TEST_F(OSCryptKeyBackupTest, AppendsAGenuinelyDifferentKey) {
  const std::string first = WrapWithDPAPI("key-one");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), first, ""));

  const std::string second = WrapWithDPAPI("key-two");
  EXPECT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), second, ""));

  EXPECT_EQ(2u, EncryptedKeyHistorySize());
  EXPECT_NE(std::string::npos, Contents().find(first));
  EXPECT_NE(std::string::npos, Contents().find(second));
}

TEST_F(OSCryptKeyBackupTest, EvictsTheOldestEntryPastTheCap) {
  const std::string first = WrapWithDPAPI("key-one");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), first, ""));
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("key-two"), ""));
  ASSERT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("key-three"), ""));
  ASSERT_EQ(3u, EncryptedKeyHistorySize());

  EXPECT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("key-four"), ""));

  EXPECT_EQ(3u, EncryptedKeyHistorySize());
  EXPECT_EQ(std::string::npos, Contents().find(first));
}

TEST_F(OSCryptKeyBackupTest, ReplacesABackupItCannotRead) {
  ASSERT_TRUE(base::WriteFile(path(), "{ this is not json"));

  const std::string wrapped = WrapWithDPAPI("key-one");
  EXPECT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), wrapped, ""));
  EXPECT_NE(std::string::npos, Contents().find(wrapped));
  EXPECT_EQ(1u, EncryptedKeyHistorySize());
}

TEST_F(OSCryptKeyBackupTest, OmitsAnAbsentAppBoundKey) {
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("key-one"), ""));
  EXPECT_EQ(std::string::npos,
            Contents().find("app_bound_encrypted_key_history"));
}

// --- Append/dedup, app-bound side (mocked elevation service). ---

TEST_F(OSCryptKeyBackupTest, AppendsAndDedupesAppBoundKeys) {
  EXPECT_CALL(mock_app_bound_, DecryptAppBoundString).Times(1);
  const std::string first = WrapAppBound("app-bound-one");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), "", first));
  ASSERT_EQ(1u, AppBoundKeyHistorySize());
  ::testing::Mock::VerifyAndClearExpectations(&mock_app_bound_);

  // Same plaintext, mocked decrypt called once for the live key, once more
  // for the (single, newest) history entry - the newest-first short-circuit
  // means it stops there rather than continuing to scan.
  EXPECT_CALL(mock_app_bound_, DecryptAppBoundString).Times(2);
  EXPECT_EQ(OSCryptKeyBackupResult::kUpToDate,
            AppendOSCryptKeyBackupIfNew(path(), "", first));
  EXPECT_EQ(1u, AppBoundKeyHistorySize());
  ::testing::Mock::VerifyAndClearExpectations(&mock_app_bound_);

  const std::string second = WrapAppBound("app-bound-two");
  EXPECT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), "", second));
  EXPECT_EQ(2u, AppBoundKeyHistorySize());
}

TEST_F(OSCryptKeyBackupTest, EvictsTheOldestAppBoundEntryPastTheCap) {
  const std::string first = WrapAppBound("app-bound-one");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), "", first));
  ASSERT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), "", WrapAppBound("app-bound-two")));
  ASSERT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), "", WrapAppBound("app-bound-three")));
  ASSERT_EQ(3u, AppBoundKeyHistorySize());

  EXPECT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), "", WrapAppBound("app-bound-four")));

  EXPECT_EQ(3u, AppBoundKeyHistorySize());
  EXPECT_EQ(std::string::npos, Contents().find(first));
}

// --- Restore ---

class OSCryptKeyRestoreTest : public OSCryptKeyBackupTest {
 public:
  void SetUp() override {
    OSCryptKeyBackupTest::SetUp();
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

  base::FilePath RestorePath() const {
    return temp_dir_.GetPath().Append(kOSCryptKeyRestoreFileName);
  }

  // The last (most recent) record appended to the restore log, or nullopt if
  // no restore was ever attempted (the log file was never written).
  std::optional<base::DictValue> LastRestoreRecord() {
    std::string contents;
    if (!base::ReadFileToString(RestorePath(), &contents)) {
      return std::nullopt;
    }
    std::optional<base::DictValue> root =
        base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
    if (!root) {
      return std::nullopt;
    }
    base::ListValue* restores = root->FindList("os_crypt_restores");
    if (!restores || restores->empty()) {
      return std::nullopt;
    }
    return restores->back().GetDict().Clone();
  }

  // Defaults to `kNotAttempted` when no restore log entry exists at all -
  // matching the production default for a key that was never attempted.
  OSCryptKeyRestoreResult RecordedResult() {
    std::optional<base::DictValue> record = LastRestoreRecord();
    if (!record) {
      return OSCryptKeyRestoreResult::kNotAttempted;
    }
    return static_cast<OSCryptKeyRestoreResult>(
        record->FindInt("dpapi_result").value_or(0));
  }

  OSCryptKeyRestoreResult RecordedAppBoundResult() {
    std::optional<base::DictValue> record = LastRestoreRecord();
    if (!record) {
      return OSCryptKeyRestoreResult::kNotAttempted;
    }
    return static_cast<OSCryptKeyRestoreResult>(
        record->FindInt("app_bound_result").value_or(0));
  }

  size_t RestoreLogSize() {
    std::string contents;
    if (!base::ReadFileToString(RestorePath(), &contents)) {
      return 0;
    }
    std::optional<base::DictValue> root =
        base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
    if (!root) {
      return 0;
    }
    base::ListValue* restores = root->FindList("os_crypt_restores");
    return restores ? restores->size() : 0;
  }

  TestingPrefServiceSimple local_state_;

 private:
  // Pin the feature on so these tests exercise restore regardless of the
  // feature's production default. `DoesNothingWhenTheFeatureIsOff` installs
  // its own local override on top of this to test the kill switch.
  base::test::ScopedFeatureList feature_list_{kBraveOSCryptKeyRestore};
};

TEST_F(OSCryptKeyRestoreTest, PutsTheKeyBackWhenItIsMissing) {
  const std::string wrapped = WrapWithDPAPI("wrapped-key");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), wrapped, ""));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(wrapped, LiveKey());
  EXPECT_TRUE(AppBoundKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedResult());
  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted, RecordedAppBoundResult());
}

// A key that is present but different cannot be told apart from a key the
// user legitimately has now, and replacing it would orphan everything
// encrypted since it arrived.
TEST_F(OSCryptKeyRestoreTest, LeavesAKeyThatIsAlreadyThereAlone) {
  ASSERT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("backed-up"), ""));
  local_state_.SetString("os_crypt.encrypted_key", "live-key");

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ("live-key", LiveKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted, RecordedResult());
}

TEST_F(OSCryptKeyRestoreTest, ReportsWhenThereIsNothingToRestoreFrom) {
  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(LiveKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kKeyMissingNoBackup, RecordedResult());
}

TEST_F(OSCryptKeyRestoreTest, ReportsAnUnusableBackup) {
  ASSERT_TRUE(base::WriteFile(path(), "{ this is not json"));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(LiveKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kKeyMissingBackupUnusable,
            RecordedResult());
}

TEST_F(OSCryptKeyRestoreTest, ReportsAnEmptyEncryptedKeyHistory) {
  ASSERT_TRUE(base::WriteFile(path(), R"({"os_crypt": {}})"));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(LiveKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kKeyMissingBackupUnusable,
            RecordedResult());
}

// A hand-edited or otherwise corrupted file could have more than the cap.
// Reading it must not let restore's per-entry verification (an
// elevation-service round trip, for app-bound) scale with an
// attacker/corruption-controlled entry count - only the newest
// `kMaxHistoryEntries` are kept on read.
TEST_F(OSCryptKeyRestoreTest, CapsAnOversizedHistoryOnRead) {
  const std::string oldest_good = WrapWithDPAPI("oldest-key");
  const std::string second_corrupt = CorruptDPAPIWrappedKey();
  const std::string third_corrupt = CorruptDPAPIWrappedKey();
  const std::string newest_corrupt = CorruptDPAPIWrappedKey();
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {"encrypted_key_history": [
        {"wrapped_key": "%s", "created": "0"},
        {"wrapped_key": "%s", "created": "1"},
        {"wrapped_key": "%s", "created": "2"},
        {"wrapped_key": "%s", "created": "3"}
      ]}})",
      oldest_good.c_str(), second_corrupt.c_str(), third_corrupt.c_str(),
      newest_corrupt.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  // The oldest entry (which would have verified) was dropped on read for
  // being past the cap, so only the newest 3 - all corrupted - are
  // considered, none of those verify, and nothing is installed.
  EXPECT_TRUE(LiveKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreFailed, RecordedResult());
}

// The DPAPI key's history being empty/unusable must not prevent an
// independently usable app-bound history from being restored - the two are
// checked separately, not gated on each other.
TEST_F(OSCryptKeyRestoreTest, RestoresAppBoundEvenWithNoEncryptedKeyHistory) {
  const std::string app_bound = WrapAppBound("app-bound-key");
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {
        "app_bound_encrypted_key_history": [
          {"wrapped_key": "%s", "created": "0"}
        ]
      }})",
      app_bound.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(LiveKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kKeyMissingBackupUnusable,
            RecordedResult());
  EXPECT_EQ(app_bound, AppBoundKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedAppBoundResult());
}

// The kill switch: with the feature off, a lost key is left lost.
TEST_F(OSCryptKeyRestoreTest, DoesNothingWhenTheFeatureIsOff) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(kBraveOSCryptKeyRestore);
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("wrapped-key"),
                                        WrapAppBound("app-bound")));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(LiveKey().empty());
  EXPECT_TRUE(AppBoundKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted, RecordedResult());
  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted, RecordedAppBoundResult());
}

// Verify newest-to-oldest: a corrupted newest entry is skipped in favor of
// an older, good one.
TEST_F(OSCryptKeyRestoreTest, SkipsACorruptedNewestEntry) {
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("good-key"), ""));
  // Hand-craft a file with a good oldest entry and a corrupted newest one,
  // simulating a rotation where the newest backup entry itself is unusable.
  const std::string good = WrapWithDPAPI("good-key");
  const std::string corrupt = CorruptDPAPIWrappedKey();
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {"encrypted_key_history": [
        {"wrapped_key": "%s", "created": "0"},
        {"wrapped_key": "%s", "created": "1"}
      ]}})",
      good.c_str(), corrupt.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(good, LiveKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedResult());
}

// When nothing in history verifies, leave the pref untouched rather than
// install a value that doesn't actually unwrap - that would permanently
// block OSCrypt's own absent/empty-pref recovery path.
TEST_F(OSCryptKeyRestoreTest, InstallsNothingWhenNoneVerify) {
  const std::string oldest_corrupt = CorruptDPAPIWrappedKey();
  const std::string newest_corrupt = CorruptDPAPIWrappedKey();
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {"encrypted_key_history": [
        {"wrapped_key": "%s", "created": "0"},
        {"wrapped_key": "%s", "created": "1"}
      ]}})",
      oldest_corrupt.c_str(), newest_corrupt.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(LiveKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreFailed, RecordedResult());
}

// The loop doesn't special-case first/last: a verified match in the middle
// of a 3-entry history is used.
TEST_F(OSCryptKeyRestoreTest, VerifiesAMiddleEntry) {
  const std::string oldest_corrupt = CorruptDPAPIWrappedKey();
  const std::string middle_good = WrapWithDPAPI("middle-key");
  const std::string newest_corrupt = CorruptDPAPIWrappedKey();
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {"encrypted_key_history": [
        {"wrapped_key": "%s", "created": "0"},
        {"wrapped_key": "%s", "created": "1"},
        {"wrapped_key": "%s", "created": "2"}
      ]}})",
      oldest_corrupt.c_str(), middle_good.c_str(), newest_corrupt.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(middle_good, LiveKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedResult());
}

// The app-bound provider re-mints its own key whenever the stored one stops
// working, so a live app-bound key can be newer than the backed-up one.
// Putting the stale one back would orphan whatever the live one encrypted.
TEST_F(OSCryptKeyRestoreTest, LeavesALiveAppBoundKeyAlone) {
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("wrapped-key"),
                                        WrapAppBound("stale-v20")));
  local_state_.SetString(os_crypt_async::kAppBoundEncryptedKeyPrefName,
                         "live-v20");

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ("live-v20", AppBoundKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedResult());
  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted, RecordedAppBoundResult());
}

TEST_F(OSCryptKeyRestoreTest, OmitsAnAppBoundKeyTheBackupDoesNotHave) {
  ASSERT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("wrapped-key"), ""));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(AppBoundKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedResult());
  EXPECT_EQ(OSCryptKeyRestoreResult::kNotAttempted, RecordedAppBoundResult());
}

// --- Restore, app-bound side (mocked elevation service). ---

TEST_F(OSCryptKeyRestoreTest, RestoresAVerifiedAppBoundKey) {
  const std::string wrapped_app_bound = WrapAppBound("app-bound-key");
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("wrapped-key"),
                                        wrapped_app_bound));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(wrapped_app_bound, AppBoundKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedAppBoundResult());
}

TEST_F(OSCryptKeyRestoreTest, SkipsACorruptedNewestAppBoundEntry) {
  const std::string good = WrapAppBound("good-app-bound");
  const std::string corrupt = CorruptAppBoundWrappedKey();
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {
        "encrypted_key_history": [{"wrapped_key": "%s", "created": "0"}],
        "app_bound_encrypted_key_history": [
          {"wrapped_key": "%s", "created": "0"},
          {"wrapped_key": "%s", "created": "1"}
        ]
      }})",
      WrapWithDPAPI("wrapped-key").c_str(), good.c_str(), corrupt.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(good, AppBoundKey());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedAppBoundResult());
}

TEST_F(OSCryptKeyRestoreTest, InstallsNoAppBoundKeyWhenNoneVerify) {
  const std::string oldest_corrupt = CorruptAppBoundWrappedKey();
  const std::string newest_corrupt = CorruptAppBoundWrappedKey();
  const std::string json = absl::StrFormat(
      R"({"os_crypt": {
        "encrypted_key_history": [{"wrapped_key": "%s", "created": "0"}],
        "app_bound_encrypted_key_history": [
          {"wrapped_key": "%s", "created": "0"},
          {"wrapped_key": "%s", "created": "1"}
        ]
      }})",
      WrapWithDPAPI("wrapped-key").c_str(), oldest_corrupt.c_str(),
      newest_corrupt.c_str());
  ASSERT_TRUE(base::WriteFile(path(), json));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(AppBoundKey().empty());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreFailed, RecordedAppBoundResult());
}

// --- Restore log durability ---

// A restore's outcome must not be lost even though it's never surfaced in
// Local State - it needs its own durable, timestamped record.
TEST_F(OSCryptKeyRestoreTest, RestoreIsRecordedInItsOwnFile) {
  ASSERT_EQ(OSCryptKeyBackupResult::kAppended,
            AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("wrapped-key"),
                                        WrapAppBound("app-bound")));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_TRUE(base::PathExists(RestorePath()));
  EXPECT_EQ(1u, RestoreLogSize());
  // Both outcomes from the same attempt land in a single record together.
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedResult());
  EXPECT_EQ(OSCryptKeyRestoreResult::kRestoreSuccess, RecordedAppBoundResult());
}

// The log is append-only: an earlier restore's record must survive a later,
// unrelated launch that also writes to the same file.
TEST_F(OSCryptKeyRestoreTest, RestoresAccumulateAcrossLaunches) {
  ASSERT_EQ(
      OSCryptKeyBackupResult::kAppended,
      AppendOSCryptKeyBackupIfNew(path(), WrapWithDPAPI("wrapped-key"), ""));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);
  ASSERT_EQ(1u, RestoreLogSize());

  // Simulate a second launch that also loses its key and restores again.
  local_state_.SetString("os_crypt.encrypted_key", "");
  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(2u, RestoreLogSize());
}

// `kKeyMissingNoBackup` and `kKeyMissingBackupUnusable` are themselves
// restore attempts (the DPAPI key was missing, which is what triggers this
// whole function) and must be recorded too, not just successful restores.
TEST_F(OSCryptKeyRestoreTest, RecordsANoBackupAttempt) {
  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(1u, RestoreLogSize());
  EXPECT_EQ(OSCryptKeyRestoreResult::kKeyMissingNoBackup, RecordedResult());
}

TEST_F(OSCryptKeyRestoreTest, RecordsAnUnusableBackupAttempt) {
  ASSERT_TRUE(base::WriteFile(path(), "{ this is not json"));

  MaybeRestoreOSCryptKey(temp_dir_.GetPath(), &local_state_);

  EXPECT_EQ(1u, RestoreLogSize());
  EXPECT_EQ(OSCryptKeyRestoreResult::kKeyMissingBackupUnusable,
            RecordedResult());
}

}  // namespace brave
