/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_service.h"

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/scoped_command_line.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/sponsored_content/sponsored_content_features.h"
#include "brave/components/sponsored_content/test/mock_sponsored_content_service_observer.h"
#include "brave/components/sponsored_content/test/sponsored_content_service_waiter.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/test/test_enabled_state_provider.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/service/test_variations_service.h"
#include "components/variations/variations_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sponsored_content {

class SponsoredContentServiceTest : public testing::Test {
 public:
  SponsoredContentServiceTest() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kSponsoredContentComponent,
        {{"component_update_check_after", "10m"}});

    variations::TestVariationsService::RegisterPrefs(local_state_.registry());
    metrics_state_manager_ = metrics::MetricsStateManager::Create(
        &local_state_, &enabled_state_provider_,
        /*backup_registry_key=*/std::wstring(),
        /*user_data_dir=*/base::FilePath());
    variations_service_ = std::make_unique<variations::TestVariationsService>(
        &local_state_, metrics_state_manager_.get());
    scoped_command_line_.GetProcessCommandLine()->AppendSwitchASCII(
        variations::switches::kVariationsOverrideCountry, "us");

    service_ = std::make_unique<SponsoredContentService>(
        variations_service_.get(), &component_update_service_);
  }

  void TearDown() override {
    // Mirrors production shutdown order so no stray unregister call hits
    // the mock after a test's own expectations are set.
    service_->StartTearDown();
    service_->RemoveObserver(&observer_);
  }

 protected:
  // Performs a real, disk-reading load so `ReplaysOnReAddingSameObserver`
  // and the other replay tests exercise the cached-content path.
  void LoadComponent() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::WriteFile(
        temp_dir_.GetPath().AppendASCII("campaigns.json"), "{}"));

    // Waits for the real, async registration this triggers to finish, so
    // it can't complete later and hit a test's own expectation.
    base::test::TestFuture<void> register_component_test_future;
    EXPECT_CALL(component_update_service_, RegisterComponent)
        .WillOnce(
            testing::DoAll(base::test::RunOnceClosure(
                               register_component_test_future.GetCallback()),
                           testing::Return(true)));
    service_->AddObserver(&observer_);
    ASSERT_TRUE(register_component_test_future.Wait());
    testing::Mock::VerifyAndClearExpectations(&component_update_service_);

    EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate(
                               temp_dir_.GetPath(), /*dict=*/testing::_))
        .Times(1);
    test::SponsoredContentServiceWaiter waiter(*service_);
    service_->LoadComponentFromOverridePath(temp_dir_.GetPath());
    waiter.WaitForOnSponsoredNewTabTakeoverContentDidUpdate();
    // Each test using this helper arms its own fresh expectation for the
    // replay it triggers, independent of this initial load.
    testing::Mock::VerifyAndClearExpectations(&observer_);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  component_updater::MockComponentUpdateService component_update_service_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;

  base::test::ScopedCommandLine scoped_command_line_;
  TestingPrefServiceSimple local_state_;
  metrics::TestEnabledStateProvider enabled_state_provider_{/*consent=*/false,
                                                            /*enabled=*/false};
  std::unique_ptr<metrics::MetricsStateManager> metrics_state_manager_;
  std::unique_ptr<variations::TestVariationsService> variations_service_;

  std::unique_ptr<SponsoredContentService> service_;
  test::MockSponsoredContentServiceObserver observer_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(SponsoredContentServiceTest, RegistersComponentWhenFirstObserverAdded) {
  base::test::TestFuture<void> register_component_test_future;
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .WillOnce(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));

  service_->AddObserver(&observer_);
  ASSERT_TRUE(register_component_test_future.Wait());

  // Set synchronously, independent of whether the async registration
  // completes.
  EXPECT_TRUE(service_->GetComponentId());
}

TEST_F(SponsoredContentServiceTest, DoesNotReregisterWhenSecondObserverAdded) {
  test::MockSponsoredContentServiceObserver second_observer;
  base::test::TestFuture<void> register_component_test_future;
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .Times(1)
      .WillOnce(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));

  service_->AddObserver(&observer_);
  service_->AddObserver(&second_observer);
  ASSERT_TRUE(register_component_test_future.Wait());

  service_->RemoveObserver(&second_observer);
}

TEST_F(SponsoredContentServiceTest, OnlyUnregistersWhenLastObserverRemoved) {
  test::MockSponsoredContentServiceObserver second_observer;
  base::test::TestFuture<void> register_component_test_future;
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .WillOnce(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));
  service_->AddObserver(&observer_);
  service_->AddObserver(&second_observer);
  ASSERT_TRUE(register_component_test_future.Wait());

  EXPECT_CALL(component_update_service_, UnregisterComponent).Times(0);
  service_->RemoveObserver(&second_observer);

  EXPECT_CALL(component_update_service_, UnregisterComponent).Times(1);
  service_->RemoveObserver(&observer_);
}

