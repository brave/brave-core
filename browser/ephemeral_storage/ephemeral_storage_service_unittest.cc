/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom-data-view.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace ephemeral_storage {
namespace {

class ScopedVerifyAndClearExpectations {
 public:
  explicit ScopedVerifyAndClearExpectations(void* mock_obj)
      : mock_obj_(mock_obj) {}
  ~ScopedVerifyAndClearExpectations() {
    testing::Mock::VerifyAndClearExpectations(mock_obj_);
  }

 private:
  raw_ptr<void> mock_obj_;
};

class MockDelegate : public EphemeralStorageServiceDelegate {
 public:
  // EphemeralStorageServiceDelegate:
  MOCK_METHOD(void,
              CleanupTLDEphemeralArea,
              (const TLDEphemeralAreaKey& key),
              (override));
  MOCK_METHOD(void,
              CleanupFirstPartyStorageArea,
              (const TLDEphemeralAreaKey& key),
              (override));
  MOCK_METHOD(void,
              RegisterFirstWindowOpenedCallback,
              (base::OnceClosure callback),
              (override));
  MOCK_METHOD(bool,
              IsShieldsDisabledOnAnyHostMatchingDomainOf,
              (const GURL& url),
              (const override));
  MOCK_METHOD(void,
              PrepareTabsForFirstPartyStorageCleanup,
              (const std::string& ephemeral_domain),
              (override));

  void ExpectRegisterFirstWindowOpenedCallback(base::OnceClosure callback,
                                               bool trigger_callback) {
    EXPECT_CALL(*this, RegisterFirstWindowOpenedCallback(_))
        .WillOnce([this, trigger_callback](base::OnceClosure callback) {
          if (trigger_callback) {
            std::move(callback).Run();
          } else {
            first_window_opened_callback_ = std::move(callback);
          }
        });
  }

  void TriggerFirstWindowOpenedCallback() {
    ASSERT_TRUE(first_window_opened_callback_);
    std::move(first_window_opened_callback_).Run();
  }

 private:
  base::OnceClosure first_window_opened_callback_;
};

class MockObserver : public EphemeralStorageServiceObserver {
 public:
  // EphemeralStorageServiceObserver:
  MOCK_METHOD(void,
              OnCleanupTLDEphemeralArea,
              (const TLDEphemeralAreaKey& key),
              (override));
};

}  // namespace

class EphemeralStorageServiceTest : public testing::Test {
 public:
  enum class ExpectFirstWindowOpenedCallback {
    kTrigger,
    kDontTrigger,
  };

  EphemeralStorageServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~EphemeralStorageServiceTest() override = default;

  void SetUp() override {
    service_ = CreateEphemeralStorageService(&profile_, mock_delegate_,
                                             &mock_observer_);
  }

  void TearDown() override { ShutdownEphemeralStorageService(service_); }

  HostContentSettingsMap* host_content_settings_map(
      Profile* profile = nullptr) {
    return HostContentSettingsMapFactory::GetForProfile(profile ? profile
                                                                : &profile_);
  }

  static std::unique_ptr<EphemeralStorageService> CreateEphemeralStorageService(
      Profile* profile,
      raw_ptr<MockDelegate>& mock_delegate_ptr,
      MockObserver* observer,
      std::optional<ExpectFirstWindowOpenedCallback>
          expect_first_window_opened_callback =
              ExpectFirstWindowOpenedCallback::kTrigger) {
    auto mock_delegate = std::make_unique<testing::StrictMock<MockDelegate>>();
    if (expect_first_window_opened_callback) {
      mock_delegate->ExpectRegisterFirstWindowOpenedCallback(
          base::OnceClosure(), expect_first_window_opened_callback ==
                                   ExpectFirstWindowOpenedCallback::kTrigger);
    }
    mock_delegate_ptr = mock_delegate.get();
    auto service = std::make_unique<EphemeralStorageService>(
        profile, HostContentSettingsMapFactory::GetForProfile(profile),
        std::move(mock_delegate));
    if (observer) {
      service->AddObserver(observer);
    }
    return service;
  }

  void ShutdownEphemeralStorageService(
      std::unique_ptr<EphemeralStorageService>& service) {
    ASSERT_TRUE(service);
    mock_delegate_ = nullptr;
    service->Shutdown();
    service.reset();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  testing::StrictMock<MockObserver> mock_observer_;
  std::unique_ptr<EphemeralStorageService> service_;
  raw_ptr<MockDelegate> mock_delegate_ = nullptr;
  base::OnceClosure first_window_opened_callback_;
};

TEST_F(EphemeralStorageServiceTest, EphemeralCleanup) {
  const std::string ephemeral_domain = "a.com";
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);
  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  // No callbacks should be called while the keepalive is active.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
    task_environment_.FastForwardBy(base::Seconds(10));
  }

  // Reopen tld ephemeral lifetime while the keepalive is active.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);

  // Again, no callbacks should be called while the keepalive is active.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
    task_environment_.FastForwardBy(base::Seconds(20));
  }

  // Callbacks should be called after the timeout (10+20=30 seconds).
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
    EXPECT_CALL(mock_observer_, OnCleanupTLDEphemeralArea(key));
    EXPECT_CALL(*mock_delegate_, CleanupTLDEphemeralArea(key));
    task_environment_.FastForwardBy(base::Seconds(10));
  }
}

