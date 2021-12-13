/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_SOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_SOURCE_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_regional_catalog_provider.h"
#include "brave/components/brave_shields/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/browser/ad_block_source_provider.h"
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

class AdBlockDefaultSourceProvider : public AdBlockSourceProvider,
                                     public AdBlockResourceProvider,
                                     public AdBlockRegionalCatalogProvider {
 public:
  explicit AdBlockDefaultSourceProvider(
      component_updater::ComponentUpdateService* cus);
  ~AdBlockDefaultSourceProvider() override;

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

  base::WeakPtrFactory<AdBlockDefaultSourceProvider> weak_factory_{this};

  AdBlockDefaultSourceProvider(const AdBlockDefaultSourceProvider&) = delete;
  AdBlockDefaultSourceProvider& operator=(const AdBlockDefaultSourceProvider&) =
      delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_SOURCE_PROVIDER_H_
