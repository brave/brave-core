// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_service.h"

#include <memory>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_mock_time_task_runner.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

namespace brave_shields {

class FilterListObserver : public AdBlockService::Observer {
 public:
  using Callback = base::RepeatingCallback<void(bool, bool)>;
  explicit FilterListObserver(Callback cb) : cb_(std::move(cb)) {}
  ~FilterListObserver() override = default;
  void OnFilterListLoaded(bool is_default_engine, bool success) override {
    cb_.Run(is_default_engine, success);
  }

 private:
  Callback cb_;
};

// TODO need a test that dat gets rebuilt if only one filter list is newer
class DATLoadObserver : public AdBlockService::Observer {
 public:
  DATLoadObserver() = default;
  ~DATLoadObserver() override = default;

  void OnDATFileLoaded(bool is_default_engine, bool success) override {
    if (is_default_engine) {
      default_loaded_ = true;
      default_success_ = success;
    } else {
      additional_loaded_ = true;
      additional_success_ = success;
    }
  }

  bool BothLoaded() const { return default_loaded_ && additional_loaded_; }
  bool default_success() const { return default_success_; }
  bool additional_success() const { return additional_success_; }

 private:
  bool default_loaded_ = false;
  bool default_success_ = false;
  bool additional_loaded_ = false;
  bool additional_success_ = false;
};

class AdBlockServiceTestBase : public testing::Test {
 public:
  AdBlockServiceTestBase()
      : resource_storage_(adblock::new_empty_resource_storage()) {}

  void SetUp() override {
    RegisterPrefsForAdBlockService(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(profile_dir_.CreateUniqueTempDir());
  }

 protected:
  DATFileDataBuffer CreateAdblockDAT(std::string_view filters) {
    std::vector<unsigned char> buffer(filters.begin(), filters.end());
    AdBlockEngine engine(true);
    engine.Load(false, buffer, *resource_storage_);
    return engine.Serialize();
  }

  void CreateCachedDATFiles(std::string_view default_rules,
                            std::string_view additional_rules) {
    base::FilePath cache_dir =
        profile_dir_.GetPath().AppendASCII("adblock_cache");
    ASSERT_TRUE(base::CreateDirectory(cache_dir));
    ASSERT_TRUE(base::WriteFile(cache_dir.AppendASCII("engine0.dat"),
                                CreateAdblockDAT(default_rules)));
    ASSERT_TRUE(base::WriteFile(cache_dir.AppendASCII("engine1.dat"),
                                CreateAdblockDAT(additional_rules)));
  }

  void CreateCachedDefaultDATFile(std::string_view default_rules) {
    base::FilePath cache_dir =
        profile_dir_.GetPath().AppendASCII("adblock_cache");
    ASSERT_TRUE(base::CreateDirectory(cache_dir));
    ASSERT_TRUE(base::WriteFile(cache_dir.AppendASCII("engine0.dat"),
                                CreateAdblockDAT(default_rules)));
  }

  void CreateCachedAdditionalDATFile(std::string_view additional_rules) {
    base::FilePath cache_dir =
        profile_dir_.GetPath().AppendASCII("adblock_cache");
    ASSERT_TRUE(base::CreateDirectory(cache_dir));
    ASSERT_TRUE(base::WriteFile(cache_dir.AppendASCII("engine1.dat"),
                                CreateAdblockDAT(additional_rules)));
  }

  std::unique_ptr<AdBlockService> CreateServiceWithTaskRunner(
      scoped_refptr<base::SequencedTaskRunner> task_runner) {
    download_manager_ = std::make_unique<AdBlockSubscriptionDownloadManager>(
        nullptr, task_runner);
    auto service = std::make_unique<AdBlockService>(
        &prefs_, "en", nullptr, task_runner,
        base::BindOnce(
            [](AdBlockSubscriptionDownloadManager* manager,
               base::OnceCallback<void(AdBlockSubscriptionDownloadManager*)>
                   cb) { std::move(cb).Run(manager); },
            download_manager_.get()),
        profile_dir_.GetPath());
    service->default_resource_provider()->OverrideResourcesForTesting(
        adblock::new_empty_resource_storage());
    service->custom_resource_provider()->OverrideResourcesForTesting(
        adblock::new_empty_resource_storage());
    return service;
  }

  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  base::ScopedTempDir profile_dir_;
  rust::Box<adblock::BraveCoreResourceStorage> resource_storage_;
  std::unique_ptr<AdBlockSubscriptionDownloadManager> download_manager_;
};

class AdBlockServiceTest : public AdBlockServiceTestBase {
 public:
  AdBlockServiceTest() {
    feature_list_.InitAndEnableFeature(features::kAdblockDATCache);
  }