TEST_F(EphemeralStorageServiceTest,
       EphemeralCleanupNonDefaultStoragePartition) {
  const std::string ephemeral_domain = "a.com";
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);
  const auto second_storage_partition_config =
      content::StoragePartitionConfig::Create(&profile_, "partition_domain",
                                              "partition_name", false);
  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  // Create tld ephemeral lifetime in a different storage partition.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        second_storage_partition_config);

  // Callbacks should be called after the timeout, but only for the first
  // storage partition.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
    EXPECT_CALL(mock_observer_, OnCleanupTLDEphemeralArea(key));
    EXPECT_CALL(*mock_delegate_, CleanupTLDEphemeralArea(key));
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
    task_environment_.FastForwardBy(base::Seconds(30));
  }

  // Trigger the cleanup for the second storage partition.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, second_storage_partition_config);
    EXPECT_CALL(mock_observer_, OnCleanupTLDEphemeralArea(key));
    EXPECT_CALL(*mock_delegate_, CleanupTLDEphemeralArea(key));
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, second_storage_partition_config, false, false);
    task_environment_.FastForwardBy(base::Seconds(30));
  }
}

class EphemeralStorageServiceNoKeepAliveTest
    : public EphemeralStorageServiceTest {
 public:
  EphemeralStorageServiceNoKeepAliveTest() {
    scoped_feature_list_.InitAndDisableFeature(
        net::features::kBraveEphemeralStorageKeepAlive);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(EphemeralStorageServiceNoKeepAliveTest, ImmediateCleanup) {
  const std::string ephemeral_domain = "a.com";
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);
  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);

  // Callbacks should be called right after the TLD is destroyed.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
    EXPECT_CALL(mock_observer_, OnCleanupTLDEphemeralArea(key));
    EXPECT_CALL(*mock_delegate_, CleanupTLDEphemeralArea(key));
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
  }
}

