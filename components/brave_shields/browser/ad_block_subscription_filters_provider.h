/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_FILTERS_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

using brave_component_updater::DATFileDataBuffer;

class PrefService;

namespace adblock {
struct FilterListMetadata;
}  // namespace adblock

namespace brave_shields {

class AdBlockSubscriptionFiltersProvider : public AdBlockFiltersProvider {
 public:
  AdBlockSubscriptionFiltersProvider(
      PrefService* local_state,
      base::FilePath list_file,
      base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
          on_metadata_retrieved);
  AdBlockSubscriptionFiltersProvider(
      const AdBlockSubscriptionFiltersProvider&) = delete;
  AdBlockSubscriptionFiltersProvider& operator=(
      const AdBlockSubscriptionFiltersProvider&) = delete;
  ~AdBlockSubscriptionFiltersProvider() override;

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

  void OnDATFileDataReady(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)> cb,
      const DATFileDataBuffer& dat_buf);

  void OnListAvailable();

  std::string GetNameForDebugging() override;

 private:
  base::FilePath list_file_;

  base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
      on_metadata_retrieved_;

  base::WeakPtrFactory<AdBlockSubscriptionFiltersProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_FILTERS_PROVIDER_H_
