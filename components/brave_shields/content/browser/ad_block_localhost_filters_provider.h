// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_LOCALHOST_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_LOCALHOST_FILTERS_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

class AdBlockLocalhostFiltersProvider : public AdBlockFiltersProvider {
 public:
  explicit AdBlockLocalhostFiltersProvider(
      AdBlockFiltersProviderManager* manager);
  ~AdBlockLocalhostFiltersProvider() override;
  AdBlockLocalhostFiltersProvider(const AdBlockLocalhostFiltersProvider&) =
      delete;
  AdBlockLocalhostFiltersProvider& operator=(
      const AdBlockLocalhostFiltersProvider&) = delete;

  std::string GetLocalhostFilters();
  bool UpdateLocalhostFilters(const std::string& localhost_filters);

  // AdBlockFiltersProvider
  void LoadFilters(
      base::OnceCallback<void(std::vector<unsigned char> filter_buffer,
                              uint8_t permission_mask)>) override;

  std::string GetNameForDebugging() override;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_LOCALHOST_FILTERS_PROVIDER_H_
