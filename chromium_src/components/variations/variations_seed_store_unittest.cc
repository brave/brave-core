/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/variations/variations_seed_store_unittest.cc>

namespace variations {

TEST_F(VariationsSeedStoreTest,
       SetSessionCountryUpdatesCountryWhenUsingSeedFile) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithEmptyFeatureAndFieldTrialLists();
  SetUpSeedFileTrial(kSeedFilesGroup);
  ASSERT_EQ(base::FieldTrialList::FindFullName(kSeedFileTrial),
            kSeedFilesGroup);

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());

  TestVariationsSeedStore seed_store(&prefs, temp_dir.GetPath());

  seed_store.SetSessionCountry("gb");

  EXPECT_EQ(seed_store.GetSeedReaderWriterForTesting()
                ->GetSeedInfo()
                .session_country_code,
            "gb");
  EXPECT_EQ(prefs.GetString(prefs::kVariationsCountry), "gb");
}

TEST_F(VariationsSeedStoreTest,
       SetSessionCountryUpdatesCountryWhenNotUsingSeedFile) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithEmptyFeatureAndFieldTrialLists();
  SetUpSeedFileTrial(kControlGroup);
  ASSERT_EQ(base::FieldTrialList::FindFullName(kSeedFileTrial), kControlGroup);

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());

  TestVariationsSeedStore seed_store(&prefs, temp_dir.GetPath());

  seed_store.SetSessionCountry("gb");

  EXPECT_EQ(prefs.GetString(prefs::kVariationsCountry), "gb");
}

}  // namespace variations
