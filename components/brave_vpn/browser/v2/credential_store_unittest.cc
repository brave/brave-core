/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_store.h"

#include <optional>
#include <string>
#include <utility>

#include "base/json/values_util.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_vpn/browser/v2/credential_store_test_util.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
using test::CredentialIs;

constexpr char kTestCredential[] = "test-credential";
constexpr char kTestSkusCredential[] = "test-skus-credential";
}  // namespace

class CredentialStoreTest : public testing::Test {
 public:
  CredentialStoreTest() {
    prefs_.registry()->RegisterDictionaryPref(
        prefs::kBraveVPNSubscriberCredential);
    prefs_.registry()->RegisterTimePref(prefs::kBraveVPNLastCredentialExpiry,
                                        base::Time());
  }

  base::Time Future() const { return base::Time::Now() + base::Days(30); }
  base::Time Past() const { return base::Time::Now() - base::Days(1); }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple prefs_;
  CredentialStore store_{&prefs_};
};

TEST_F(CredentialStoreTest, EmptyByDefault) {
  EXPECT_FALSE(store_.HasAnyCredential());
  EXPECT_FALSE(store_.GetValidSubscriberCredential().has_value());
  EXPECT_FALSE(store_.GetValidSkusCredential().has_value());
  EXPECT_FALSE(store_.IsExchangeRetried());
}

TEST_F(CredentialStoreTest, SubscriberCredentialRoundTrip) {
  store_.SetSubscriberCredential(
      {.value = kTestCredential, .expiration = Future()});

  EXPECT_TRUE(store_.HasAnyCredential());
  EXPECT_THAT(store_.GetValidSubscriberCredential(),
              CredentialIs(kTestCredential, Future()));
}

TEST_F(CredentialStoreTest, ExpiredSubscriberCredentialIsInvalid) {
  store_.SetSubscriberCredential(
      {.value = kTestCredential, .expiration = Past()});

  EXPECT_TRUE(store_.HasAnyCredential());  // present but...
  EXPECT_FALSE(
      store_.GetValidSubscriberCredential().has_value());  // ...expired.
}

TEST_F(CredentialStoreTest, SkusCredentialRoundTripAndLastExpiryStamp) {
  store_.SetSkusCredential(
      {.value = kTestSkusCredential, .expiration = Future()});

  EXPECT_THAT(store_.GetValidSkusCredential(),
              CredentialIs(kTestSkusCredential, Future()));
  EXPECT_EQ(prefs_.GetTime(prefs::kBraveVPNLastCredentialExpiry), Future());
}

// A valid credential is returned as a bundle carrying its expiration; an empty
// slot yields nullopt for both credential kinds.
TEST_F(CredentialStoreTest, ValidCredentialCarriesExpiration) {
  EXPECT_FALSE(store_.GetValidSkusCredential().has_value());
  EXPECT_FALSE(store_.GetValidSubscriberCredential().has_value());

  store_.SetSkusCredential(
      {.value = kTestSkusCredential, .expiration = Future()});
  EXPECT_THAT(store_.GetValidSkusCredential(),
              CredentialIs(kTestSkusCredential, Future()));

  store_.SetSubscriberCredential(
      {.value = kTestCredential, .expiration = Future()});
  EXPECT_THAT(store_.GetValidSubscriberCredential(),
              CredentialIs(kTestCredential, Future()));
}

TEST_F(CredentialStoreTest, SettingSubscriberDropsSkus) {
  store_.SetSkusCredential(
      {.value = kTestSkusCredential, .expiration = Future()});
  ASSERT_TRUE(store_.GetValidSkusCredential().has_value());

  store_.SetSubscriberCredential(
      {.value = kTestCredential, .expiration = Future()});

  EXPECT_TRUE(store_.GetValidSubscriberCredential().has_value());
  EXPECT_FALSE(store_.GetValidSkusCredential().has_value());
}

TEST_F(CredentialStoreTest, SettingSkusDropsSubscriber) {
  store_.SetSubscriberCredential(
      {.value = kTestCredential, .expiration = Future()});
  ASSERT_TRUE(store_.GetValidSubscriberCredential().has_value());

  store_.SetSkusCredential(
      {.value = kTestSkusCredential, .expiration = Future()});

  EXPECT_TRUE(store_.GetValidSkusCredential().has_value());
  EXPECT_FALSE(store_.GetValidSubscriberCredential().has_value());
}

TEST_F(CredentialStoreTest, ClearEmptiesTheSlot) {
  store_.SetSubscriberCredential(
      {.value = kTestCredential, .expiration = Future()});
  ASSERT_TRUE(store_.HasAnyCredential());

  store_.Clear();

  EXPECT_FALSE(store_.HasAnyCredential());
  EXPECT_FALSE(store_.GetValidSubscriberCredential().has_value());
  EXPECT_FALSE(store_.GetValidSkusCredential().has_value());
}

TEST_F(CredentialStoreTest, ExchangeRetryGuardSurvivesClear) {
  // The whole point of keeping the guard in memory: the re-fetch path calls
  // Clear(), and the guard must NOT be reset by it, or the "one extra attempt"
  // logic would loop forever.
  store_.SetExchangeRetried(true);
  store_.Clear();
  EXPECT_TRUE(store_.IsExchangeRetried());
}

TEST_F(CredentialStoreTest, ExchangeRetryGuardResettable) {
  store_.SetExchangeRetried(true);
  store_.SetExchangeRetried(false);
  EXPECT_FALSE(store_.IsExchangeRetried());
}

TEST_F(CredentialStoreTest, ReadsPreexistingV1Credential) {
  // Seamless takeover: a credential written by v1 (including a now-defunct
  // retry key in the dict) is read as valid by the v2 store.
  base::DictValue v1_dict;
  v1_dict.Set(kSubscriberCredentialKey, kTestCredential);
  v1_dict.Set(kSubscriberCredentialExpirationKey, base::TimeToValue(Future()));
  v1_dict.Set(kRetriedSkusCredentialKey, true);
  prefs_.SetDict(prefs::kBraveVPNSubscriberCredential, std::move(v1_dict));

  EXPECT_THAT(store_.GetValidSubscriberCredential(),
              CredentialIs(kTestCredential));
  // The stale persisted retry key does not feed the in-memory guard.
  EXPECT_FALSE(store_.IsExchangeRetried());
}

}  // namespace brave_vpn::v2
