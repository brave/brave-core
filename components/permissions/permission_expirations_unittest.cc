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
  const GURL kOriginWithETHAccount{
      "https://example.com0x6bd1dbc7f4779d02a64053ca1b908767268c7164"};
  const GURL kOrigin2WithETHAccount{
      "https://brave1.com0x6bd1dbc7f4779d02a64053ca1b908767268c7164"};
  const GURL kOrigin3WithETHAccount{
      "https://brave2.com0x6bd1dbc7f4779d02a64053ca1b908767268c7164"};
  const GURL kOriginWithSOLAccount{
      "https://example.com__3zuhutvg3ax8vd1r9j5klr41raa1fxxqtuqyksz6kkot"};
  const GURL kOrigin2WithSOLAccount{
      "https://brave1.com__3zuhutvg3ax8vd1r9j5klr41raa1fxxqtuqyksz6kkot"};
  const GURL kOrigin3WithSOLAccount{
      "https://brave2.com__3zuhutvg3ax8vd1r9j5klr41raa1fxxqtuqyksz6kkot"};
  const base::TimeDelta kLifetime{base::Seconds(5)};
  const base::TimeDelta kOneSecond{base::Seconds(1)};

  const base::Time now_{base::Time::Now()};

  sync_preferences::TestingPrefServiceSyncable testing_pref_service_;
  std::unique_ptr<PermissionExpirations> expirations_;
};

TEST_F(PermissionExpirationsTest, AddAndRemoveAfterExpiration) {
  const struct {
    const GURL origin;
    ContentSettingsType type;
    const char* type_key;
  } cases[] = {{kOrigin, ContentSettingsType::NOTIFICATIONS, "notifications"},
               {kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
                "brave_ethereum"},
               {kOriginWithSOLAccount, ContentSettingsType::BRAVE_SOLANA,
                "brave_solana"}};

  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message() << entry.type_key << ": " << entry.origin);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    AddExpiringPermission(entry.type, expiration_delta, entry.origin);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec()});

    auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type, {MakePermissionOrigins(entry.origin)}}}));

    // Prefs data should be empty.
    CheckExpirationsPref(FROM_HERE, "{}");

    // Nothing should be removed on second call.
    EXPECT_TRUE(
        expirations()->RemoveExpiredPermissions(expiration_time).empty());
  }
}

TEST_F(PermissionExpirationsTest, AddWithAllAndRemoveDataAfterExpiration) {
  const struct {
    const GURL origin;
    const GURL origin2;
    ContentSettingsType type;
    const char* type_key;
  } cases[] = {
      {kOrigin, kOrigin2, ContentSettingsType::NOTIFICATIONS, "notifications"},
      {kOriginWithETHAccount, kOrigin2WithETHAccount,
       ContentSettingsType::BRAVE_ETHEREUM, "brave_ethereum"},
      {kOriginWithSOLAccount, kOrigin2WithSOLAccount,
       ContentSettingsType::BRAVE_SOLANA, "brave_solana"}};
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message() << entry.type_key << ": " << entry.origin
                                    << ", " << entry.origin2);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    PermissionOrigins permission_origins(entry.origin, entry.origin2,
                                         CONTENT_SETTING_BLOCK);
    expirations()->AddExpiringPermission(
        entry.type, PermissionExpirationKey(expiration_time),
        permission_origins);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationWithAllDataPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec(),
         entry.origin2.spec(), std::to_string(CONTENT_SETTING_BLOCK)});

    auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
    EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                           {{entry.type, {permission_origins}}}));

    // Prefs data should be empty.
    CheckExpirationsPref(FROM_HERE, "{}");

    // Nothing should be removed on second call.
    EXPECT_TRUE(
        expirations()->RemoveExpiredPermissions(expiration_time).empty());
  }
}

TEST_F(PermissionExpirationsTest, AddAndRemoveExpiring) {
  const struct {
    const GURL origin;
    const GURL origin2;
    ContentSettingsType type;
    const char* type_key;
  } cases[] = {
      {kOrigin, kOrigin2, ContentSettingsType::NOTIFICATIONS, "notifications"},
      {kOriginWithETHAccount, kOrigin2WithETHAccount,
       ContentSettingsType::BRAVE_ETHEREUM, "brave_ethereum"},
      {kOriginWithSOLAccount, kOrigin2WithSOLAccount,
       ContentSettingsType::BRAVE_SOLANA, "brave_solana"}};
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message() << entry.type_key << ": " << entry.origin
                                    << ", " << entry.origin2);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    AddExpiringPermission(entry.type, expiration_delta, entry.origin);
    const auto expiration_delta2 = base::Seconds(15);
    const auto expiration_time2 = now_ + expiration_delta2;
    AddExpiringPermission(entry.type, expiration_delta2, entry.origin2);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kOneTypeTwoExpirationsPrefValue,
        {entry.type_key, TimeKey(expiration_time2), entry.origin2.spec(),
         TimeKey(expiration_time), entry.origin.spec()});

    EXPECT_TRUE(expirations()->RemoveExpiringPermissions(
        entry.type,
        base::BindLambdaForTesting([&](const PermissionOrigins& origins) {
          return origins.requesting_origin() == entry.origin2;
        })));

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec()});

    EXPECT_TRUE(expirations()->RemoveExpiringPermissions(
        entry.type,
        base::BindLambdaForTesting([&](const PermissionOrigins& origins) {
          return origins.requesting_origin() == entry.origin;
        })));

    // Prefs data should be empty.
    CheckExpirationsPref(FROM_HERE, "{}");

    // Nothing should be removed on second call.
    EXPECT_TRUE(
        expirations()->RemoveExpiredPermissions(expiration_time).empty());
  }
}