 protected:
  std::unique_ptr<AdBlockService> CreateService() {
    return CreateServiceWithTaskRunner(
        task_environment_.GetMainThreadTaskRunner());
  }

  base::test::TaskEnvironment task_environment_;
};

class AdBlockServiceQueuedTest : public AdBlockServiceTestBase {
 public:
  AdBlockServiceQueuedTest() {
    feature_list_.InitAndEnableFeature(features::kAdblockDATCache);
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME,
      base::test::TaskEnvironment::ThreadPoolExecutionMode::QUEUED};
};

class AdBlockServiceDATCacheDisabledTest : public AdBlockServiceTestBase {
 public:
  AdBlockServiceDATCacheDisabledTest() {
    feature_list_.InitAndDisableFeature(features::kAdblockDATCache);
  }

 protected:
  std::unique_ptr<AdBlockService> CreateService() {
    return CreateServiceWithTaskRunner(
        task_environment_.GetMainThreadTaskRunner());
  }

  base::test::TaskEnvironment task_environment_;
};

TEST_F(AdBlockServiceTest, LoadsCachedDATFilesOnCreation) {
  // Create cached DAT files with known rules
  CreateCachedDATFiles("||blocked-by-default.com^\n",
                       "||blocked-by-additional.com^\n");

  // Set cache timestamps to indicate the cache is valid
  // This prevents filter set loading and allows DAT loading
  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, now);
  prefs_.SetTime(prefs::kAdBlockAdditionalCacheTimestamp, now);

  // Create the service - it should load the cached DAT files
  auto service = CreateService();

  // Verify nothing is blocked before DAT files are loaded
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-default.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
  result = service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-additional.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);

  DATLoadObserver observer;
  service->AddObserver(&observer);
  ASSERT_TRUE(base::test::RunUntil([&]() { return observer.BothLoaded(); }));

  // Verify default engine rules are loaded
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-default.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Verify additional engine rules are loaded
  result = service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-additional.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Verify non-blocked URLs are not blocked
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://allowed.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceTest, LoadsOnlyDefaultCachedDATFile) {
  // Create a cached DAT file only for the default engine
  CreateCachedDefaultDATFile("||blocked-by-default.com^\n");

  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, now);
  prefs_.SetTime(prefs::kAdBlockAdditionalCacheTimestamp, now);

  auto service = CreateService();

  // Verify nothing is blocked before DAT files are loaded
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-default.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);

  DATLoadObserver observer;
  service->AddObserver(&observer);
  ASSERT_TRUE(base::test::RunUntil([&]() { return observer.BothLoaded(); }));

  EXPECT_TRUE(observer.default_success());
  EXPECT_FALSE(observer.additional_success());

  // Default engine rules should be active
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-default.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Additional engine should have no rules
  result = service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-default.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceTest, LoadsOnlyAdditionalCachedDATFile) {
  // Create a cached DAT file only for the additional engine
  CreateCachedAdditionalDATFile("||blocked-by-additional.com^\n");

  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, now);
  prefs_.SetTime(prefs::kAdBlockAdditionalCacheTimestamp, now);

  auto service = CreateService();

  // Verify nothing is blocked before DAT files are loaded
  auto result =
      service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
          GURL("https://blocked-by-additional.com/script.js"),
          blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);

  DATLoadObserver observer;
  service->AddObserver(&observer);
  ASSERT_TRUE(base::test::RunUntil([&]() { return observer.BothLoaded(); }));

  EXPECT_FALSE(observer.default_success());
  EXPECT_TRUE(observer.additional_success());

  // Additional engine rules should be active
  result = service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-additional.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Default engine should have no rules
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-additional.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceTest, WorksWithoutCachedDATFiles) {
  // Don't create any cached files - service should still work

  auto service = CreateService();

  DATLoadObserver observer;
  service->AddObserver(&observer);
  ASSERT_TRUE(base::test::RunUntil([&]() { return observer.BothLoaded(); }));

  // Should not crash and not block anything
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://example.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceTest,
       DefaultEngineLoadsEvenWhenAdditionalEngineCacheIsSet) {
  // Set only the additional engine cache timestamp. The default cache timestamp
  // is left unset (base::Time()), meaning the default engine should always
  // load.
  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockAdditionalCacheTimestamp, now);

  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  // Register a test filters provider for the default engine. Its timestamp
  // matches the additional cache timestamp — with the bug, the default
  // observer would check the additional pref, see it's cached, and skip.
  auto provider =
      std::make_unique<TestFiltersProvider>("||default-rule.com^",
                                            /*engine_is_default=*/true);
  provider->set_timestamp(now);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Timeout waiting for default engine filter set to load";

  // The default engine must have loaded the filter set rules.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://default-rule.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

