/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_on_demand_update.h"

#include <string>

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sponsored_content {

namespace {
constexpr char kComponentId[] = "abcdefghijklmnopqrstuvwxyzabcdef";
}  // namespace

class SponsoredContentOnDemandUpdateTest : public testing::Test {
 protected:
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
};

TEST_F(SponsoredContentOnDemandUpdateTest, QueuesUpdateWithForegroundPriority) {
  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(std::string(kComponentId),
                     component_updater::OnDemandUpdater::Priority::FOREGROUND,
                     /*callback=*/testing::_));

  QueueSponsoredContentComponentUpdate(kComponentId);
}

// Only reachable by actually running the callback passed to `OnDemandUpdate`,
// which a bare `EXPECT_CALL` with no action never does.
class SponsoredContentOnDemandUpdateCompletionTest
    : public SponsoredContentOnDemandUpdateTest,
      public testing::WithParamInterface<update_client::Error> {};

TEST_P(SponsoredContentOnDemandUpdateCompletionTest,
       CompletionCallbackHandlesEveryErrorValueWithoutCrashing) {
  const update_client::Error error = GetParam();
  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(),
                     component_updater::OnDemandUpdater::Priority::FOREGROUND,
                     /*callback=*/testing::_))
      .WillOnce([error](const std::string& /*id*/,
                        component_updater::OnDemandUpdater::Priority
                        /*priority*/,
                        component_updater::Callback callback) {
        std::move(callback).Run(error);
      });

  QueueSponsoredContentComponentUpdate(kComponentId);
}

INSTANTIATE_TEST_SUITE_P(
    AllErrorValues,
    SponsoredContentOnDemandUpdateCompletionTest,
    testing::Values(update_client::Error::NONE,
                    update_client::Error::UPDATE_IN_PROGRESS,
                    update_client::Error::UPDATE_CANCELED,
                    update_client::Error::RETRY_LATER,
                    update_client::Error::SERVICE_ERROR,
                    update_client::Error::UPDATE_CHECK_ERROR,
                    update_client::Error::CRX_NOT_FOUND,
                    update_client::Error::INVALID_ARGUMENT,
                    update_client::Error::BAD_CRX_DATA_CALLBACK,
                    update_client::Error::UPDATE_DISABLED));

TEST_F(SponsoredContentOnDemandUpdateTest,
       DoesNotQueueUpdateWhenComponentUpdatesAreDisabled) {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(/*is_component_update_disabled=*/true,
                                &on_demand_updater_);

  EXPECT_CALL(on_demand_updater_,
              OnDemandUpdate(testing::An<const std::string&>(),
                             /*priority=*/testing::_,
                             /*callback=*/testing::_))
      .Times(0);

  QueueSponsoredContentComponentUpdate(kComponentId);

  // Restore, so a later test in this binary is not affected.
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(/*is_component_update_disabled=*/false,
                                &on_demand_updater_);
}

}  // namespace sponsored_content
