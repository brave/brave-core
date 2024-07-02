/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_manager.h"

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/permissions/permission_lifetime_pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using content_settings::WebsiteSettingsInfo;
using content_settings::WebsiteSettingsRegistry;
using testing::_;

namespace permissions {

namespace {

using PermissionDecidedCallback = PermissionRequest::PermissionDecidedCallback;

constexpr std::string_view kOneTypeOneExpirationPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": 1}
    ]
  }
})";

constexpr std::string_view kOneTypeOneExpirationWithCsPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": $4}
    ]
  }
})";

constexpr std::string_view kOneTypeSameTimeExpirationsPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": 1},
      {"ro": "$4", "cs": 1}
    ],
  }
})";

constexpr std::string_view kOneTypeTwoExpirationsPrefValue = R"({
  "$1": {
    "$2": [
      {"ro": "$3", "cs": 1}
    ],
    "$4": [
      {"ro": "$5", "cs": 1}
    ]
  }
})";

constexpr std::string_view kTwoTypesOneExpirationPrefValue = R"({
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

class MockPermissionOriginLifetimeMonitor
    : public PermissionOriginLifetimeMonitor {
 public:
  MockPermissionOriginLifetimeMonitor() {
    ON_CALL(*this, SetOnPermissionOriginDestroyedCallback(_))
        .WillByDefault(
            [this](base::RepeatingCallback<void(const std::string&)> callback) {
              origin_destroyed_callback_ = std::move(callback);
            });
  }

  MOCK_METHOD(void,
              SetOnPermissionOriginDestroyedCallback,
              (base::RepeatingCallback<void(const std::string&)> callback),
              (override));
  MOCK_METHOD(std::string,
              SubscribeToPermissionOriginDestruction,
              (const GURL& requesting_origin),
              (override));

  void NotifyOriginDestroyed(const std::string& origin) {
    ASSERT_TRUE(origin_destroyed_callback_);
    origin_destroyed_callback_.Run(origin);
  }

 private:
  base::RepeatingCallback<void(const std::string&)> origin_destroyed_callback_;
};

}  // namespace

class PermissionLifetimeManagerTest : public testing::Test {
 public:
  PermissionLifetimeManagerTest()
      : browser_task_environment_(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  PermissionLifetimeManagerTest(const PermissionLifetimeManagerTest&) = delete;
  PermissionLifetimeManagerTest& operator=(
      const PermissionLifetimeManagerTest&) = delete;

  void SetUp() override {
    host_content_settings_map_ =
        HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

  void TearDown() override { manager_.reset(); }

  PrefService* prefs() { return profile_.GetPrefs(); }

  virtual std::unique_ptr<PermissionOriginLifetimeMonitor>
  GetPermissionOriginLifetimeMonitor() {
    return nullptr;
  }

  PermissionLifetimeManager* manager() {
    if (!manager_) {
      manager_ = std::make_unique<PermissionLifetimeManager>(
          *host_content_settings_map_, prefs(),
          GetPermissionOriginLifetimeMonitor());
    }
    return manager_.get();
  }

  virtual void ResetManager() {
    ASSERT_TRUE(manager_);
    manager_->Shutdown();
    manager_.reset();
  }

  const base::WallClockTimer& timer() { return *manager()->expiration_timer_; }

  std::unique_ptr<PermissionRequest> CreateRequestAndChooseContentSetting(
      const GURL& origin,
      ContentSettingsType content_type,
      base::TimeDelta lifetime,
      ContentSetting content_setting) {
    EXPECT_EQ(host_content_settings_map_->GetContentSetting(origin, origin,
                                                            content_type),
              GetDefaultContentSetting(content_type));
    host_content_settings_map_->SetContentSettingDefaultScope(
        origin, origin, content_type, content_setting);
    EXPECT_EQ(host_content_settings_map_->GetContentSetting(origin, origin,
                                                            content_type),
              content_setting);
    ExpectContentSetting(FROM_HERE, origin, content_type, content_setting);

    auto request = std::make_unique<PermissionRequest>(
        origin, ContentSettingsTypeToRequestType(content_type), true,
        PermissionDecidedCallback(), base::OnceClosure());
    request->SetLifetime(lifetime);
    return request;
  }

  std::unique_ptr<PermissionRequest> CreateRequestAndAllowContentSetting(
      const GURL& origin,
      ContentSettingsType content_type,
      base::TimeDelta lifetime) {
    return CreateRequestAndChooseContentSetting(
        origin, content_type, lifetime, ContentSetting::CONTENT_SETTING_ALLOW);
  }

  ContentSetting GetDefaultContentSetting(
      ContentSettingsType content_type) const {
    return host_content_settings_map_->GetDefaultContentSetting(content_type,
                                                                nullptr);
  }

  ContentSetting GetContentSetting(const GURL& origin,
                                   ContentSettingsType content_type) const {
    return host_content_settings_map_->GetContentSetting(origin, origin,
                                                         content_type);
  }

  void ExpectContentSetting(const base::Location& location,
                            const GURL& origin,
                            ContentSettingsType content_type,
                            ContentSetting content_setting) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    EXPECT_EQ(GetContentSetting(origin, content_type),
              content_setting == ContentSetting::CONTENT_SETTING_DEFAULT
                  ? GetDefaultContentSetting(content_type)
                  : content_setting);
  }

  void CheckExpirationsPref(const base::Location& location,
                            std::string_view pref_value_template,
                            const std::vector<std::string>& subst = {}) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    const auto& expirations =
        prefs()->GetDict(prefs::kPermissionLifetimeExpirations);
    EXPECT_EQ(expirations,
              base::test::ParseJsonDict(base::ReplaceStringPlaceholders(
                  pref_value_template, subst, nullptr)));
  }