TEST_F(AdBlockServiceTest, FutureCacheTimestampAllowsReload) {
  // A cache timestamp in the future (e.g. clock skew) should not permanently
  // block filter set loading — ShouldLoadFilterState has a guard for this.
  base::Time future = base::Time::Now() + base::Hours(24);
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, future);

  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  auto provider =
      std::make_unique<TestFiltersProvider>("||after-future-cache.com^",
                                            /*engine_is_default=*/true);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Timeout: future cache timestamp blocked filter set loading";

  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://after-future-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

TEST_F(AdBlockServiceTest, NewerProviderTimestampOverridesCache) {
  // When a provider fires with a timestamp newer than the cache, the filter
  // set should be loaded even though a valid cache exists.
  base::Time old_time = base::Time::Now() - base::Hours(2);
  base::Time new_time = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, old_time);

  // Create a cached DAT file with old rules.
  CreateCachedDefaultDATFile("||old-cached-rule.com^\n");

  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  // Register a provider with a newer timestamp than the cache.
  auto provider = std::make_unique<TestFiltersProvider>(
      "||new-filter-rule.com^", /*engine_is_default=*/true);
  provider->set_timestamp(new_time);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Timeout: newer provider timestamp should have triggered reload";

  // The new filter set rules should be active, not the old cache.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://new-filter-rule.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

