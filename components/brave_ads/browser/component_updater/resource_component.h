/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component_observer.h"
#include "brave/components/brave_ads/browser/component_updater/resource_info.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class ResourceComponent : public brave_component_updater::BraveComponent {
 public:
  explicit ResourceComponent(Delegate* delegate);

  ResourceComponent(const ResourceComponent&) = delete;
  ResourceComponent& operator=(const ResourceComponent&) = delete;

  ResourceComponent(ResourceComponent&&) noexcept = delete;
  ResourceComponent& operator=(ResourceComponent&&) noexcept = delete;

  ~ResourceComponent() override;

  void AddObserver(ResourceComponentObserver* observer);
  void RemoveObserver(ResourceComponentObserver* observer);

  void RegisterComponentForCountryCode(const std::string& country_code);
  void RegisterComponentForLanguageCode(const std::string& language_code);

  void NotifyDidUpdateResourceComponent(const std::string& manifest_version,
                                        const std::string& id);
  absl::optional<base::FilePath> GetPath(const std::string& id, int version);

 private:
  void LoadManifestCallback(const std::string& component_id,
                            const base::FilePath& install_dir,
                            const std::string& json);

  void LoadResourceCallback(const std::string& manifest_version,
                            const std::string& component_id,
                            const base::FilePath& install_dir,
                            const std::string& json);

  // brave_component_updater::BraveComponent:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  std::map<std::string, ResourceInfo> resources_;
  base::ObserverList<ResourceComponentObserver> observers_;
  base::WeakPtrFactory<ResourceComponent> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_H_