 protected:
  const GURL kOrigin{"https://example.com"};
  const GURL kOrigin2{"https://brave.com"};
  const base::TimeDelta kLifetime{base::Seconds(5)};
  const base::TimeDelta kOneSecond{base::Seconds(1)};

  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  std::unique_ptr<PermissionLifetimeManager> manager_;
};

TEST_F(PermissionLifetimeManagerTest, SetAndResetAfterExpiration) {
  for (ContentSetting content_setting :
       {CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK}) {
    auto request(CreateRequestAndChooseContentSetting(
        kOrigin, ContentSettingsType::NOTIFICATIONS, kLifetime,
        content_setting));
    const base::Time expected_expiration_time =
        base::Time::Now() + *request->GetLifetime();
    manager()->PermissionDecided(*request, kOrigin, kOrigin, content_setting,
                                 false);
    EXPECT_TRUE(timer().IsRunning());

    browser_task_environment_.RunUntilIdle();
    // Setting should be intact.
    ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                         content_setting);
    // Forward time a little, setting still should be intact.
    browser_task_environment_.FastForwardBy(kOneSecond);
    ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                         content_setting);

    // Check data stored in prefs.
    CheckExpirationsPref(
        FROM_HERE, kOneTypeOneExpirationWithCsPrefValue,
        {"notifications",
         std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                            .InMicroseconds()),
         kOrigin.spec(), std::to_string(content_setting)});

    // Forward time, this should trigger a setting reset to default state.
    browser_task_environment_.FastForwardBy(*request->GetLifetime() -
                                            kOneSecond);
    ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                         ContentSetting::CONTENT_SETTING_DEFAULT);

    // Prefs data should be empty.
    CheckExpirationsPref(FROM_HERE, "{}");
    EXPECT_FALSE(timer().IsRunning());
  }
}

TEST_F(PermissionLifetimeManagerTest, DifferentTypePermissions) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, kLifetime));
  const base::Time expected_expiration_time =
      base::Time::Now() + *request->GetLifetime();
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_TRUE(timer().IsRunning());

  browser_task_environment_.FastForwardBy(kOneSecond);

  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::GEOLOCATION, kLifetime));
  const base::Time expected_expiration_time2 =
      base::Time::Now() + *request2->GetLifetime();
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  browser_task_environment_.RunUntilIdle();

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kTwoTypesOneExpirationPrefValue,
      {"notifications",
       std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin.spec(), "geolocation",
       std::to_string(expected_expiration_time2.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin2.spec()});

  // Forward time, this should trigger a first setting reset to default state.
  browser_task_environment_.FastForwardBy(*request->GetLifetime() - kOneSecond);
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::GEOLOCATION,
                       ContentSetting::CONTENT_SETTING_ALLOW);
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"geolocation",
       std::to_string(expected_expiration_time2.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin2.spec()});

  browser_task_environment_.FastForwardBy(kOneSecond);
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::GEOLOCATION,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
  EXPECT_FALSE(timer().IsRunning());
}