class EphemeralStorageServiceForgetFirstPartyTest
    : public EphemeralStorageServiceTest {
 public:
  EphemeralStorageServiceForgetFirstPartyTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveForgetFirstPartyStorage);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(EphemeralStorageServiceForgetFirstPartyTest, CleanupFirstPartyStorage) {
  struct TestCase {
    bool shields_enabled;
    bool forget_first_party;
    bool should_cleanup;
  } constexpr kTestCases[] = {
      {.shields_enabled = false,
       .forget_first_party = false,
       .should_cleanup = false},
      {.shields_enabled = true,
       .forget_first_party = false,
       .should_cleanup = false},
      {.shields_enabled = true,
       .forget_first_party = true,
       .should_cleanup = true},
      {.shields_enabled = false,
       .forget_first_party = true,
       .should_cleanup = false},
  };

  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  for (const auto& test_case : kTestCases) {
    LOG(INFO) << "[SHRED] shields_enabled: " << test_case.shields_enabled
              << ", forget_first_party: " << test_case.forget_first_party
              << ", should_cleanup: " << test_case.should_cleanup;
    SCOPED_TRACE(testing::Message()
                 << test_case.shields_enabled << test_case.forget_first_party);
    host_content_settings_map()->SetContentSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
        test_case.forget_first_party ? CONTENT_SETTING_BLOCK
                                     : CONTENT_SETTING_ALLOW);

    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          storage_partition_config);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);

    {
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
      TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
      EXPECT_CALL(mock_observer_, OnCleanupTLDEphemeralArea(key));
      EXPECT_CALL(*mock_delegate_, CleanupTLDEphemeralArea(key))
          .Times(test_case.shields_enabled);
      EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key))
          .Times(test_case.should_cleanup);
      service_->TLDEphemeralLifetimeDestroyed(
          ephemeral_domain, storage_partition_config,
          !test_case.shields_enabled, false);
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                test_case.should_cleanup ? 1u : 0u);
      task_environment_.FastForwardBy(base::Seconds(30));
    }

    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest, CleanupOnRestart) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      CONTENT_SETTING_BLOCK);

  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      0u);

  // Make sure prefs is filled with the origin to cleanup.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        1u);
  }

  // Simulate a browser restart. No cleanup should happen at construction.
  {
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    ShutdownEphemeralStorageService(service_);

    service_ = CreateEphemeralStorageService(&profile_, mock_delegate_,
                                             &mock_observer_);
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        1u);
  }

  // Cleanup should happen in 5 seconds after the startup.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
    EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key));
    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest,
       PreventCleanupOnSessionRestore) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      CONTENT_SETTING_BLOCK);

  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      0u);

  service_->TLDEphemeralLifetimeDestroyed(
      ephemeral_domain, storage_partition_config, false, false);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      1u);

  // Simulate a browser restart. No cleanup should happen at construction.
  {
    ShutdownEphemeralStorageService(service_);
    service_ = CreateEphemeralStorageService(&profile_, mock_delegate_,
                                             &mock_observer_);
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        1u);
    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          storage_partition_config);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }

  // Cleanup should NOT happen in 5 seconds after the startup.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    task_environment_.FastForwardBy(base::Seconds(5));
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest,
       PreventCleanupOnSessionRestoreWithMultipleStoragePartitions) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);
  const auto second_storage_partition_config =
      content::StoragePartitionConfig::Create(&profile_, "partition_domain",
                                              "partition_name", false);

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      CONTENT_SETTING_BLOCK);

  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        second_storage_partition_config);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      0u);

  LOG(INFO) << "[SHRED] Test #100";
  service_->TLDEphemeralLifetimeDestroyed(
      ephemeral_domain, storage_partition_config, false, false);
  service_->TLDEphemeralLifetimeDestroyed(
      ephemeral_domain, second_storage_partition_config, false, false);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      2u);
  LOG(INFO) << "[SHRED] Test #200 Lift:"
            << profile_.GetPrefs()
                   ->GetList(kFirstPartyStorageOriginsToCleanup)
                   .DebugString();

  // Simulate a browser restart. No cleanup should happen at construction.
  {
    ShutdownEphemeralStorageService(service_);
    service_ = CreateEphemeralStorageService(&profile_, mock_delegate_,
                                             &mock_observer_);
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        2u);
    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          storage_partition_config);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        1u);
  }

  // Cleanup should happen only for the second storage partition in 5 seconds
  // after the startup.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, second_storage_partition_config);
    EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key));
    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest,
       PreventCleanupIfNoWindowsOpened) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      CONTENT_SETTING_BLOCK);

  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      0u);

  // Make sure prefs is filled with the origin to cleanup.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        1u);
  }

  // Simulate a browser restart. No cleanup should happen at construction.
  {
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    ShutdownEphemeralStorageService(service_);

    service_ = CreateEphemeralStorageService(
        &profile_, mock_delegate_, &mock_observer_,
        ExpectFirstWindowOpenedCallback::kDontTrigger);
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        1u);
  }

  // Cleanup should NOT happen in 5 seconds after the startup.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    task_environment_.FastForwardBy(base::Seconds(5));
  }

  // Trigger the first window opened callback.
  mock_delegate_->TriggerFirstWindowOpenedCallback();

  // Cleanup should happen in the next 5 seconds after the window is opened.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
    EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key));
    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest, OffTheRecordSkipsPrefs) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  Profile* otr_profile =
      profile_.GetOffTheRecordProfile(Profile::OTRProfileID::PrimaryID(), true);

  auto otr_service = CreateEphemeralStorageService(
      otr_profile, mock_delegate_, &mock_observer_, std::nullopt);
  host_content_settings_map(otr_profile)
      ->SetContentSettingDefaultScope(
          url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
          CONTENT_SETTING_BLOCK);

  // Create tld ephemeral lifetime.
  otr_service->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                           storage_partition_config);
  EXPECT_EQ(otr_profile->GetPrefs()
                ->GetList(kFirstPartyStorageOriginsToCleanup)
                .size(),
            0u);

  otr_service->TLDEphemeralLifetimeDestroyed(
      ephemeral_domain, storage_partition_config, false, false);
  EXPECT_EQ(otr_profile->GetPrefs()
                ->GetList(kFirstPartyStorageOriginsToCleanup)
                .size(),
            0u);

  // Simulate a browser restart. No cleanup should happen at all.
  {
    ShutdownEphemeralStorageService(otr_service);
    otr_service = CreateEphemeralStorageService(otr_profile, mock_delegate_,
                                                &mock_observer_, std::nullopt);
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    task_environment_.FastForwardBy(base::Seconds(5));
  }

  ShutdownEphemeralStorageService(otr_service);
}

