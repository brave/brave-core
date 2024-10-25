/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_BRAVE_ON_DEMAND_UPDATER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_BRAVE_ON_DEMAND_UPDATER_H_

#include <string>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "components/component_updater/component_updater_service.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_component_updater {

class BraveOnDemandUpdater {
 public:
  static BraveOnDemandUpdater* GetInstance();

  BraveOnDemandUpdater(const BraveOnDemandUpdater&) = delete;
  BraveOnDemandUpdater& operator=(const BraveOnDemandUpdater&) = delete;

  component_updater::OnDemandUpdater* RegisterOnDemandUpdater(
      component_updater::OnDemandUpdater* on_demand_updater);

  // Install the component with the given id. If the component is already
  // installed, do nothing.
  void EnsureInstalled(
      const std::string& id,
      component_updater::Callback callback = base::DoNothing());

  void OnDemandUpdate(const std::string& id,
                      component_updater::OnDemandUpdater::Priority priority,
                      component_updater::Callback callback = base::DoNothing());

  void OnDemandUpdate(const std::vector<std::string>& ids,
                      component_updater::OnDemandUpdater::Priority priority,
                      component_updater::Callback callback = base::DoNothing());

 private:
  friend base::NoDestructor<BraveOnDemandUpdater>;
  BraveOnDemandUpdater();
  ~BraveOnDemandUpdater();

  raw_ptr<component_updater::OnDemandUpdater, DanglingUntriaged>
      on_demand_updater_ = nullptr;
};

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_BRAVE_ON_DEMAND_UPDATER_H_
