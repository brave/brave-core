// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_CUSTOM_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_CUSTOM_FILTERS_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/adblock/rs/src/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

using brave_component_updater::DATFileDataBuffer;

class PrefService;

namespace brave_shields {

class AdBlockCustomFiltersProvider : public AdBlockFiltersProvider {
 public:
  explicit AdBlockCustomFiltersProvider(PrefService* local_state,
                                        AdBlockFiltersProviderManager* manager);
  ~AdBlockCustomFiltersProvider() override;
  AdBlockCustomFiltersProvider(const AdBlockCustomFiltersProvider&) = delete;
  AdBlockCustomFiltersProvider& operator=(const AdBlockCustomFiltersProvider&) =
      delete;

  void AddUserCosmeticFilter(std::string_view filter);
  void CreateSiteExemption(std::string_view host);

  bool AreAnyBlockedElementsPresent(std::string_view host);
  void ResetCosmeticFilter(std::string_view host);

  std::string GetCustomFilters();
  bool UpdateCustomFilters(std::string_view custom_filters);

  // Used in BraveAdBlockHandler and updates the manually edited custom filters
  // only if developer mode is turned on.
  bool UpdateCustomFiltersFromSettings(PrefService* profile_prefs,
                                       std::string_view custom_filters);

  // AdBlockFiltersProvider
  void LoadFilterSet(
      base::OnceCallback<void(
          base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)>) override;

  std::string GetNameForDebugging() override;

 private:
  void AppendCustomFilter(std::string_view filter);

  const raw_ptr<PrefService> local_state_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_CUSTOM_FILTERS_PROVIDER_H_