class EphemeralStorageServiceAutoShredForgetFirstPartyTest
    : public EphemeralStorageServiceTest {
 public:
  EphemeralStorageServiceAutoShredForgetFirstPartyTest() {
    scoped_feature_list_.InitWithFeatureStates({
        {net::features::kBraveForgetFirstPartyStorage, true},
        {brave_shields::features::kBraveShredFeature, true},
    });
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(EphemeralStorageServiceAutoShredForgetFirstPartyTest,
       CleanupFirstPartyStorage) {
  struct TestCase {
    const char* name;
    bool shields_enabled;
    bool forget_first_party;
    std::optional<brave_shields::mojom::AutoShredMode> auto_shred_mode;
    size_t cleanup_list_size;
    bool should_cleanup;
  } constexpr kTestCases[] = {
      {.name = "Case: Shields disabled, no forget first party",
       .shields_enabled = false,
       .forget_first_party = false,
       .auto_shred_mode = std::nullopt,
       .cleanup_list_size = 0,
       .should_cleanup = false},
      {.name = "Case: Shields enabled, no forget first party",
       .shields_enabled = true,
       .forget_first_party = false,
       .auto_shred_mode = std::nullopt,
       .cleanup_list_size = 0,
       .should_cleanup = false},
      {.name = "Case: Shields enabled, forget first party, no AutoShred",
       .shields_enabled = true,
       .forget_first_party = true,
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::NEVER,
       .cleanup_list_size = 0,
       .should_cleanup = false},
      {.name = "Case: Shields enabled, no forget first party, AutoShred "
               "LAST_TAB_CLOSED",
       .shields_enabled = true,
       .forget_first_party = true,
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED,
       .cleanup_list_size = 0,
       .should_cleanup = true},
      {.name = "Case: Shields enabled, no forget first party, AutoShred "
               "LAST_TAB_CLOSED",
       .shields_enabled = true,
       .forget_first_party = false,
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED,
       .cleanup_list_size = 0,
       .should_cleanup = true},
      {.name = "Case: Shields enabled, forget first party, AutoShred APP_EXIT",
       .shields_enabled = true,
       .forget_first_party = true,
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::APP_EXIT,
       .cleanup_list_size = 1,
       .should_cleanup = false},
  };

  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << test_case.name);

    if (test_case.auto_shred_mode) {
      host_content_settings_map()->SetWebsiteSettingCustomScope(
          content_settings::CreateDomainPattern(url),
          ContentSettingsPattern::Wildcard(),
          ContentSettingsType::BRAVE_AUTO_SHRED,
          brave_shields::AutoShredSetting::ToValue(
              test_case.auto_shred_mode.value()));
    }

    host_content_settings_map()->SetContentSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
        test_case.forget_first_party ? CONTENT_SETTING_BLOCK
                                     : CONTENT_SETTING_ALLOW);

    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          storage_partition_config);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);

    {
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
      TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
      EXPECT_CALL(mock_observer_, OnCleanupTLDEphemeralArea(key))
          .Times(test_case.should_cleanup);
      EXPECT_CALL(*mock_delegate_, CleanupTLDEphemeralArea(key))
          .Times(test_case.should_cleanup);
      EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key))
          .Times(test_case.should_cleanup);
      service_->TLDEphemeralLifetimeDestroyed(
          ephemeral_domain, storage_partition_config,
          !test_case.shields_enabled, false);
      // Wait for 30 seconds to ensure any delayed cleanup would have occurred.
      task_environment_.FastForwardBy(base::Seconds(30));
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                test_case.cleanup_list_size);
    }
  }
}

