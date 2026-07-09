/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_store.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
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
  EXPECT_FALSE(store_.HasValidSubscriberCredential());
  EXPECT_FALSE(store_.HasValidSkusCredential());
  EXPECT_TRUE(store_.GetSubscriberCredential().empty());
  EXPECT_TRUE(store_.GetSkusCredential().empty());
  EXPECT_FALSE(store_.GetExpirationTime().has_value());
  EXPECT_FALSE(store_.IsExchangeRetried());
}

TEST_F(CredentialStoreTest, SubscriberCredentialRoundTrip) {
  store_.SetSubscriberCredential(kTestCredential, Future());

  EXPECT_TRUE(store_.HasAnyCredential());
  EXPECT_TRUE(store_.HasValidSubscriberCredential());
  EXPECT_EQ(store_.GetSubscriberCredential(), kTestCredential);
  ASSERT_TRUE(store_.GetExpirationTime().has_value());
  EXPECT_EQ(*store_.GetExpirationTime(), Future());
}

TEST_F(CredentialStoreTest, ExpiredSubscriberCredentialIsInvalid) {
  store_.SetSubscriberCredential(kTestCredential, Past());

  EXPECT_TRUE(store_.HasAnyCredential());               // present but...
  EXPECT_FALSE(store_.HasValidSubscriberCredential());  // ...expired.
  EXPECT_TRUE(store_.GetSubscriberCredential().empty());
  EXPECT_FALSE(store_.GetExpirationTime().has_value());
}

TEST_F(CredentialStoreTest, SkusCredentialRoundTripAndLastExpiryStamp) {
  store_.SetSkusCredential(kTestSkusCredential, Future());

  EXPECT_TRUE(store_.HasValidSkusCredential());
  EXPECT_EQ(store_.GetSkusCredential(), kTestSkusCredential);
  EXPECT_EQ(prefs_.GetTime(prefs::kBraveVPNLastCredentialExpiry), Future());
}

TEST_F(CredentialStoreTest, ExpirationTimeRequiresSubscriberCredential) {
  store_.SetSkusCredential(kTestSkusCredential, Future());
  EXPECT_TRUE(store_.HasValidSkusCredential());
  EXPECT_FALSE(store_.GetExpirationTime().has_value());
}

TEST_F(CredentialStoreTest, SettingSubscriberDropsSkus) {
  store_.SetSkusCredential(kTestSkusCredential, Future());
  ASSERT_TRUE(store_.HasValidSkusCredential());

  store_.SetSubscriberCredential(kTestCredential, Future());

  EXPECT_TRUE(store_.HasValidSubscriberCredential());
  EXPECT_FALSE(store_.HasValidSkusCredential());
  EXPECT_TRUE(store_.GetSkusCredential().empty());
}

TEST_F(CredentialStoreTest, SettingSkusDropsSubscriber) {
  store_.SetSubscriberCredential(kTestCredential, Future());
  ASSERT_TRUE(store_.HasValidSubscriberCredential());

  store_.SetSkusCredential(kTestSkusCredential, Future());

  EXPECT_TRUE(store_.HasValidSkusCredential());
  EXPECT_FALSE(store_.HasValidSubscriberCredential());
  EXPECT_TRUE(store_.GetSubscriberCredential().empty());
}

TEST_F(CredentialStoreTest, ClearEmptiesTheSlot) {
  store_.SetSubscriberCredential(kTestCredential, Future());
  ASSERT_TRUE(store_.HasAnyCredential());

  store_.Clear();

  EXPECT_FALSE(store_.HasAnyCredential());
  EXPECT_FALSE(store_.HasValidSubscriberCredential());
  EXPECT_FALSE(store_.HasValidSkusCredential());
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

  EXPECT_TRUE(store_.HasValidSubscriberCredential());
  EXPECT_EQ(store_.GetSubscriberCredential(), kTestCredential);
  // The stale persisted retry key does not feed the in-memory guard.
  EXPECT_FALSE(store_.IsExchangeRetried());
}

}  // namespace brave_vpn::v2
