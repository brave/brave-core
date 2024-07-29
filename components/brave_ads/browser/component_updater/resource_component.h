/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_H_

#include <map>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component_observer.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component_registrar.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component_registrar_delegate.h"
#include "brave/components/brave_ads/browser/component_updater/resource_info.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

namespace brave_ads {

class ResourceComponent : public ResourceComponentRegistrarDelegate {
 public:
  explicit ResourceComponent(
      brave_component_updater::BraveComponent::Delegate* delegate);

  ResourceComponent(const ResourceComponent&) = delete;
  ResourceComponent& operator=(const ResourceComponent&) = delete;

  ResourceComponent(ResourceComponent&&) noexcept = delete;
  ResourceComponent& operator=(ResourceComponent&&) noexcept = delete;

  ~ResourceComponent() override;

  void AddObserver(ResourceComponentObserver* observer);
  void RemoveObserver(ResourceComponentObserver* observer);

  void RegisterComponentForCountryCode(const std::string& country_code);
  void RegisterComponentForLanguageCode(const std::string& language_code);

  std::optional<base::FilePath> MaybeGetPath(const std::string& id,
                                             int version);

 private:
  void LoadManifestCallback(const std::string& component_id,
                            const base::FilePath& install_dir,
                            const std::string& json);

  void LoadResourceCallback(const std::string& manifest_version,
                            const std::string& component_id,
                            const base::FilePath& install_dir,
                            const std::string& json);

  void NotifyResourceComponentDidChange(const std::string& manifest_version,
                                        const std::string& id);
  void NotifyDidUnregisterResourceComponent(const std::string& id);

  // ResourceComponentRegistrarDelegate:
  void OnResourceComponentRegistered(
      const std::string& component_id,
      const base::FilePath& install_dir) override;
  void OnResourceComponentUnregistered(
      const std::string& component_id) override;

  base::ObserverList<ResourceComponentObserver> observers_;

  ResourceComponentRegistrar country_resource_component_registrar_;
  ResourceComponentRegistrar language_resource_component_registrar_;
  std::map</*resource_key*/ std::string, ResourceInfo> resources_;

  base::WeakPtrFactory<ResourceComponent> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_H_
