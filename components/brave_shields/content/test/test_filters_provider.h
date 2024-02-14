// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_TEST_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_TEST_FILTERS_PROVIDER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/content/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

class TestFiltersProvider : public AdBlockFiltersProvider,
                            public AdBlockResourceProvider {
 public:
  TestFiltersProvider(const std::string& rules, const std::string& resources);
  TestFiltersProvider(const std::string& rules,
                      const std::string& resources,
                      bool engine_is_default,
                      uint8_t permission_mask = 0);
  ~TestFiltersProvider() override;

  void LoadFilterSet(
      base::OnceCallback<void(std::pair<uint8_t, DATFileDataBuffer>)>) override;

  void LoadResources(
      base::OnceCallback<void(const std::string& resources_json)> cb) override;

  std::string GetNameForDebugging() override;

 private:
  std::string rules_;
  std::string resources_;
  uint8_t permission_mask_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_TEST_FILTERS_PROVIDER_H_