TEST_F(PermissionLifetimeManagerTest, TwoPermissionsSameTime) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, kLifetime));
  const base::Time expected_expiration_time =
      base::Time::Now() + *request->GetLifetime();
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_TRUE(timer().IsRunning());

  browser_task_environment_.FastForwardBy(kOneSecond);
  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::NOTIFICATIONS, kLifetime - kOneSecond));
  const base::Time expected_expiration_time2 =
      base::Time::Now() + *request2->GetLifetime();
  ASSERT_EQ(expected_expiration_time, expected_expiration_time2);
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeSameTimeExpirationsPrefValue,
      {"notifications",
       std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin.spec(), kOrigin2.spec()});

  // Forward time, this should trigger a setting reset to default state.
  browser_task_environment_.FastForwardBy(*request->GetLifetime() - kOneSecond);
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
  EXPECT_FALSE(timer().IsRunning());
}

TEST_F(PermissionLifetimeManagerTest, TwoPermissionsBigTimeDifference) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, base::Days(5)));
  const base::Time expected_expiration_time =
      base::Time::Now() + *request->GetLifetime();
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_TRUE(timer().IsRunning());
  EXPECT_EQ(timer().desired_run_time(), expected_expiration_time);

  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::NOTIFICATIONS, kLifetime));
  const base::Time expected_expiration_time2 =
      base::Time::Now() + *request2->GetLifetime();
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  // Timer should be restarted.
  EXPECT_EQ(timer().desired_run_time(), expected_expiration_time2);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeTwoExpirationsPrefValue,
      {"notifications",
       std::to_string(expected_expiration_time2.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin2.spec(),
       std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin.spec()});

  // Forward time, this should trigger a setting reset to default state.
  browser_task_environment_.FastForwardBy(*request2->GetLifetime());
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_ALLOW);

  browser_task_environment_.FastForwardBy(*request->GetLifetime());
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
  EXPECT_FALSE(timer().IsRunning());
}

TEST_F(PermissionLifetimeManagerTest, RestoreAfterRestart) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, kLifetime));
  const base::Time expected_expiration_time =
      base::Time::Now() + *request->GetLifetime();
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_TRUE(timer().IsRunning());

  ResetManager();
  // This will create a new PermissionLifetimeManager instance.
  ASSERT_TRUE(manager());
  // Timer should be running.
  EXPECT_TRUE(timer().IsRunning());

  browser_task_environment_.RunUntilIdle();
  // Setting should be intact.
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_ALLOW);
  // Forward time a little, setting still should be intact.
  browser_task_environment_.FastForwardBy(kOneSecond);
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_ALLOW);

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications",
       std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin.spec()});

  // Forward time, this should trigger a setting reset to default state.
  browser_task_environment_.FastForwardBy(*request->GetLifetime() - kOneSecond);
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
  EXPECT_FALSE(timer().IsRunning());
}

TEST_F(PermissionLifetimeManagerTest, ExpiredRestoreAfterRestart) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, kLifetime));
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_TRUE(timer().IsRunning());

  ResetManager();
  browser_task_environment_.FastForwardBy(kLifetime);

  // This will create a new PermissionLifetimeManager instance.
  ASSERT_TRUE(manager());
  // Timer should not be running.
  EXPECT_FALSE(timer().IsRunning());

  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
}

