/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_REGISTRAR_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_REGISTRAR_DELEGATE_H_

#include <string>

namespace base {
class FilePath;
}  // namespace base

namespace brave_ads {

class ResourceComponentRegistrarDelegate {
 public:
  // Invoked when a resource component is registered.
  virtual void OnResourceComponentRegistered(
      const std::string& component_id,
      const base::FilePath& install_dir) = 0;

  // Invoked when a resource component is unregistered.
  virtual void OnResourceComponentUnregistered(
      const std::string& component_id) = 0;

 protected:
  virtual ~ResourceComponentRegistrarDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_COMPONENT_REGISTRAR_DELEGATE_H_
