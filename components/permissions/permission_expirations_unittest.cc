/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_expirations.h"

#include <algorithm>

#include "base/stl_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/values_test_util.h"
#include "brave/components/permissions/permission_lifetime_pref_names.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

namespace {

constexpr base::StringPiece kOneTypeOneExpirationPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3"}
    ]
  }
})";

constexpr base::StringPiece kOneTypeTwoExpirationsPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3"}
    ],
    "$4": [
      {"ro": "$5"}
    ]
  }
})";

constexpr base::StringPiece kTwoTypesOneExpirationPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3"}
    ]
  },
  "$4": {
    "$5": [
      {"ro": "$6"}
    ]
  }
})";

}  // namespace

class PermissionExpirationsTest : public testing::Test {
 public:
  PermissionExpirationsTest() {}
  PermissionExpirationsTest(const PermissionExpirationsTest&) = delete;
  PermissionExpirationsTest& operator=(const PermissionExpirationsTest&) =
      delete;

  void SetUp() override {
    PermissionExpirations::RegisterProfilePrefs(
        testing_pref_service_.registry());
  }

  PrefService* prefs() { return &testing_pref_service_; }

  PermissionExpirations* expirations() {
    if (!expirations_) {
      expirations_ = std::make_unique<PermissionExpirations>(prefs());
    }
    return expirations_.get();
  }

  void ResetExpirations() {
    ASSERT_TRUE(expirations_);
    expirations_.reset();
  }

  void AddExpiringPermission(ContentSettingsType content_type,
                             base::TimeDelta time_delta,
                             const GURL& origin) {
    expirations()->AddExpiringPermission(content_type, now_ + time_delta,
                                         PermissionOrigins(origin, origin));
  }

  void CheckExpirationsPref(const base::Location& location,
                            base::StringPiece pref_value_template,
                            const std::vector<std::string>& subst = {}) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    const base::Value* expirations =
        prefs()->GetDictionary(prefs::kPermissionLifetimeExpirations);
    ASSERT_TRUE(expirations);
    EXPECT_EQ(*expirations,
              base::test::ParseJson(base::ReplaceStringPlaceholders(
                  pref_value_template, subst, nullptr)));
  }

 protected:
  const GURL kOrigin{"https://example.com"};
  const GURL kOrigin2{"https://brave.com"};
  const base::TimeDelta kLifetime{base::TimeDelta::FromSeconds(5)};
  const base::TimeDelta kOneSecond{base::TimeDelta::FromSeconds(1)};

  const base::Time now_{base::Time::Now()};

  sync_preferences::TestingPrefServiceSyncable testing_pref_service_;
  std::unique_ptr<PermissionExpirations> expirations_;
};

TEST_F(PermissionExpirationsTest, AddAndRemoveAfterExpiration) {
  const auto expiration_delta = base::TimeDelta::FromSeconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications",
       std::to_string(
           expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds()),
       kOrigin.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {PermissionOrigins(kOrigin, kOrigin)}}}));

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed on second call.
  EXPECT_TRUE(expirations()->RemoveExpiredPermissions(expiration_time).empty());
}

TEST_F(PermissionExpirationsTest, AddAndRemoveExpiring) {
  const auto expiration_delta = base::TimeDelta::FromSeconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  const auto expiration_delta2 = base::TimeDelta::FromSeconds(15);
  const auto expiration_time2 = now_ + expiration_delta2;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta2,
                        kOrigin2);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeTwoExpirationsPrefValue,
      {"notifications",
       std::to_string(
           expiration_time2.ToDeltaSinceWindowsEpoch().InMicroseconds()),
       kOrigin2.spec(),
       std::to_string(
           expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds()),
       kOrigin.spec()});

  EXPECT_TRUE(expirations()->RemoveExpiringPermissions(
      ContentSettingsType::NOTIFICATIONS,
      base::BindLambdaForTesting([&](const PermissionOrigins& origins) {
        return origins.requesting_origin() == kOrigin2;
      })));

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications",
       std::to_string(
           expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds()),
       kOrigin.spec()});

  EXPECT_TRUE(expirations()->RemoveExpiringPermissions(
      ContentSettingsType::NOTIFICATIONS,
      base::BindLambdaForTesting([&](const PermissionOrigins& origins) {
        return origins.requesting_origin() == kOrigin;
      })));

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed on second call.
  EXPECT_TRUE(expirations()->RemoveExpiredPermissions(expiration_time).empty());
}

TEST_F(PermissionExpirationsTest, RemoveExpiredDifferentTypes) {
  const auto expiration_delta = base::TimeDelta::FromSeconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  AddExpiringPermission(ContentSettingsType::GEOLOCATION, expiration_delta,
                        kOrigin);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kTwoTypesOneExpirationPrefValue,
      {"notifications",
       std::to_string(
           expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds()),
       kOrigin.spec(), "geolocation",
       std::to_string(
           expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds()),
       kOrigin.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {PermissionOrigins(kOrigin, kOrigin)}},
                          {ContentSettingsType::GEOLOCATION,
                           {PermissionOrigins(kOrigin, kOrigin)}}}));

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed on second call.
  EXPECT_TRUE(expirations()->RemoveExpiredPermissions(expiration_time).empty());
}

TEST_F(PermissionExpirationsTest, ClearInvalidContentType) {
  base::Value val(base::Value::Type::DICTIONARY);
  val.SetKey("_invalid_content_type_",
             base::Value(base::Value::Type::DICTIONARY));
  val.SetKey("another_invalid_content_type_",
             base::Value(base::Value::Type::DICTIONARY));
  prefs()->Set(prefs::kPermissionLifetimeExpirations, val);

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, R"({
    "_invalid_content_type_":{},
    "another_invalid_content_type_":{}
  })");

  // Create expirations object, it should read data from prefs and remove all
  // invalid types.
  expirations();
  EXPECT_TRUE(expirations()->expirations().empty());

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
}

}  // namespace permissions
