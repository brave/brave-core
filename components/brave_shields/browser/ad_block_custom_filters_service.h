/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_CUSTOM_FILTERS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_CUSTOM_FILTERS_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_shields/browser/ad_block_base_service.h"

class AdBlockServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

// The brave shields service in charge of custom filter ad-block
// checking and init.
class AdBlockCustomFiltersService : public AdBlockBaseService {
 public:
  explicit AdBlockCustomFiltersService(BraveComponent::Delegate* delegate);
  AdBlockCustomFiltersService(const AdBlockCustomFiltersService&) = delete;
  AdBlockCustomFiltersService& operator=(const AdBlockCustomFiltersService&) =
      delete;
  ~AdBlockCustomFiltersService() override;

  std::string GetCustomFilters();
  bool UpdateCustomFilters(const std::string& custom_filters);

 protected:
  bool Init() override;

 private:
  friend class ::AdBlockServiceTest;
  void UpdateCustomFiltersOnFileTaskRunner(const std::string& custom_filters);
};

// Creates the AdBlockCustomFiltersService
std::unique_ptr<AdBlockCustomFiltersService>
AdBlockCustomFiltersServiceFactory(BraveComponent::Delegate* delegate);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_CUSTOM_FILTERS_SERVICE_H_
