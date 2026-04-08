// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_service.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/hash/hash.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
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
  static std::string HashOf(std::string_view content) {
    return base::NumberToString(base::FastHash(std::string(content)));
  }

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

  // Pre-populates the cache hash prefs with values matching the current
  // provider set. This simulates a previous session that cached the DAT files.
  void SetCacheHashesFromProviders() {
    auto service = CreateService();
    prefs_.SetString(prefs::kAdBlockDefaultCacheHash,
                     service->ComputeCombinedCacheKeyForTesting(true));
    prefs_.SetString(prefs::kAdBlockAdditionalCacheHash,
                     service->ComputeCombinedCacheKeyForTesting(false));
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

  // Pre-populate cache hashes to match the current provider set, simulating
  // a previous session that cached these DAT files.
  SetCacheHashesFromProviders();

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

  SetCacheHashesFromProviders();

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

  SetCacheHashesFromProviders();

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
  // Set the additional engine cache hash but leave the default cache hash
  // empty, meaning the default engine should always load.
  prefs_.SetString(prefs::kAdBlockAdditionalCacheHash, HashOf(""));

  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  // Register a test filters provider for the default engine. The default cache
  // hash is empty, so ShouldLoadFilterState returns true and loading proceeds.
  auto provider =
      std::make_unique<TestFiltersProvider>("||default-rule.com^",
                                            /*engine_is_default=*/true);
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

TEST_F(AdBlockServiceTest, MismatchedCacheHashTriggersReload) {
  // A stale cache hash that doesn't match the provider's content hash should
  // trigger a filter set reload.
  prefs_.SetString(prefs::kAdBlockDefaultCacheHash, "stale_hash");

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
      std::make_unique<TestFiltersProvider>("||after-stale-cache.com^",
                                            /*engine_is_default=*/true);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Timeout: mismatched cache hash should have triggered reload";

  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://after-stale-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

TEST_F(AdBlockServiceTest, DifferentProviderContentTriggersReload) {
  // When a provider's content hash doesn't match the cached hash, the filter
  // set should be loaded even though a valid cache exists.
  prefs_.SetString(prefs::kAdBlockDefaultCacheHash, HashOf("old-content"));

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

  // Register a provider whose content hash won't match the cached hash.
  auto provider = std::make_unique<TestFiltersProvider>(
      "||new-filter-rule.com^", /*engine_is_default=*/true);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Timeout: different provider content should have triggered reload";

  // The new filter set rules should be active, not the old cache.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://new-filter-rule.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

TEST_F(AdBlockServiceTest, ProviderWithNulloptCacheKeyTriggersReload) {
  // When a provider's GetCacheKey() returns nullopt (e.g. after invalidation),
  // the filter set should still reload rather than being blocked. This tests
  // that the hash computation in ShouldLoadFilterState handles nullopt
  // providers correctly.
  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  // Register a provider that returns nullopt from GetCacheKey(), simulating
  // a provider whose cache key has been invalidated (e.g. subscription after
  // download).
  auto provider =
      std::make_unique<TestFiltersProvider>("||reloaded-rule.com^",
                                            /*engine_is_default=*/true);
  provider->set_cache_key_nullopt();
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Timeout: filter set should load even when a provider has no "
         "cache key";

  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://reloaded-rule.com/script.js"),
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

  // Drain thread pool (DAT serialize + write) and main thread (OnDatCached
  // reply) so the cache hash pref is written.
  task_environment_.RunUntilIdle();
  ASSERT_NE(prefs_.GetString(prefs::kAdBlockAdditionalCacheHash), "");

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

TEST_F(AdBlockServiceQueuedTest, MatchingCacheHashSkipsFilterSetLoad) {
  // When cache hash matches the provider's combined content hash, filter set
  // loading should be skipped (ShouldLoadFilterState returns false).
  // This test requires queued execution to ensure the DAT loads before any
  // filter set loading would occur.
  CreateCachedDefaultDATFile("||from-cache.com^\n");

  // Pre-compute the cache hashes that will match the initial provider set
  // (only localhost provider for default, only custom filters for additional).
  // Use a temporary service to get the hashes, then destroy it.
  {
    auto temp_service = CreateServiceWithTaskRunner(
        base::MakeRefCounted<base::TestMockTimeTaskRunner>());
    prefs_.SetString(prefs::kAdBlockDefaultCacheHash,
                     temp_service->ComputeCombinedCacheKeyForTesting(true));
    prefs_.SetString(prefs::kAdBlockAdditionalCacheHash,
                     temp_service->ComputeCombinedCacheKeyForTesting(false));
  }

  auto service_task_runner =
      base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  auto service = CreateServiceWithTaskRunner(service_task_runner);

  // Let the DAT loading complete.
  task_environment_.RunUntilIdle();
  while (service_task_runner->HasPendingTask()) {
    service_task_runner->FastForwardBy(
        service_task_runner->NextPendingTaskDelay());
    base::RunLoop().RunUntilIdle();
  }
  base::RunLoop().RunUntilIdle();

  // The cached DAT rules should be loaded since cache hash matched and
  // ShouldLoadFilterState returned false, skipping filter set loading.
  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Drain any remaining tasks.
  while (service_task_runner->HasPendingTask()) {
    service_task_runner->FastForwardBy(
        service_task_runner->NextPendingTaskDelay());
    base::RunLoop().RunUntilIdle();
  }
  base::RunLoop().RunUntilIdle();

  // Cached DAT rules should still be active (no filter set overrode them).
  result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://from-cache.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);

  // Now register a new provider. Update the cache hash to match the new
  // combined hash so ShouldLoadFilterState returns false (skip).
  auto provider = std::make_unique<TestFiltersProvider>(
      "||from-filter-set.com^", /*engine_is_default=*/true);
  provider->RegisterAsSourceProvider(service.get());
  prefs_.SetString(prefs::kAdBlockDefaultCacheHash,
                   service->ComputeCombinedCacheKeyForTesting(true));

  // Drain tasks from the provider registration.
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
  // Create cached DAT files and set valid cache hashes.
  CreateCachedDATFiles("||from-cache.com^\n", "");
  prefs_.SetString(prefs::kAdBlockDefaultCacheHash,
                   HashOf("||from-filter-set.com^"));
  prefs_.SetString(prefs::kAdBlockAdditionalCacheHash, HashOf(""));

  auto service = CreateService();

  // Register a provider for the default engine -- with the flag disabled,
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
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Filter set should load even when cache hashes are set";

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
       FilterSetAlwaysLoadsWithMatchingHash) {
  // Even with a cache hash that matches the provider's content hash (which
  // would normally skip loading), it should still load when the flag is
  // disabled.
  prefs_.SetString(prefs::kAdBlockDefaultCacheHash,
                   HashOf("||should-still-load.com^"));

  auto service = CreateService();

  bool default_filter_list_loaded = false;
  FilterListObserver observer(
      base::BindLambdaForTesting([&](bool is_default, bool success) {
        if (is_default) {
          default_filter_list_loaded = success;
        }
      }));
  service->AddObserver(&observer);

  // Provider content hash matches cache -- with the flag enabled this would be
  // skipped. With the flag disabled it should always load.
  auto provider = std::make_unique<TestFiltersProvider>(
      "||should-still-load.com^", /*engine_is_default=*/true);
  provider->RegisterAsSourceProvider(service.get());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return default_filter_list_loaded;
  })) << "Filter set should load even when hash matches cache";

  auto result = service->GetDefaultEngineForTesting().ShouldStartRequest(
      GURL("https://should-still-load.com/script.js"),
      blink::mojom::ResourceType::kScript, "test.com", false, false, false);
  EXPECT_TRUE(result.matched);
}

}  // namespace brave_shields
