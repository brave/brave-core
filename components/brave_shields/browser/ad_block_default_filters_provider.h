/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_FILTERS_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_regional_catalog_provider.h"
#include "brave/components/brave_shields/browser/ad_block_resource_provider.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_component_updater::DATFileDataBuffer;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace base {
class FilePath;
}

class AdBlockServiceTest;

namespace brave_shields {

class AdBlockDefaultFiltersProvider : public AdBlockFiltersProvider,
                                      public AdBlockResourceProvider,
                                      public AdBlockRegionalCatalogProvider {
 public:
  explicit AdBlockDefaultFiltersProvider(
      component_updater::ComponentUpdateService* cus);
  ~AdBlockDefaultFiltersProvider() override;
  AdBlockDefaultFiltersProvider(const AdBlockDefaultFiltersProvider&) = delete;
  AdBlockDefaultFiltersProvider& operator=(
      const AdBlockDefaultFiltersProvider&) = delete;

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

  void LoadResources(
      base::OnceCallback<void(const std::string& resources_json)>) override;

  void LoadRegionalCatalog(
      base::OnceCallback<void(const std::string& catalog_json)>) override;

 private:
  friend class ::AdBlockServiceTest;
  void OnComponentReady(const base::FilePath&);

  base::FilePath component_path_;

  base::WeakPtrFactory<AdBlockDefaultFiltersProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_FILTERS_PROVIDER_H_
