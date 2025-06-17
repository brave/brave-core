// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/p3a_service.h"

#include <memory>

#include "base/metrics/histogram_functions.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

constexpr char kTestExpressHistogramName[] = "Brave.Core.UsageDaily";
constexpr char kTestSlowHistogramName[] = "Brave.Core.UsageMonthly";
constexpr char kTestTypicalHistogramName[] = "Brave.Core.IsDefault";

class P3AServiceTest : public testing::Test {
 public:
  P3AServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    P3AService::RegisterPrefs(local_state_.registry(), true);
  }

  void TearDown() override { p3a_service_ = nullptr; }

  void CreateP3AService() {
    base::Time install_time;
    ASSERT_TRUE(base::Time::FromString("2049-01-01", &install_time));
    p3a_service_ = scoped_refptr(
        new P3AService(local_state_, "release", install_time, {}));
    p3a_service_->Init(shared_url_loader_factory_);
  }

  void TriggerRemoteConfigLoad() {
    // Set the remote config as loaded and trigger the callback
    p3a_service_->remote_config_manager()->SetIsLoadedForTesting(true);
    p3a_service_->OnRemoteConfigLoaded();
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  scoped_refptr<P3AService> p3a_service_;
  TestingPrefServiceSimple local_state_;
};

TEST_F(P3AServiceTest, MessageManagerStartedWhenP3AEnabled) {
  local_state_.SetBoolean(kP3AEnabled, true);
  CreateP3AService();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  TriggerRemoteConfigLoad();
  EXPECT_TRUE(p3a_service_->message_manager_->IsActive());
}

TEST_F(P3AServiceTest, MessageManagerNotStartedWhenP3ADisabled) {
  local_state_.SetBoolean(kP3AEnabled, false);
  CreateP3AService();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  TriggerRemoteConfigLoad();
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());
}

TEST_F(P3AServiceTest, MessageManagerStartsAndStopsOnPrefChange) {
  local_state_.SetBoolean(kP3AEnabled, false);
  CreateP3AService();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  TriggerRemoteConfigLoad();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  local_state_.SetBoolean(kP3AEnabled, true);
  EXPECT_TRUE(p3a_service_->message_manager_->IsActive());

  local_state_.SetBoolean(kP3AEnabled, false);
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());
}

TEST_F(P3AServiceTest, MetricValueStored) {
  local_state_.SetBoolean(kP3AEnabled, true);

  CreateP3AService();

  EXPECT_TRUE(local_state_.GetDict(kTypicalConstellationPrepPrefName).empty());
  EXPECT_TRUE(local_state_.GetDict(kExpressConstellationPrepPrefName).empty());
  EXPECT_TRUE(local_state_.GetDict(kSlowConstellationPrepPrefName).empty());

  base::UmaHistogramExactLinear(kTestTypicalHistogramName, 0, 10);
  p3a_service_->OnHistogramChanged(kTestTypicalHistogramName, 1, 0);
  task_environment_.FastForwardBy(base::Seconds(3));

  EXPECT_TRUE(local_state_.GetDict(kTypicalConstellationPrepPrefName).empty());

  TriggerRemoteConfigLoad();

  const auto* stored_log =
      local_state_.GetDict(kTypicalConstellationPrepPrefName)
          .FindDict(kTestTypicalHistogramName);
  EXPECT_TRUE(stored_log != nullptr);

  EXPECT_TRUE(local_state_.GetDict(kExpressConstellationPrepPrefName).empty());

  base::UmaHistogramExactLinear(kTestExpressHistogramName, 0, 10);
  p3a_service_->OnHistogramChanged(kTestExpressHistogramName, 1, 0);
  task_environment_.FastForwardBy(base::Seconds(3));

  stored_log = local_state_.GetDict(kExpressConstellationPrepPrefName)
                   .FindDict(kTestExpressHistogramName);
  EXPECT_TRUE(stored_log != nullptr);

  EXPECT_TRUE(local_state_.GetDict(kSlowConstellationPrepPrefName).empty());

  base::UmaHistogramExactLinear(kTestSlowHistogramName, 0, 10);
  p3a_service_->OnHistogramChanged(kTestSlowHistogramName, 1, 0);
  task_environment_.FastForwardBy(base::Seconds(3));

  stored_log = local_state_.GetDict(kSlowConstellationPrepPrefName)
                   .FindDict(kTestSlowHistogramName);
  EXPECT_TRUE(stored_log != nullptr);
}

}  // namespace p3a
