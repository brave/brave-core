// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_DEFAULT_RESOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_DEFAULT_RESOURCE_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace base {
class FilePath;
}

class AdBlockServiceTest;

namespace brave_shields {

class AdBlockDefaultResourceProvider : public AdBlockResourceProvider {
 public:
  explicit AdBlockDefaultResourceProvider(
      component_updater::ComponentUpdateService* cus);
  ~AdBlockDefaultResourceProvider() override;
  AdBlockDefaultResourceProvider(const AdBlockDefaultResourceProvider&) =
      delete;
  AdBlockDefaultResourceProvider& operator=(
      const AdBlockDefaultResourceProvider&) = delete;

  /// Returns the path to the resources file.
  base::FilePath GetResourcesPath();

  /// Returns the cached resources JSON if available.
  const std::string& GetResourcesJson() const { return resources_json_; }

  void LoadResources(
      base::OnceCallback<void(BraveResourceStorageBox)>) override;

 private:
  friend class ::AdBlockServiceTest;

  void OnComponentReady(const base::FilePath&);
  void OnResourcesLoaded(const std::string& resources_json);
  void OnLoadResourcesLoaded(
      base::OnceCallback<void(BraveResourceStorageBox)> cb,
      const std::string& resources_json);

  base::FilePath component_path_;
  std::string resources_json_;  // Cached resources JSON

  base::WeakPtrFactory<AdBlockDefaultResourceProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_DEFAULT_RESOURCE_PROVIDER_H_
