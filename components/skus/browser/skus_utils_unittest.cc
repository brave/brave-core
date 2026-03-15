/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_utils.h"

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/values_test_util.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace skus {

TEST(SkusUtilsUnittest, GetDefaultEnvironment) {
#if defined(OFFICIAL_BUILD)
  EXPECT_EQ(GetDefaultEnvironment(), kEnvProduction);
#else
  EXPECT_EQ(GetDefaultEnvironment(), kEnvDevelopment);
#endif
}

TEST(SkusUtilsUnittest, GetEnvironmentForDomain) {
  EXPECT_EQ(GetEnvironmentForDomain("account.brave.com"), kEnvProduction);
  EXPECT_EQ(GetEnvironmentForDomain("vpn.brave.com"), kEnvProduction);

  EXPECT_EQ(GetEnvironmentForDomain("vpn.bravesoftware.com"), kEnvStaging);
  EXPECT_EQ(GetEnvironmentForDomain("account.bravesoftware.com"), kEnvStaging);

  EXPECT_EQ(GetEnvironmentForDomain("vpn.brave.software"), kEnvDevelopment);
  EXPECT_EQ(GetEnvironmentForDomain("account.brave.software"), kEnvDevelopment);
}

TEST(SkusUtilsUnittest, GetDomain) {
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("vpn", kEnvDevelopment)),
            kEnvDevelopment);
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("talk", kEnvDevelopment)),
            kEnvDevelopment);

  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("vpn", kEnvStaging)),
            kEnvStaging);
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("talk", kEnvStaging)),
            kEnvStaging);

  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("vpn", kEnvProduction)),
            kEnvProduction);
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("talk", kEnvProduction)),
            kEnvProduction);
}

TEST(SkusUtilsUnittest, DomainIsForProduct) {
  EXPECT_EQ(DomainIsForProduct("vpn.brave.com", "vpn"), true);
  EXPECT_EQ(DomainIsForProduct("leo.brave.com", "vpn"), false);
  EXPECT_EQ(DomainIsForProduct("leon.brave.com", "leo"), false);
}

TEST(SkusUtilsUnittest, Migrate) {
  TestingPrefServiceSimple profile_pref_service;
  skus::RegisterProfilePrefsForMigration(profile_pref_service.registry());
  TestingPrefServiceSimple local_state_pref_service;
  skus::RegisterLocalStatePrefs(local_state_pref_service.registry());
  EXPECT_FALSE(local_state_pref_service.HasPrefPath(
      skus::prefs::kSkusStateMigratedToLocalState));
  EXPECT_FALSE(local_state_pref_service.HasPrefPath(skus::prefs::kSkusState));
  auto skus_settings = base::test::ParseJsonDict(R"({
    "skus":
      {
          "state":
          {
              "skus:development": "{}"
          }
      }
    })");
  profile_pref_service.SetDict(skus::prefs::kSkusState, skus_settings.Clone());
  skus::MigrateSkusSettings(&profile_pref_service, &local_state_pref_service);

  EXPECT_FALSE(profile_pref_service.HasPrefPath(skus::prefs::kSkusState));
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(skus::prefs::kSkusState));
  EXPECT_EQ(local_state_pref_service.GetDict(skus::prefs::kSkusState),
            skus_settings);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      skus::prefs::kSkusStateMigratedToLocalState));
}

TEST(SkusUtilsUnittest, AlreadyMigrated) {
  TestingPrefServiceSimple profile_pref_service;
  skus::RegisterProfilePrefsForMigration(profile_pref_service.registry());
  TestingPrefServiceSimple local_state_pref_service;
  skus::RegisterLocalStatePrefs(local_state_pref_service.registry());
  local_state_pref_service.SetBoolean(
      skus::prefs::kSkusStateMigratedToLocalState, true);
  auto existing_skus = base::test::ParseJsonDict(R"({
      "skus": {
            "state":{
              "migrated_to_local_state": true
            }
        }
      })");
  local_state_pref_service.SetDict(skus::prefs::kSkusState,
                                   existing_skus.Clone());
  auto skus_settings = base::test::ParseJsonDict(R"({
    "skus":
      {
          "state":
          {
              "development": "{}"
          }
      }
    })");
  profile_pref_service.SetDict(skus::prefs::kSkusState,
                               std::move(skus_settings));
  skus::MigrateSkusSettings(&profile_pref_service, &local_state_pref_service);

  EXPECT_TRUE(local_state_pref_service.HasPrefPath(skus::prefs::kSkusState));
  EXPECT_EQ(local_state_pref_service.GetDict(skus::prefs::kSkusState),
            existing_skus);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      skus::prefs::kSkusStateMigratedToLocalState));
}

class SkusImportTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    skus::RegisterLocalStatePrefs(local_state_.registry());
    // Store original command line to restore later.
    original_command_line_ = *base::CommandLine::ForCurrentProcess();
  }

  void TearDown() override {
    // Restore original command line.
    *base::CommandLine::ForCurrentProcess() = original_command_line_;
  }

  base::FilePath CreateTestFile(const std::string& filename,
                                const std::string& contents) {
    base::FilePath file_path = temp_dir_.GetPath().AppendASCII(filename);
    EXPECT_TRUE(base::WriteFile(file_path, contents));
    return file_path;
  }

  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple local_state_;
  base::CommandLine original_command_line_{base::CommandLine::NO_PROGRAM};
};

TEST_F(SkusImportTest, NoSwitchProvided) {
  // When no switch is provided, should return false without modifying prefs.
  base::CommandLine::ForCurrentProcess()->RemoveSwitch(
      switches::kSkusStateImportPath);

  EXPECT_FALSE(MaybeImportSkusStateFromCommandLine(&local_state_));
  EXPECT_FALSE(local_state_.HasPrefPath(skus::prefs::kSkusState));
}

TEST_F(SkusImportTest, EmptyPath) {
  // When switch is provided with empty value, should return false.
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kSkusStateImportPath, "");

  EXPECT_FALSE(MaybeImportSkusStateFromCommandLine(&local_state_));
}

TEST_F(SkusImportTest, FileNotFound) {
  // When file doesn't exist, should return false.
  base::FilePath non_existent =
      temp_dir_.GetPath().AppendASCII("non_existent.json");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kSkusStateImportPath, non_existent);

  EXPECT_FALSE(MaybeImportSkusStateFromCommandLine(&local_state_));
}

TEST_F(SkusImportTest, InvalidJson) {
  // When file contains invalid JSON, should return false.
  base::FilePath file_path = CreateTestFile("invalid.json", "not valid json {");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kSkusStateImportPath, file_path);

  EXPECT_FALSE(MaybeImportSkusStateFromCommandLine(&local_state_));
}

TEST_F(SkusImportTest, ValidJsonNoSkusKeys) {
  // When file contains valid JSON but no "skus:" keys, should return false.
  base::FilePath file_path = CreateTestFile("no_skus.json", R"({
    "env": "production",
    "other_key": "value"
  })");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kSkusStateImportPath, file_path);

  EXPECT_FALSE(MaybeImportSkusStateFromCommandLine(&local_state_));
}

TEST_F(SkusImportTest, ImportSingleEnvironment) {
  // When file contains valid skus state, should import it.
  base::FilePath file_path = CreateTestFile("skus_state.json", R"({
    "env": "production",
    "skus:production": {
      "credentials": {"token": "test_token"},
      "orders": []
    }
  })");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kSkusStateImportPath, file_path);

  EXPECT_TRUE(MaybeImportSkusStateFromCommandLine(&local_state_));
  EXPECT_TRUE(local_state_.HasPrefPath(skus::prefs::kSkusState));

  const base::Value::Dict& state =
      local_state_.GetDict(skus::prefs::kSkusState);
  const std::string* production_state = state.FindString("skus:production");
  ASSERT_TRUE(production_state);
  // The value should be a JSON string representation of the dict.
  EXPECT_TRUE(production_state->find("test_token") != std::string::npos);
}

TEST_F(SkusImportTest, ImportMultipleEnvironments) {
  // Should import multiple skus: keys.
  base::FilePath file_path = CreateTestFile("skus_multi.json", R"({
    "env": "staging",
    "skus:production": {"credentials": {"env": "prod"}},
    "skus:staging": {"credentials": {"env": "staging"}},
    "skus:development": {"credentials": {"env": "dev"}}
  })");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kSkusStateImportPath, file_path);

  EXPECT_TRUE(MaybeImportSkusStateFromCommandLine(&local_state_));

  const base::Value::Dict& state =
      local_state_.GetDict(skus::prefs::kSkusState);
  EXPECT_TRUE(state.FindString("skus:production"));
  EXPECT_TRUE(state.FindString("skus:staging"));
  EXPECT_TRUE(state.FindString("skus:development"));
  // "env" key should not be imported since it doesn't start with "skus:".
  EXPECT_FALSE(state.FindString("env"));
}

TEST_F(SkusImportTest, IgnoresNonSkusKeys) {
  // Should only import keys that start with "skus:".
  base::FilePath file_path = CreateTestFile("skus_mixed.json", R"({
    "env": "production",
    "skus:production": {"token": "valid"},
    "other_data": {"should": "ignore"},
    "skus_invalid": {"missing": "colon"}
  })");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kSkusStateImportPath, file_path);

  EXPECT_TRUE(MaybeImportSkusStateFromCommandLine(&local_state_));

  const base::Value::Dict& state =
      local_state_.GetDict(skus::prefs::kSkusState);
  EXPECT_TRUE(state.FindString("skus:production"));
  EXPECT_FALSE(state.Find("env"));
  EXPECT_FALSE(state.Find("other_data"));
  EXPECT_FALSE(state.Find("skus_invalid"));
}

}  // namespace skus
