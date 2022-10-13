/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_expirations.h"

#include <algorithm>
#include <memory>

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
      {"ro": "$3", "cs": 1}
    ]
  }
})";

constexpr base::StringPiece kOneTypeOneExpirationWithAllDataPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "eo": "$4", "cs": $5}
    ]
  }
})";

constexpr base::StringPiece kOneTypeTwoExpirationsPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": 1}
    ],
    "$4": [
      {"ro": "$5", "cs": 1}
    ]
  }
})";

constexpr base::StringPiece kOneTypeThreeExpirationsPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": 1}
    ],
    "$4": [
      {"ro": "$5", "cs": 1}
    ],
    "$6": [
      {"ro": "$7", "cs": 1}
    ]
  }
})";

constexpr base::StringPiece kTwoTypesOneExpirationPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": 1}
    ]
  },
  "$4": {
    "$5": [
      {"ro": "$6", "cs": 1}
    ]
  }
})";

}  // namespace

class PermissionExpirationsTest : public testing::Test {
 public:
  PermissionExpirationsTest() = default;
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

  static PermissionOrigins MakePermissionOrigins(const GURL& origin) {
    return PermissionOrigins(origin, origin, CONTENT_SETTING_ALLOW);
  }

  void AddExpiringPermission(ContentSettingsType content_type,
                             base::TimeDelta time_delta,
                             const GURL& origin) {
    expirations()->AddExpiringPermission(
        content_type, PermissionExpirationKey(now_ + time_delta),
        MakePermissionOrigins(origin));
  }

  void AddExpiringPermission(ContentSettingsType content_type,
                             const GURL& origin) {
    expirations()->AddExpiringPermission(content_type,
                                         PermissionExpirationKey(origin.host()),
                                         MakePermissionOrigins(origin));
  }

  void CheckExpirationsPref(const base::Location& location,
                            base::StringPiece pref_value_template,
                            const std::vector<std::string>& subst = {}) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    const auto& expirations =
        prefs()->GetDict(prefs::kPermissionLifetimeExpirations);
    EXPECT_EQ(expirations,
              base::test::ParseJsonDict(base::ReplaceStringPlaceholders(
                  pref_value_template, subst, nullptr)));
  }

  static std::string TimeKey(const base::Time& time) {
    return std::to_string(time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  }

  static std::string DomainKey(const GURL& url) { return url.host(); }

 protected:
  const GURL kOrigin{"https://example.com"};
  const GURL kOrigin2{"https://brave1.com"};
  const GURL kOrigin3{"https://brave2.com"};
  const base::TimeDelta kLifetime{base::Seconds(5)};
  const base::TimeDelta kOneSecond{base::Seconds(1)};

  const base::Time now_{base::Time::Now()};

  sync_preferences::TestingPrefServiceSyncable testing_pref_service_;
  std::unique_ptr<PermissionExpirations> expirations_;
};

TEST_F(PermissionExpirationsTest, AddAndRemoveAfterExpiration) {
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {MakePermissionOrigins(kOrigin)}}}));

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed on second call.
  EXPECT_TRUE(expirations()->RemoveExpiredPermissions(expiration_time).empty());
}

TEST_F(PermissionExpirationsTest, AddWithAllAndRemoveDataAfterExpiration) {
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  PermissionOrigins permission_origins(kOrigin, kOrigin2,
                                       CONTENT_SETTING_BLOCK);
  expirations()->AddExpiringPermission(ContentSettingsType::NOTIFICATIONS,
                                       PermissionExpirationKey(expiration_time),
                                       permission_origins);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationWithAllDataPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec(),
       kOrigin2.spec(), std::to_string(CONTENT_SETTING_BLOCK)});

  auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed,
            PermissionExpirations::ExpiredPermissions(
                {{ContentSettingsType::NOTIFICATIONS, {permission_origins}}}));

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed on second call.
  EXPECT_TRUE(expirations()->RemoveExpiredPermissions(expiration_time).empty());
}