TEST_F(PermissionLifetimeManagerTest, PartiallyExpiredRestoreAfterRestart) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, base::Days(5)));
  const base::Time expected_expiration_time =
      base::Time::Now() + *request->GetLifetime();
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);

  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::NOTIFICATIONS, kLifetime));
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);

  ResetManager();
  browser_task_environment_.FastForwardBy(kLifetime);
  // This will create a new PermissionLifetimeManager instance.
  ASSERT_TRUE(manager());
  // Timer should be running.
  EXPECT_TRUE(timer().IsRunning());

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeOneExpirationPrefValue,
      {"notifications",
       std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin.spec()});

  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_ALLOW);

  browser_task_environment_.FastForwardBy(*request->GetLifetime());
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
  EXPECT_FALSE(timer().IsRunning());
}

TEST_F(PermissionLifetimeManagerTest, ExternalContentSettingChange) {
  for (auto external_content_setting :
       {ContentSetting::CONTENT_SETTING_DEFAULT,
        ContentSetting::CONTENT_SETTING_BLOCK}) {
    auto request(CreateRequestAndAllowContentSetting(
        kOrigin, ContentSettingsType::GEOLOCATION, kLifetime));
    manager()->PermissionDecided(*request, kOrigin, kOrigin,
                                 ContentSetting::CONTENT_SETTING_ALLOW, false);
    EXPECT_TRUE(timer().IsRunning());

    host_content_settings_map_->SetContentSettingDefaultScope(
        kOrigin, kOrigin, ContentSettingsType::GEOLOCATION,
        external_content_setting);
    EXPECT_FALSE(timer().IsRunning());

    ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::GEOLOCATION,
                         external_content_setting);

    // Prefs data should be empty.
    CheckExpirationsPref(FROM_HERE, "{}");
  }
}

TEST_F(PermissionLifetimeManagerTest, ClearAllExpiredAfterRestart) {
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, kOneSecond));
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);

  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::NOTIFICATIONS, kLifetime));
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);

  ResetManager();
  browser_task_environment_.FastForwardBy(kLifetime);
  // This will create a new PermissionLifetimeManager instance.
  ASSERT_TRUE(manager());
  // Timer should not be running.
  EXPECT_FALSE(timer().IsRunning());

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, "{}");

  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
}

class PermissionLifetimeManagerWithOriginMonitorTest
    : public PermissionLifetimeManagerTest {
 public:
  std::unique_ptr<PermissionOriginLifetimeMonitor>
  GetPermissionOriginLifetimeMonitor() override {
    auto monitor = std::make_unique<MockPermissionOriginLifetimeMonitor>();
    origin_lifetime_monitor_ = monitor.get();
    EXPECT_CALL(*origin_lifetime_monitor_,
                SetOnPermissionOriginDestroyedCallback(_));
    return monitor;
  }

  void TearDown() override {
    origin_lifetime_monitor_ = nullptr;
    PermissionLifetimeManagerTest::TearDown();
  }

  void ResetManager() override {
    origin_lifetime_monitor_ = nullptr;
    PermissionLifetimeManagerTest::ResetManager();
  }

 protected:
  raw_ptr<MockPermissionOriginLifetimeMonitor> origin_lifetime_monitor_ =
      nullptr;
};

TEST_F(PermissionLifetimeManagerWithOriginMonitorTest,
       SetAndResetDomainPermission) {
  // Create a manager with a mocked origin lifetime monitor.
  ASSERT_TRUE(manager());
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, base::TimeDelta()));
  EXPECT_CALL(*origin_lifetime_monitor_,
              SubscribeToPermissionOriginDestruction(kOrigin))
      .WillOnce(testing::Return(kOrigin.host()));
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_FALSE(timer().IsRunning());

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, kOneTypeOneExpirationPrefValue,
                       {"notifications", kOrigin.host(), kOrigin.spec()});

  // Invalid host destroy shouldn't trigger any reset.
  origin_lifetime_monitor_->NotifyOriginDestroyed("test.com");
  CheckExpirationsPref(FROM_HERE, kOneTypeOneExpirationPrefValue,
                       {"notifications", kOrigin.host(), kOrigin.spec()});

  // Destroy origin, this should trigger a setting reset to default state.
  origin_lifetime_monitor_->NotifyOriginDestroyed(kOrigin.host());
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
}