TEST_F(AdBlockServiceQueuedTest, FilterSetLoadingBlocksDATLoading) {
  // Create cached DAT files with specific rules for the default engine.
  CreateCachedDATFiles("||blocked-by-dat.com^\n", "");

  auto service_task_runner =
      base::MakeRefCounted<base::TestMockTimeTaskRunner>();

  std::unique_ptr<AdBlockService> service =
      CreateServiceWithTaskRunner(service_task_runner);

  // Verify DAT loading is initially allowed.
  ASSERT_TRUE(service->GetAllowDatLoadingForTesting());

  // Verify neither DAT rules nor filter rules are active before loading.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-dat.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
  result = service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-filter.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);

  // Trigger filter set loading by updating custom filters.
  // Custom filters are for the additional engine (engine_is_default=false).
  service->custom_filters_provider()->UpdateCustomFilters(
      "||blocked-by-filter.com^");

  base::RunLoop on_changed_run_loop;
  service_task_runner->PostTask(FROM_HERE, on_changed_run_loop.QuitClosure());
  while (!on_changed_run_loop.AnyQuitCalled()) {
    if (service_task_runner->HasPendingTask()) {
      service_task_runner->FastForwardBy(
          service_task_runner->NextPendingTaskDelay());
    } else {
      base::RunLoop().RunUntilIdle();
      // Safety break if the runner is empty but the quit signal never came.
      if (!service_task_runner->HasPendingTask()) {
        break;
      }
    }
  }

  base::RunLoop().RunUntilIdle();
  base::RunLoop on_filter_set_loaded_run_loop;
  service_task_runner->PostTask(FROM_HERE,
                                on_filter_set_loaded_run_loop.QuitClosure());
  while (!on_filter_set_loaded_run_loop.AnyQuitCalled()) {
    if (service_task_runner->HasPendingTask()) {
      service_task_runner->FastForwardBy(
          service_task_runner->NextPendingTaskDelay());
    } else {
      base::RunLoop().RunUntilIdle();
      // Safety break if the runner is empty but the quit signal never came.
      if (!service_task_runner->HasPendingTask()) {
        break;
      }
    }
  }

  base::RunLoop().RunUntilIdle();
  base::RunLoop on_resources_loaded_run_loop;
  service_task_runner->PostTask(FROM_HERE,
                                on_resources_loaded_run_loop.QuitClosure());
  while (!on_resources_loaded_run_loop.AnyQuitCalled()) {
    if (service_task_runner->HasPendingTask()) {
      service_task_runner->FastForwardBy(
          service_task_runner->NextPendingTaskDelay());
    } else {
      base::RunLoop().RunUntilIdle();
      // Safety break if the runner is empty but the quit signal never came.
      if (!service_task_runner->HasPendingTask()) {
        break;
      }
    }
  }
  ASSERT_FALSE(service->GetAllowDatLoadingForTesting());
  ASSERT_EQ(prefs_.GetTime(prefs::kAdBlockAdditionalCacheTimestamp),
            base::Time());

  base::RunLoop().RunUntilIdle();
  ASSERT_NE(prefs_.GetTime(prefs::kAdBlockAdditionalCacheTimestamp),
            base::Time());

  base::RunLoop on_read_cached_dat_files_run_loop;
  service_task_runner->PostTask(
      FROM_HERE, on_read_cached_dat_files_run_loop.QuitClosure());

  // Kick off the dat loading
  task_environment_.RunUntilIdle();

  while (!on_read_cached_dat_files_run_loop.AnyQuitCalled()) {
    if (service_task_runner->HasPendingTask()) {
      service_task_runner->FastForwardBy(
          service_task_runner->NextPendingTaskDelay());
    } else {
      base::RunLoop().RunUntilIdle();
      // Safety break if the runner is empty but the quit signal never came.
      if (!service_task_runner->HasPendingTask()) {
        break;
      }
    }
  }
  base::RunLoop().RunUntilIdle();

  ASSERT_FALSE(service_task_runner->HasPendingTask());

  // Custom filter should be active on the additional engine
  result = service->GetAdditionalFiltersEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-filter.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // DAT rules should NOT be loaded because filter set loading
  // set allow_load_dat_loading_ = false before DAT callback ran.
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://blocked-by-dat.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceQueuedTest, ValidCacheTimestampSkipsFilterSetLoad) {
  // When cache timestamp matches the provider's timestamp, filter set loading
  // should be skipped (ShouldLoadFilterState returns false).
  // This test requires queued execution to ensure the DAT loads before the
  // filter set provider is registered.
  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, now);
  prefs_.SetTime(prefs::kAdBlockAdditionalCacheTimestamp, now);

  CreateCachedDefaultDATFile("||from-cache.com^\n");

  auto service_task_runner =
      base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  auto service = CreateServiceWithTaskRunner(service_task_runner);

  // Let the DAT loading complete first.
  task_environment_.RunUntilIdle();
  while (service_task_runner->HasPendingTask()) {
    service_task_runner->FastForwardBy(
        service_task_runner->NextPendingTaskDelay());
    base::RunLoop().RunUntilIdle();
  }
  base::RunLoop().RunUntilIdle();

  // The cached DAT rules should now be loaded.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Now register a provider with a timestamp matching the cache.
  // ShouldLoadFilterState should return false, skipping the filter set load.
  auto provider = std::make_unique<TestFiltersProvider>(
      "||from-filter-set.com^", /*engine_is_default=*/true);
  provider->set_timestamp(now);
  provider->RegisterAsSourceProvider(service.get());

  // Drain any remaining tasks.
  while (service_task_runner->HasPendingTask()) {
    service_task_runner->FastForwardBy(
        service_task_runner->NextPendingTaskDelay());
    base::RunLoop().RunUntilIdle();
  }
  base::RunLoop().RunUntilIdle();

  // Cached DAT rules should still be active.
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Filter set rules should NOT be active (loading was skipped).
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-filter-set.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceDATCacheDisabledTest, CachedDATIgnoredWhenFlagDisabled) {
  // Create cached DAT files and set valid cache timestamps.
  CreateCachedDATFiles("||from-cache.com^\n", "");
  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, now);
  prefs_.SetTime(prefs::kAdBlockAdditionalCacheTimestamp, now);

  auto service = CreateService();

  // Register a provider for the default engine — with the flag disabled,
  // filter set loading should always proceed regardless of cache state.
  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  auto provider = std::make_unique<TestFiltersProvider>(
      "||from-filter-set.com^", /*engine_is_default=*/true);
  provider->set_timestamp(now);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Filter set should load even when cache timestamps are set";

  // The filter set rules should be active, not the cached DAT rules.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-filter-set.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // The cached DAT rules should NOT be loaded.
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_FALSE(result.matched);
}

TEST_F(AdBlockServiceDATCacheDisabledTest,
       FilterSetAlwaysLoadsWithStaleTimestamp) {
  // Even with a cache timestamp that would normally cause the filter set
  // to be skipped, it should still load when the flag is disabled.
  base::Time now = base::Time::Now();
  prefs_.SetTime(prefs::kAdBlockDefaultCacheTimestamp, now);

  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  // Provider timestamp matches cache — with the flag enabled this would be
  // skipped. With the flag disabled it should always load.
  auto provider = std::make_unique<TestFiltersProvider>(
      "||should-still-load.com^", /*engine_is_default=*/true);
  provider->set_timestamp(now);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Filter set should load even when timestamp matches cache";

  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://should-still-load.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

}  // namespace brave_shields
