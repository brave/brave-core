/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/variations/seed_reader_writer_unittest.cc>

namespace variations {

TEST_P(SeedReaderWriterSeedFilesGroupTest,
       SetSessionCountryUpdatesCacheAndPrefWhenUsingSeedFile) {
  ASSERT_EQ(base::FieldTrialList::FindFullName(kSeedFileTrial),
            GetParam().field_trial_group);
  local_state_.SetString(GetParam().seed_fields_prefs.session_country_code,
                         "us");

  SeedReaderWriter seed_reader_writer(
      &local_state_, /*seed_file_dir=*/temp_dir_.GetPath(), kSeedFilename,
      kOldSeedFilename, GetParam().seed_fields_prefs, GetParam().channel,
      entropy_providers_.get(), GetHistogramSuffix(),
      file_writer_thread_.task_runner());

  ASSERT_EQ(seed_reader_writer.GetSeedInfo().session_country_code, "us");

  seed_reader_writer.SetSessionCountry("gb");

  EXPECT_EQ(seed_reader_writer.GetSeedInfo().session_country_code, "gb");
  EXPECT_EQ(
      local_state_.GetString(GetParam().seed_fields_prefs.session_country_code),
      "gb");
}

TEST_P(SeedReaderWriterSeedFilesGroupTest,
       SetSessionCountryDoesNotAffectGeoLevel1) {
  ASSERT_EQ(base::FieldTrialList::FindFullName(kSeedFileTrial),
            GetParam().field_trial_group);
  if (!GetParam().HasGeoLevel1Pref()) {
    return;
  }
  local_state_.SetString(GetParam().seed_fields_prefs.session_country_code,
                         "us");
  local_state_.SetString(GetParam().seed_fields_prefs.session_geo_level1,
                         "us-ny");

  SeedReaderWriter seed_reader_writer(
      &local_state_, /*seed_file_dir=*/temp_dir_.GetPath(), kSeedFilename,
      kOldSeedFilename, GetParam().seed_fields_prefs, GetParam().channel,
      entropy_providers_.get(), GetHistogramSuffix(),
      file_writer_thread_.task_runner());

  seed_reader_writer.SetSessionCountry("gb");

  EXPECT_EQ(seed_reader_writer.GetSeedInfo().session_geo_level1, "us-ny");
}

TEST_P(SeedReaderWriterSeedFilesGroupTest,
       SetSessionCountryAcceptsEmptyCountryCode) {
  ASSERT_EQ(base::FieldTrialList::FindFullName(kSeedFileTrial),
            GetParam().field_trial_group);
  local_state_.SetString(GetParam().seed_fields_prefs.session_country_code,
                         "us");

  SeedReaderWriter seed_reader_writer(
      &local_state_, /*seed_file_dir=*/temp_dir_.GetPath(), kSeedFilename,
      kOldSeedFilename, GetParam().seed_fields_prefs, GetParam().channel,
      entropy_providers_.get(), GetHistogramSuffix(),
      file_writer_thread_.task_runner());

  seed_reader_writer.SetSessionCountry("");

  EXPECT_THAT(seed_reader_writer.GetSeedInfo().session_country_code, IsEmpty());
  EXPECT_THAT(
      local_state_.GetString(GetParam().seed_fields_prefs.session_country_code),
      IsEmpty());
}

TEST_P(SeedReaderWriterLocalStateGroupsTest,
       SetSessionCountryUpdatesPrefWhenNotUsingSeedFile) {
  ASSERT_EQ(base::FieldTrialList::FindFullName(kSeedFileTrial),
            GetParam().field_trial_group);

  SeedReaderWriter seed_reader_writer(
      &local_state_, /*seed_file_dir=*/temp_dir_.GetPath(), kSeedFilename,
      kOldSeedFilename, GetParam().seed_fields_prefs, GetParam().channel,
      entropy_providers_.get(), GetHistogramSuffix(),
      file_writer_thread_.task_runner());

  local_state_.SetString(GetParam().seed_fields_prefs.session_country_code,
                         "us");

  seed_reader_writer.SetSessionCountry("gb");

  EXPECT_EQ(seed_reader_writer.GetSeedInfo().session_country_code, "gb");
  EXPECT_EQ(
      local_state_.GetString(GetParam().seed_fields_prefs.session_country_code),
      "gb");
}

}  // namespace variations