TEST_F(EphemeralStorageServiceAutoShredForgetFirstPartyTest, CleanupOnRestart) {
  struct TestCase {
    const char* name;
    std::optional<brave_shields::mojom::AutoShredMode> auto_shred_mode;
    bool forget_first_party;
    bool saved_to_cleanup_list;
    int cleanup_first_party_calls;
  } constexpr kTestCases[] = {
      {.name = "Simple forgetful mode",
       .auto_shred_mode = std::nullopt,
       .forget_first_party = true,
       .saved_to_cleanup_list = false,
       .cleanup_first_party_calls = 0},
      {.name = "AutoShred: APP_EXIT mode",
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::APP_EXIT,
       .forget_first_party = true,
       .saved_to_cleanup_list = true,
       .cleanup_first_party_calls = 1},
      {.name = "AutoShred: LAST_TAB_CLOSED mode",
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED,
       .forget_first_party = true,
       .saved_to_cleanup_list = true,
       .cleanup_first_party_calls = 0},
      {.name = "AutoShred: NEVER mode",
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::NEVER,
       .forget_first_party = true,
       .saved_to_cleanup_list = false,
       .cleanup_first_party_calls = 0},
  };

  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << test_case.name);
    if (test_case.auto_shred_mode) {
      host_content_settings_map()->SetWebsiteSettingCustomScope(
          content_settings::CreateDomainPattern(url),
          ContentSettingsPattern::Wildcard(),
          ContentSettingsType::BRAVE_AUTO_SHRED,
          brave_shields::AutoShredSetting::ToValue(
              test_case.auto_shred_mode.value()));
    }

    host_content_settings_map()->SetContentSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
        test_case.forget_first_party ? CONTENT_SETTING_BLOCK
                                     : CONTENT_SETTING_ALLOW);

    // Create tld ephemeral lifetime.
    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          storage_partition_config);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);

    // Make sure prefs is filled with the origin to cleanup.
    {
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
      service_->TLDEphemeralLifetimeDestroyed(
          ephemeral_domain, storage_partition_config, false, false);
      LOG(INFO) << "[SHRED] List after destruction: "
                << profile_.GetPrefs()
                       ->GetList(kFirstPartyStorageOriginsToCleanup)
                       .DebugString();
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                test_case.saved_to_cleanup_list ? 1u : 0u);
    }

    LOG(INFO) << "[SHRED] Simulate a browser restart. No cleanup should happen "
                 "at construction.";
    // Simulate a browser restart. No cleanup should happen at construction.
    {
      ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
      ShutdownEphemeralStorageService(service_);

      service_ = CreateEphemeralStorageService(&profile_, mock_delegate_,
                                               &mock_observer_);
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                test_case.saved_to_cleanup_list ? 1u : 0u);
    }

    LOG(INFO)
        << "[SHRED] Cleanup should happen in 5 seconds after the startup.";
    // Cleanup should happen in 5 seconds after the startup.
    {
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
      TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
      EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key))
          .Times(test_case.cleanup_first_party_calls);
      task_environment_.FastForwardBy(base::Seconds(5));
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                0u);
    }
  }
}