TEST_F(PermissionExpirationsTest, RemoveExpiredDifferentTypes) {
  const struct {
    const GURL origin;
    ContentSettingsType type;
    const char* type_key;
    ContentSettingsType type2;
    const char* type_key2;
  } cases[] = {
      {kOrigin, ContentSettingsType::NOTIFICATIONS, "notifications",
       ContentSettingsType::GEOLOCATION, "geolocation"},
      {kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum", ContentSettingsType::BRAVE_SOLANA, "brave_solana"},
      {kOriginWithSOLAccount, ContentSettingsType::BRAVE_SOLANA, "brave_solana",
       ContentSettingsType::BRAVE_ETHEREUM, "brave_ethereum"},
  };
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message() << entry.type_key << ", " << entry.type_key2
                                    << ": " << entry.origin);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    AddExpiringPermission(entry.type, expiration_delta, entry.origin);
    AddExpiringPermission(entry.type2, expiration_delta, entry.origin);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kTwoTypesOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec(),
         entry.type_key2, TimeKey(expiration_time), entry.origin.spec()});

    auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type, {MakePermissionOrigins(entry.origin)}},
                   {entry.type2, {MakePermissionOrigins(entry.origin)}}}));

    // Prefs data should be empty.
    CheckExpirationsPref(FROM_HERE, "{}");

    // Nothing should be removed on second call.
    EXPECT_TRUE(
        expirations()->RemoveExpiredPermissions(expiration_time).empty());
  }
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
  const struct {
    const GURL origin;
    ContentSettingsType type;
    const char* type_key;
    const GURL origin2;
    ContentSettingsType type2;
    const char* type_key2;
  } cases[] = {
      {kOrigin, ContentSettingsType::NOTIFICATIONS, "notifications", kOrigin2,
       ContentSettingsType::GEOLOCATION, "geolocation"},
      {kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum", kOriginWithSOLAccount,
       ContentSettingsType::BRAVE_SOLANA, "brave_solana"},
      {kOriginWithSOLAccount, ContentSettingsType::BRAVE_SOLANA, "brave_solana",
       kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum"},
  };
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message()
                 << entry.type_key << ", " << entry.type_key2 << ": "
                 << entry.origin << ": " << entry.origin2);
    AddExpiringPermission(entry.type, entry.origin);
    AddExpiringPermission(entry.type2, entry.origin2);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kTwoTypesOneExpirationPrefValue,
        {entry.type_key, DomainKey(entry.origin), entry.origin.spec(),
         entry.type_key2, DomainKey(entry.origin2), entry.origin2.spec()});

    auto removed = expirations()->RemoveExpiredPermissions(entry.origin.host());
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type, {MakePermissionOrigins(entry.origin)}}}));

    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationPrefValue,
        {entry.type_key2, DomainKey(entry.origin2), entry.origin2.spec()});

    removed = expirations()->RemoveExpiredPermissions(entry.origin2.host());
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type2, {MakePermissionOrigins(entry.origin2)}}}));
    CheckExpirationsPref(FROM_HERE, "{}");

    // Nothing should be removed in the end.
    EXPECT_TRUE(expirations()->RemoveAllDomainPermissions().empty());
  }
}