TEST_F(PermissionLifetimeManagerWithOriginMonitorTest,
       ResetAllDomainsAfterRestart) {
  // Create a manager with a mocked origin lifetime monitor.
  ASSERT_TRUE(manager());
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, base::TimeDelta()));
  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::NOTIFICATIONS, base::TimeDelta()));
  EXPECT_CALL(*origin_lifetime_monitor_,
              SubscribeToPermissionOriginDestruction(kOrigin))
      .WillOnce(testing::Return(kOrigin.host()));
  EXPECT_CALL(*origin_lifetime_monitor_,
              SubscribeToPermissionOriginDestruction(kOrigin2))
      .WillOnce(testing::Return(kOrigin2.host()));
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_FALSE(timer().IsRunning());

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, kOneTypeTwoExpirationsPrefValue,
                       {"notifications", kOrigin.host(), kOrigin.spec(),
                        kOrigin2.host(), kOrigin2.spec()});

  ResetManager();
  // This will create a new PermissionLifetimeManager instance.
  ASSERT_TRUE(manager());

  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
}

TEST_F(PermissionLifetimeManagerWithOriginMonitorTest,
       TimeAndDomainKeyedPermissionsWorks) {
  // Create a manager with a mocked origin lifetime monitor.
  ASSERT_TRUE(manager());
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, kLifetime));
  const base::Time expected_expiration_time =
      base::Time::Now() + *request->GetLifetime();
  auto request2(CreateRequestAndAllowContentSetting(
      kOrigin2, ContentSettingsType::NOTIFICATIONS, base::TimeDelta()));
  EXPECT_CALL(*origin_lifetime_monitor_,
              SubscribeToPermissionOriginDestruction(kOrigin))
      .Times(0);
  EXPECT_CALL(*origin_lifetime_monitor_,
              SubscribeToPermissionOriginDestruction(kOrigin2))
      .WillOnce(testing::Return(kOrigin2.host()));
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  manager()->PermissionDecided(*request2, kOrigin2, kOrigin2,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);
  EXPECT_TRUE(timer().IsRunning());

  // Check data stored in prefs.
  CheckExpirationsPref(
      FROM_HERE, kOneTypeTwoExpirationsPrefValue,
      {"notifications",
       std::to_string(expected_expiration_time.ToDeltaSinceWindowsEpoch()
                          .InMicroseconds()),
       kOrigin.spec(), kOrigin2.host(), kOrigin2.spec()});

  browser_task_environment_.FastForwardBy(*request->GetLifetime());
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_ALLOW);

  // Check data stored in prefs.
  CheckExpirationsPref(FROM_HERE, kOneTypeOneExpirationPrefValue,
                       {"notifications", kOrigin2.host(), kOrigin2.spec()});

  // Destroy origin, this should trigger a setting reset to default state.
  origin_lifetime_monitor_->NotifyOriginDestroyed(kOrigin2.host());
  ExpectContentSetting(FROM_HERE, kOrigin2, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);

  // Prefs data should be empty.
  CheckExpirationsPref(FROM_HERE, "{}");
}

TEST_F(PermissionLifetimeManagerWithOriginMonitorTest,
       PermissionResetIfDomainKeyIsEmpty) {
  // Create a manager with a mocked origin lifetime monitor.
  ASSERT_TRUE(manager());
  auto request(CreateRequestAndAllowContentSetting(
      kOrigin, ContentSettingsType::NOTIFICATIONS, base::TimeDelta()));
  EXPECT_CALL(*origin_lifetime_monitor_,
              SubscribeToPermissionOriginDestruction(kOrigin))
      .WillOnce(testing::Return(std::string()));
  manager()->PermissionDecided(*request, kOrigin, kOrigin,
                               ContentSetting::CONTENT_SETTING_ALLOW, false);

  // Nothing should be stored in prefs.
  CheckExpirationsPref(FROM_HERE, "{}");

  // Permission should be reset on the next loop.
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_ALLOW);
  browser_task_environment_.RunUntilIdle();
  ExpectContentSetting(FROM_HERE, kOrigin, ContentSettingsType::NOTIFICATIONS,
                       ContentSetting::CONTENT_SETTING_DEFAULT);
}

}  // namespace permissions
