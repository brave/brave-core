/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_REGISTRAR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_REGISTRAR_H_

#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave_ads {

class ResourceComponentRegistrarDelegate;

class ResourceComponentRegistrar
    : public brave_component_updater::BraveComponent {
 public:
  ResourceComponentRegistrar(Delegate* component_updater_delegate,
                             ResourceComponentRegistrarDelegate&
                                 resource_component_registrar_delegate);

  ResourceComponentRegistrar(const ResourceComponentRegistrar&) = delete;
  ResourceComponentRegistrar& operator=(const ResourceComponentRegistrar&) =
      delete;

  ~ResourceComponentRegistrar() override;

  void RegisterResourceComponent(const std::string& resource_id);

 private:
  // brave_component_updater::BraveComponent:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  void OnComponentUnregistered(const std::string& component_id);

  const raw_ref<ResourceComponentRegistrarDelegate>
      resource_component_registrar_delegate_;
  std::optional<std::string> resource_component_id_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_REGISTRAR_H_