TEST_F(PermissionExpirationsTest, RemoveDomainThenTimeExpirations) {
  const struct {
    const GURL origin;
    ContentSettingsType type;
    const char* type_key;
    const GURL origin2;
    ContentSettingsType type2;
    const char* type_key2;
  } cases[] = {
      {kOrigin, ContentSettingsType::NOTIFICATIONS, "notifications", kOrigin2,
       ContentSettingsType::GEOLOCATION, "geolocation"},
      {kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum", kOriginWithSOLAccount,
       ContentSettingsType::BRAVE_SOLANA, "brave_solana"},
      {kOriginWithSOLAccount, ContentSettingsType::BRAVE_SOLANA, "brave_solana",
       kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum"},
  };
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message()
                 << entry.type_key << ", " << entry.type_key2 << ": "
                 << entry.origin << ": " << entry.origin2);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    AddExpiringPermission(entry.type, expiration_delta, entry.origin);
    AddExpiringPermission(entry.type2, entry.origin2);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kTwoTypesOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec(),
         entry.type_key2, DomainKey(entry.origin2), entry.origin2.spec()});

    auto removed =
        expirations()->RemoveExpiredPermissions(entry.origin2.host());
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type2, {MakePermissionOrigins(entry.origin2)}}}));

    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec()});

    removed = expirations()->RemoveExpiredPermissions(expiration_time);
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type, {MakePermissionOrigins(entry.origin)}}}));
    CheckExpirationsPref(FROM_HERE, "{}");
  }
}

TEST_F(PermissionExpirationsTest, RemoveTimeThenDomainExpirations) {
  const struct {
    const GURL origin;
    ContentSettingsType type;
    const char* type_key;
    const GURL origin2;
    ContentSettingsType type2;
    const char* type_key2;
  } cases[] = {
      {kOrigin, ContentSettingsType::NOTIFICATIONS, "notifications", kOrigin2,
       ContentSettingsType::GEOLOCATION, "geolocation"},
      {kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum", kOriginWithSOLAccount,
       ContentSettingsType::BRAVE_SOLANA, "brave_solana"},
      {kOriginWithSOLAccount, ContentSettingsType::BRAVE_SOLANA, "brave_solana",
       kOriginWithETHAccount, ContentSettingsType::BRAVE_ETHEREUM,
       "brave_ethereum"},
  };
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message()
                 << entry.type_key << ", " << entry.type_key2 << ": "
                 << entry.origin << ": " << entry.origin2);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    AddExpiringPermission(entry.type, expiration_delta, entry.origin);
    AddExpiringPermission(entry.type2, entry.origin2);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kTwoTypesOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec(),
         entry.type_key2, DomainKey(entry.origin2), entry.origin2.spec()});

    auto removed = expirations()->RemoveExpiredPermissions(expiration_time);
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type, {MakePermissionOrigins(entry.origin)}}}));

    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationPrefValue,
        {entry.type_key2, DomainKey(entry.origin2), entry.origin2.spec()});

    removed = expirations()->RemoveExpiredPermissions(entry.origin2.host());
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type2, {MakePermissionOrigins(entry.origin2)}}}));
    CheckExpirationsPref(FROM_HERE, "{}");
  }
}

TEST_F(PermissionExpirationsTest, RemoveAllDomainExpirations) {
  const struct {
    const GURL origin;
    const GURL origin2;
    const GURL origin3;
    ContentSettingsType type;
    const char* type_key;
  } cases[] = {
      {kOrigin, kOrigin2, kOrigin3, ContentSettingsType::NOTIFICATIONS,
       "notifications"},
      {kOriginWithETHAccount, kOrigin2WithETHAccount, kOrigin3WithETHAccount,
       ContentSettingsType::BRAVE_ETHEREUM, "brave_ethereum"},
      {kOriginWithSOLAccount, kOrigin2WithSOLAccount, kOrigin3WithSOLAccount,
       ContentSettingsType::BRAVE_SOLANA, "brave_solana"}};
  for (const auto& entry : cases) {
    SCOPED_TRACE(testing::Message()
                 << entry.type_key << ": " << entry.origin << ", "
                 << entry.origin2 << ", " << entry.origin3);
    const auto expiration_delta = base::Seconds(10);
    const auto expiration_time = now_ + expiration_delta;
    AddExpiringPermission(entry.type, expiration_delta, entry.origin);
    AddExpiringPermission(entry.type, entry.origin2);
    AddExpiringPermission(entry.type, entry.origin3);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kOneTypeThreeExpirationsPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec(),
         DomainKey(entry.origin2), entry.origin2.spec(),
         DomainKey(entry.origin3), entry.origin3.spec()});

    auto removed = expirations()->RemoveAllDomainPermissions();
    EXPECT_EQ(removed, PermissionExpirations::ExpiredPermissions(
                           {{entry.type,
                             {{MakePermissionOrigins(entry.origin2)},
                              MakePermissionOrigins(entry.origin3)}}}));

    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationPrefValue,
        {entry.type_key, TimeKey(expiration_time), entry.origin.spec()});

    removed = expirations()->RemoveExpiredPermissions(expiration_time);
    EXPECT_EQ(removed,
              PermissionExpirations::ExpiredPermissions(
                  {{entry.type, {MakePermissionOrigins(entry.origin)}}}));

    CheckExpirationsPref(FROM_HERE, "{}");
  }
}

}  // namespace permissions
