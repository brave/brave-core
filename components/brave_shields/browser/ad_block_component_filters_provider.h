/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_COMPONENT_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_COMPONENT_FILTERS_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

using brave_component_updater::DATFileDataBuffer;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace base {
class FilePath;
}

class AdBlockServiceTest;
class DebounceBrowserTest;

namespace brave_shields {

class FilterListCatalogEntry;

class AdBlockComponentFiltersProvider : public AdBlockFiltersProvider {
 public:
  // Build an adblock filters component with given component info
  AdBlockComponentFiltersProvider(
      component_updater::ComponentUpdateService* cus,
      std::string component_id,
      std::string base64_public_key,
      std::string title,
      bool is_default_engine = true);
  // Helper to build a particular adblock component from a catalog entry
  AdBlockComponentFiltersProvider(
      component_updater::ComponentUpdateService* cus,
      const FilterListCatalogEntry& catalog_entry,
      bool is_default_engine = true);
  ~AdBlockComponentFiltersProvider() override;
  AdBlockComponentFiltersProvider(const AdBlockComponentFiltersProvider&) =
      delete;
  AdBlockComponentFiltersProvider& operator=(
      const AdBlockComponentFiltersProvider&) = delete;

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

  // Remove the component. This will force it to be redownloaded next time it
  // is registered.
  void UnregisterComponent();

  std::string GetNameForDebugging() override;

 private:
  friend class ::AdBlockServiceTest;
  friend class ::DebounceBrowserTest;

  void OnComponentReady(const base::FilePath&);

  base::FilePath component_path_;
  std::string component_id_;
  const raw_ptr<component_updater::ComponentUpdateService>
      component_updater_service_;

  base::WeakPtrFactory<AdBlockComponentFiltersProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_COMPONENT_FILTERS_PROVIDER_H_