TEST_F(EphemeralStorageServiceAutoShredForgetFirstPartyTest,
       PreventCleanupOnSessionRestoreWithMultipleStoragePartitions) {
  struct TestCase {
    const char* name;
    std::optional<brave_shields::mojom::AutoShredMode> auto_shred_mode;
    bool forget_first_party;
    size_t saved_to_cleanup_list;
    size_t saved_to_cleanup_list_after_restart;
    int cleanup_first_party_calls;
  } constexpr kTestCases[] = {
      {.name = "Simple forgetful mode",
       .auto_shred_mode = std::nullopt,
       .forget_first_party = true,
       .saved_to_cleanup_list = 0u,
       .saved_to_cleanup_list_after_restart = 0u,
       .cleanup_first_party_calls = 0},
      {.name = "AutoShred: APP_EXIT mode",
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::APP_EXIT,
       .forget_first_party = true,
       .saved_to_cleanup_list = 2u,
       .saved_to_cleanup_list_after_restart = 1u,
       .cleanup_first_party_calls = 1},
      {.name = "AutoShred: LAST_TAB_CLOSED mode",
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED,
       .forget_first_party = true,
       .saved_to_cleanup_list = 2u,
       .saved_to_cleanup_list_after_restart = 1u,
       .cleanup_first_party_calls = 0},
      {.name = "AutoShred: NEVER mode",
       .auto_shred_mode = brave_shields::mojom::AutoShredMode::NEVER,
       .forget_first_party = true,
       .saved_to_cleanup_list = 0u,
       .saved_to_cleanup_list_after_restart = 0u,
       .cleanup_first_party_calls = 0},
  };

  const GURL url("https://a.com");
  const std::string ephemeral_domain = std::string(url.host());
  const auto storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);
  const auto second_storage_partition_config =
      content::StoragePartitionConfig::Create(&profile_, "partition_domain",
                                              "partition_name", false);

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << test_case.name);
    if (test_case.auto_shred_mode) {
      host_content_settings_map()->SetWebsiteSettingCustomScope(
          content_settings::CreateDomainPattern(url),
          ContentSettingsPattern::Wildcard(),
          ContentSettingsType::BRAVE_AUTO_SHRED,
          brave_shields::AutoShredSetting::ToValue(
              test_case.auto_shred_mode.value()));
    }
    host_content_settings_map()->SetContentSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
        test_case.forget_first_party ? CONTENT_SETTING_BLOCK
                                     : CONTENT_SETTING_ALLOW);

    // Create tld ephemeral lifetime.
    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          storage_partition_config);
    service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                          second_storage_partition_config);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);

    LOG(INFO) << "[SHRED] Test #100";
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, storage_partition_config, false, false);
    service_->TLDEphemeralLifetimeDestroyed(
        ephemeral_domain, second_storage_partition_config, false, false);
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        test_case.saved_to_cleanup_list);
    LOG(INFO) << "[SHRED] Test #200 List:"
              << profile_.GetPrefs()
                     ->GetList(kFirstPartyStorageOriginsToCleanup)
                     .DebugString();

    // Simulate a browser restart. No cleanup should happen at construction.
    {
      ShutdownEphemeralStorageService(service_);
      service_ = CreateEphemeralStorageService(&profile_, mock_delegate_,
                                               &mock_observer_);
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                test_case.saved_to_cleanup_list);
      service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                            storage_partition_config);
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                test_case.saved_to_cleanup_list_after_restart);
    }

    // Cleanup should happen only for the second storage partition in 5 seconds
    // after the startup.
    {
      ScopedVerifyAndClearExpectations verify(mock_delegate_);
      ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
      TLDEphemeralAreaKey key(ephemeral_domain,
                              second_storage_partition_config);
      LOG(INFO) << "[SHRED] Test #300 Name:" << test_case.name << ", expected "
                << test_case.cleanup_first_party_calls
                << " calls to CleanupFirstPartyStorageArea";
      EXPECT_CALL(*mock_delegate_, CleanupFirstPartyStorageArea(key))
          .Times(test_case.cleanup_first_party_calls);
      task_environment_.FastForwardBy(base::Seconds(5));
      EXPECT_EQ(profile_.GetPrefs()
                    ->GetList(kFirstPartyStorageOriginsToCleanup)
                    .size(),
                0u);
    }
  }
}

}  // namespace ephemeral_storage
