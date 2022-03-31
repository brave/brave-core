/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_CUSTOM_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_CUSTOM_FILTERS_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

using brave_component_updater::DATFileDataBuffer;

class PrefService;

namespace brave_shields {

class AdBlockCustomFiltersProvider : public AdBlockFiltersProvider {
 public:
  explicit AdBlockCustomFiltersProvider(PrefService* local_state);
  ~AdBlockCustomFiltersProvider() override;
  AdBlockCustomFiltersProvider(const AdBlockCustomFiltersProvider&) = delete;
  AdBlockCustomFiltersProvider& operator=(const AdBlockCustomFiltersProvider&) =
      delete;

  std::string GetCustomFilters();
  bool UpdateCustomFilters(const std::string& custom_filters);

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

 private:
  PrefService* local_state_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_CUSTOM_FILTERS_PROVIDER_H_