TEST_F(SponsoredContentServiceTest, ReregistersComponentWhenCountryChanges) {
  base::test::TestFuture<void> register_component_test_future;
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .WillOnce(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));
  service_->AddObserver(&observer_);
  ASSERT_TRUE(register_component_test_future.Wait());

  const std::optional<std::string> us_component_id = service_->GetComponentId();
  ASSERT_TRUE(us_component_id);

  scoped_command_line_.GetProcessCommandLine()->AppendSwitchASCII(
      variations::switches::kVariationsOverrideCountry, "gb");

  base::test::TestFuture<void> reregister_component_test_future;
  EXPECT_CALL(component_update_service_, UnregisterComponent(*us_component_id));
  EXPECT_CALL(observer_, OnSponsoredContentComponentDidUnregister);
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .WillOnce(
          testing::DoAll(base::test::RunOnceClosure(
                             reregister_component_test_future.GetCallback()),
                         testing::Return(true)));
  service_->RegisterSponsoredContentComponent();
  ASSERT_TRUE(reregister_component_test_future.Wait());

  EXPECT_NE(us_component_id, service_->GetComponentId());
}

TEST_F(SponsoredContentServiceTest, DoesNotReplayBeforeAnyContentEverLoaded) {
  base::test::TestFuture<void> register_component_test_future;
  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));

  EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate).Times(0);
  EXPECT_CALL(observer_, OnSponsoredSiteContentDidUpdate).Times(0);
  EXPECT_CALL(observer_, OnSponsoredConversionConfigContentDidUpdate).Times(0);
  service_->AddObserver(&observer_);
  ASSERT_TRUE(register_component_test_future.Wait());
}

TEST_F(SponsoredContentServiceTest,
       MalformedOrMissingContentNotifiesWithNoDict) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(testing::Return(true));
  service_->AddObserver(&observer_);

  // `conversion-config.json` is missing from the temp dir, so the real read
  // fails and this notifies with no dict.
  EXPECT_CALL(observer_, OnSponsoredConversionConfigContentDidUpdate(
                             temp_dir.GetPath(), testing::Eq(std::nullopt)))
      .Times(1);
  test::SponsoredContentServiceWaiter waiter(*service_);
  service_->LoadComponentFromOverridePath(temp_dir.GetPath());
  waiter.WaitForOnSponsoredConversionConfigContentDidUpdate();
}

TEST_F(SponsoredContentServiceTest, MalformedJsonContentNotifiesWithNoDict) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::WriteFile(temp_dir.GetPath().AppendASCII("campaigns.json"),
                              "NOT_VALID_JSON"));

  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(testing::Return(true));
  service_->AddObserver(&observer_);

  // The file exists and reads successfully, but fails to parse as JSON, so
  // this must notify with no dict rather than crash.
  EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate(
                             temp_dir.GetPath(), testing::Eq(std::nullopt)))
      .Times(1);
  test::SponsoredContentServiceWaiter waiter(*service_);
  service_->LoadComponentFromOverridePath(temp_dir.GetPath());
  waiter.WaitForOnSponsoredNewTabTakeoverContentDidUpdate();
}

TEST_F(SponsoredContentServiceTest, ForceComponentUpdateQueuesOnDemandUpdate) {
  // Queues a check on the existing installation instead of re-registering,
  // avoiding a race with Component Updater's own cleanup.
  base::test::TestFuture<void> register_component_test_future;
  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));
  service_->AddObserver(&observer_);
  ASSERT_TRUE(register_component_test_future.Wait());

  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(),
                     component_updater::OnDemandUpdater::Priority::FOREGROUND,
                     /*callback=*/testing::_));
  service_->ForceComponentUpdate();
}

TEST_F(SponsoredContentServiceTest, ForceComponentUpdateReplaysCurrentContent) {
  LoadComponent();

  // Uses `observer_` directly, not a second observer, so only
  // `ForceComponentUpdate()` itself can trigger this call.
  base::test::TestFuture<void> content_updated_test_future;
  EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate(
                             temp_dir_.GetPath(), testing::_))
      .WillOnce(base::test::RunOnceClosure(
          content_updated_test_future.GetCallback()));
  service_->ForceComponentUpdate();
  ASSERT_TRUE(content_updated_test_future.Wait());
}

TEST_F(SponsoredContentServiceTest,
       DoesNotCheckForUpdateBeforeFirstRegistration) {
  // `last_updated_at_` is only set once a component has registered, so this
  // must be a safe no-op before that ever happens.
  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(),
                     component_updater::OnDemandUpdater::Priority::FOREGROUND,
                     /*callback=*/testing::_))
      .Times(0);
  service_->MaybeCheckForComponentUpdate();
}

TEST_F(SponsoredContentServiceTest,
       DoesNotCheckForUpdateBeforeIntervalElapses) {
  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(testing::Return(true));
  service_->AddObserver(&observer_);

  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(),
                     component_updater::OnDemandUpdater::Priority::FOREGROUND,
                     /*callback=*/testing::_))
      .Times(0);
  service_->MaybeCheckForComponentUpdate();
}

