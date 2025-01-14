/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
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
              (const std::string& registerable_domain),
              (override));
  MOCK_METHOD(void,
              RegisterFirstWindowOpenedCallback,
              (base::OnceClosure callback),
              (override));

  void ExpectRegisterFirstWindowOpenedCallback(base::OnceClosure callback,
                                               bool trigger_callback) {
    EXPECT_CALL(*this, RegisterFirstWindowOpenedCallback(_))
        .WillOnce(testing::Invoke(
            [this, trigger_callback](base::OnceClosure callback) {
              if (trigger_callback) {
                std::move(callback).Run();
              } else {
                first_window_opened_callback_ = std::move(callback);
              }
            }));
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
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);
  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);

  // No callbacks should be called while the keepalive is active.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                            storage_partition_config, false);
    task_environment_.FastForwardBy(base::Seconds(10));
  }

  // Reopen tld ephemeral lifetime while the keepalive is active.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);

  // Again, no callbacks should be called while the keepalive is active.
  {
    ScopedVerifyAndClearExpectations verify(mock_delegate_);
    ScopedVerifyAndClearExpectations verify_observer(&mock_observer_);
    service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                            storage_partition_config, false);
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
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);
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
    service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                            storage_partition_config, false);
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
  const std::string ephemeral_domain = url.host();
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);

  for (const auto& test_case : kTestCases) {
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
      EXPECT_CALL(*mock_delegate_,
                  CleanupFirstPartyStorageArea(ephemeral_domain))
          .Times(test_case.should_cleanup);
      service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                              storage_partition_config,
                                              !test_case.shields_enabled);
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
  const std::string ephemeral_domain = url.host();
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);

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
    service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                            storage_partition_config, false);
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
    EXPECT_CALL(*mock_delegate_,
                CleanupFirstPartyStorageArea(ephemeral_domain));
    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest,
       PreventCleanupOnSessionRestore) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = url.host();
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      CONTENT_SETTING_BLOCK);

  // Create tld ephemeral lifetime.
  service_->TLDEphemeralLifetimeCreated(ephemeral_domain,
                                        storage_partition_config);
  EXPECT_EQ(
      profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
      0u);

  service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                          storage_partition_config, false);
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
       PreventCleanupIfNoWindowsOpened) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = url.host();
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);

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
    service_->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                            storage_partition_config, false);
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
    EXPECT_CALL(*mock_delegate_,
                CleanupFirstPartyStorageArea(ephemeral_domain));
    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(
        profile_.GetPrefs()->GetList(kFirstPartyStorageOriginsToCleanup).size(),
        0u);
  }
}

TEST_F(EphemeralStorageServiceForgetFirstPartyTest, OffTheRecordSkipsPrefs) {
  const GURL url("https://a.com");
  const std::string ephemeral_domain = url.host();
  const auto storage_partition_config = content::StoragePartitionConfig::Create(
      &profile_, ephemeral_domain, {}, false);

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

  otr_service->TLDEphemeralLifetimeDestroyed(ephemeral_domain,
                                             storage_partition_config, false);
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

}  // namespace ephemeral_storage
