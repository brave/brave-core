// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/components/variations/variations_seed_store_unittest.cc"

namespace variations {

// Verifies that VariationsSeedStore::UpdateSeedDateAndMaybeCountry() sets
// prefs::kVariationsSeedDate preference on the first request.
TEST_F(VariationsSeedStoreTest, UpdateSeedDateOnFirstRequest) {
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());
  TestVariationsSeedStore seed_store(&prefs);

  const base::Time seed_fetch_time = base::Time::Now();
  seed_store.UpdateSeedDateAndMaybeCountry(/*is_first_request=*/true,
                                           /*country_code=*/"FOO",
                                           seed_fetch_time);

  EXPECT_EQ(prefs.GetTime(prefs::kVariationsSeedDate), seed_fetch_time);
}

// Verifies that VariationsSeedStore::UpdateSeedDateAndMaybeCountry() sets
// prefs::kVariationsSeedDate preference on the subsequent request.
TEST_F(VariationsSeedStoreTest, UpdateSeedDateOnSubsequentRequest) {
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());
  TestVariationsSeedStore seed_store(&prefs);

  const base::Time seed_fetch_time = base::Time::Now();
  seed_store.UpdateSeedDateAndMaybeCountry(/*is_first_request=*/false,
                                           /*country_code=*/"FOO",
                                           seed_fetch_time);

  EXPECT_EQ(prefs.GetTime(prefs::kVariationsSeedDate), seed_fetch_time);
}

// Verifies that VariationsSeedStore::UpdateSeedDateAndMaybeCountry() sets
// prefs::kVariationsCountry preference on the first request.
TEST_F(VariationsSeedStoreTest, UpdateCountryOnFirstRequest) {
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());
  TestVariationsSeedStore seed_store(&prefs);
  prefs.SetString(prefs::kVariationsCountry, "FOO");

  seed_store.UpdateSeedDateAndMaybeCountry(/*is_first_request=*/true,
                                           /*country_code=*/"BAR",
                                           base::Time::Now());

  EXPECT_EQ(prefs.GetString(prefs::kVariationsCountry), "BAR");
}

// Verifies that VariationsSeedStore::UpdateSeedDateAndMaybeCountry() does not
// update prefs::kVariationsCountry preference if the country code is empty.
TEST_F(VariationsSeedStoreTest, DoNotUpdateCountryIfEmpty) {
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());
  TestVariationsSeedStore seed_store(&prefs);
  prefs.SetString(prefs::kVariationsCountry, "FOO");

  seed_store.UpdateSeedDateAndMaybeCountry(/*is_first_request=*/true,
                                           /*country_code=*/"",
                                           base::Time::Now());

  EXPECT_EQ(prefs.GetString(prefs::kVariationsCountry), "FOO");
}

// Verifies that VariationsSeedStore::UpdateSeedDateAndMaybeCountry() does not
// update prefs::kVariationsCountry preference on the subsequent request.
TEST_F(VariationsSeedStoreTest, DoNotUpdateCountryOnSubsequentRequest) {
  TestingPrefServiceSimple prefs;
  VariationsSeedStore::RegisterPrefs(prefs.registry());
  TestVariationsSeedStore seed_store(&prefs);
  prefs.SetString(prefs::kVariationsCountry, "FOO");

  seed_store.UpdateSeedDateAndMaybeCountry(/*is_first_request=*/false,
                                           /*country_code=*/"BAR",
                                           base::Time::Now());

  EXPECT_EQ(prefs.GetString(prefs::kVariationsCountry), "FOO");
}

}  // namespace variations