TEST_F(SponsoredContentServiceTest, ChecksForUpdateOnceIntervalElapses) {
  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(testing::Return(true));
  service_->AddObserver(&observer_);

  // Advances the clock without running the timer, reaching the overdue
  // branch deterministically.
  task_environment_.AdvanceClock(base::Minutes(10) + base::Seconds(1));

  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(),
                     component_updater::OnDemandUpdater::Priority::FOREGROUND,
                     /*callback=*/testing::_));
  service_->MaybeCheckForComponentUpdate();
}

TEST_F(SponsoredContentServiceTest,
       StartTearDownStopsCallingComponentUpdateService) {
  base::test::TestFuture<void> register_component_test_future;
  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(
          testing::DoAll(base::test::RunOnceClosure(
                             register_component_test_future.GetCallback()),
                         testing::Return(true)));
  service_->AddObserver(&observer_);
  ASSERT_TRUE(register_component_test_future.Wait());

  service_->StartTearDown();

  // `component_update_service_` is already null, so this must not call
  // through to it, and must not crash.
  EXPECT_CALL(component_update_service_, UnregisterComponent).Times(0);
  service_->RemoveObserver(&observer_);
}

TEST_F(SponsoredContentServiceTest,
       StartTearDownClearsVariationsServiceBeforeItIsDestroyed) {
  // Mirrors production, where `BrowserProcessImpl::StartTearDown()` destroys
  // the real `VariationsService` well before this class is itself destroyed.
  auto local_variations_service =
      std::make_unique<variations::TestVariationsService>(
          &local_state_, metrics_state_manager_.get());
  auto local_service = std::make_unique<SponsoredContentService>(
      local_variations_service.get(), &component_update_service_);

  local_service->StartTearDown();
  local_variations_service.reset();

  // `StartTearDown` already makes this a no-op via `component_update_service_`
  // alone; the real assertion is that getting here never reads the
  // already-destroyed `VariationsService`.
  EXPECT_CALL(component_update_service_, RegisterComponent).Times(0);
  local_service->RegisterSponsoredContentComponent();
}

// Covers the real, disk-reading replay path, reading fresh rather than
// from any cache.
TEST_F(SponsoredContentServiceTest, ReplaysOnReAddingSameObserver) {
  LoadComponent();

  EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate(
                             temp_dir_.GetPath(), testing::_))
      .Times(1);
  test::SponsoredContentServiceWaiter waiter(*service_);
  service_->AddObserver(&observer_);
  waiter.WaitForOnSponsoredNewTabTakeoverContentDidUpdate();
}

TEST_F(SponsoredContentServiceTest,
       DeliversCurrentContentToObserverAddedAfterInitialLoad) {
  LoadComponent();

  test::MockSponsoredContentServiceObserver second_observer;
  EXPECT_CALL(second_observer, OnSponsoredNewTabTakeoverContentDidUpdate(
                                   temp_dir_.GetPath(), testing::_))
      .Times(1);
  test::SponsoredContentServiceWaiter waiter(*service_);
  service_->AddObserver(&second_observer);
  waiter.WaitForOnSponsoredNewTabTakeoverContentDidUpdate();

  service_->RemoveObserver(&second_observer);
}

TEST_F(SponsoredContentServiceTest,
       ReplaysContentInsteadOfReregisteringWhenCountryIsUnchanged) {
  LoadComponent();

  EXPECT_CALL(component_update_service_, RegisterComponent).Times(0);
  EXPECT_CALL(component_update_service_, UnregisterComponent).Times(0);
  EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate(
                             temp_dir_.GetPath(), testing::_))
      .Times(1);
  test::SponsoredContentServiceWaiter waiter(*service_);
  service_->RegisterSponsoredContentComponent();
  waiter.WaitForOnSponsoredNewTabTakeoverContentDidUpdate();
}

TEST_F(SponsoredContentServiceTest,
       DoesNotReplayPreviousCountrysContentWhenCountryChanges) {
  LoadComponent();

  scoped_command_line_.GetProcessCommandLine()->AppendSwitchASCII(
      variations::switches::kVariationsOverrideCountry, "gb");

  base::test::TestFuture<void> reregister_component_test_future;
  ON_CALL(component_update_service_, RegisterComponent)
      .WillByDefault(
          testing::DoAll(base::test::RunOnceClosure(
                             reregister_component_test_future.GetCallback()),
                         testing::Return(true)));
  EXPECT_CALL(observer_, OnSponsoredContentComponentDidUnregister).Times(1);
  // The new country's component has not loaded yet, so the previous
  // country's content must not be replayed.
  EXPECT_CALL(observer_, OnSponsoredNewTabTakeoverContentDidUpdate).Times(0);
  service_->RegisterSponsoredContentComponent();
  ASSERT_TRUE(reregister_component_test_future.Wait());
}

}  // namespace sponsored_content