TEST_F(PermissionExpirationsTest, AddAndRemoveExpiring) {
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  const auto expiration_delta2 = base::Seconds(15);
  const auto expiration_time2 = now_ + expiration_delta2;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta2,
                        kOrigin2);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeTwoExpirationsPrefValue,
      {"notifications", TimeKey(expiration_time2), kOrigin2.spec(),
       TimeKey(expiration_time), kOrigin.spec()});

  EXPECT_TRUE(expirations()->RemoveExpiringPermissions(
      ContentSettingsType::NOTIFICATIONS,
      base::BindLambdaForTesting([&](const PermissionOrigins& origins) {
        return origins.requesting_origin() == kOrigin2;
      })));

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec()});

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
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  AddExpiringPermission(ContentSettingsType::GEOLOCATION, expiration_delta,
                        kOrigin);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kTwoTypesOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec(), "geolocation",
       TimeKey(expiration_time), kOrigin.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {MakePermissionOrigins(kOrigin)}},
                          {ContentSettingsType::GEOLOCATION,
                           {MakePermissionOrigins(kOrigin)}}}));

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed on second call.
  EXPECT_TRUE(expirations()->RemoveExpiredPermissions(expiration_time).empty());
}

TEST_F(PermissionExpirationsTest, ClearInvalidContentType) {
  base::Value::Dict val;
  val.Set("_invalid_content_type_", base::Value::Dict());
  val.Set("another_invalid_content_type_", base::Value::Dict());
  prefs()->SetDict(prefs::kPermissionLifetimeExpirations, std::move(val));

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

TEST_F(PermissionExpirationsTest, AddRemoveDomainExpiration) {
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, kOrigin);
  AddExpiringPermission(ContentSettingsType::GEOLOCATION, kOrigin2);

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, kTwoTypesOneExpirationPrefValue,
                       {"notifications", DomainKey(kOrigin), kOrigin.spec(),
                        "geolocation", DomainKey(kOrigin2), kOrigin2.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(kOrigin.host());
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {MakePermissionOrigins(kOrigin)}}}));

  CheckExpirationsPref(FROM_HERE, kOneTypeOneExpirationPrefValue,
                       {"geolocation", DomainKey(kOrigin2), kOrigin2.spec()});

  removed = expirations()->RemoveExpiredPermissions(kOrigin2.host());
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::GEOLOCATION,
                           {MakePermissionOrigins(kOrigin2)}}}));
  CheckExpirationsPref(FROM_HERE, "{}");

  // Nothing should be removed in the end.
  EXPECT_TRUE(expirations()->RemoveAllDomainPermissions().empty());
}

TEST_F(PermissionExpirationsTest, RemoveDomainThenTimeExpirations) {
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  AddExpiringPermission(ContentSettingsType::GEOLOCATION, kOrigin2);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kTwoTypesOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec(), "geolocation",
       DomainKey(kOrigin2), kOrigin2.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(kOrigin2.host());
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::GEOLOCATION,
                           {MakePermissionOrigins(kOrigin2)}}}));

  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec()});

  removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {MakePermissionOrigins(kOrigin)}}}));
  CheckExpirationsPref(FROM_HERE, "{}");
}

TEST_F(PermissionExpirationsTest, RemoveTimeThenDomainExpirations) {
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  AddExpiringPermission(ContentSettingsType::GEOLOCATION, kOrigin2);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kTwoTypesOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec(), "geolocation",
       DomainKey(kOrigin2), kOrigin2.spec()});

  auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {MakePermissionOrigins(kOrigin)}}}));

  CheckExpirationsPref(FROM_HERE, kOneTypeOneExpirationPrefValue,
                       {"geolocation", DomainKey(kOrigin2), kOrigin2.spec()});

  removed = expirations()->RemoveExpiredPermissions(kOrigin2.host());
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::GEOLOCATION,
                           {MakePermissionOrigins(kOrigin2)}}}));
  CheckExpirationsPref(FROM_HERE, "{}");
}

TEST_F(PermissionExpirationsTest, RemoveAllDomainExpirations) {
  const auto expiration_delta = base::Seconds(10);
  const auto expiration_time = now_ + expiration_delta;
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, expiration_delta,
                        kOrigin);
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, kOrigin2);
  AddExpiringPermission(ContentSettingsType::NOTIFICATIONS, kOrigin3);

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, kOneTypeThreeExpirationsPrefValue,
                       {"notifications", TimeKey(expiration_time),
                        kOrigin.spec(), DomainKey(kOrigin2), kOrigin2.spec(),
                        DomainKey(kOrigin3), kOrigin3.spec()});

  auto removed = expirations()->RemoveAllDomainPermissions();
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {{MakePermissionOrigins(kOrigin2)},
                            MakePermissionOrigins(kOrigin3)}}}));

  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications", TimeKey(expiration_time), kOrigin.spec()});

  removed = expirations()->RemoveExpiredPermissions(expiration_time);
  EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                         {{ContentSettingsType::NOTIFICATIONS,
                           {MakePermissionOrigins(kOrigin)}}}));
  CheckExpirationsPref(FROM_HERE, "{}");
}

}  // namespace permissions
