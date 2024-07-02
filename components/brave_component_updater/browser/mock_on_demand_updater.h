/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_MOCK_ON_DEMAND_UPDATER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_MOCK_ON_DEMAND_UPDATER_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "components/component_updater/component_updater_service.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_component_updater {

class MockOnDemandUpdater : public component_updater::OnDemandUpdater {
 public:
  MockOnDemandUpdater();
  ~MockOnDemandUpdater() override;

  MOCK_METHOD(void,
              OnDemandInstall,
              (const std::string& id, component_updater::Callback callback),
              (override));
  MOCK_METHOD(void,
              OnDemandUpdate,
              (const std::string& id,
               component_updater::OnDemandUpdater::Priority priority,
               component_updater::Callback callback),
              (override));
  MOCK_METHOD(void,
              OnDemandUpdate,
              (const std::vector<std::string>& ids,
               component_updater::OnDemandUpdater::Priority priority,
               component_updater::Callback callback),
              (override));

 private:
  raw_ptr<component_updater::OnDemandUpdater> prev_on_demand_updater_ = nullptr;
};

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_MOCK_ON_DEMAND_UPDATER_H_
