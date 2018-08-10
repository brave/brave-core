/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_H_

#include "base/files/file_path.h"
#include "components/component_updater/component_updater_service.h"

// Just used to give access to OnDemandUpdater since it's private.
// Chromium has ComponentsUI which is a friend class, so we just
// do this hack here to gain access.
class ComponentsUI {
 public:
  void OnDemandUpdate(component_updater::ComponentUpdateService* cus,
                      const std::string& component_id);
};

class BraveComponentExtension : public ComponentsUI {
 public:
  BraveComponentExtension();
  virtual ~BraveComponentExtension();
  void Register(const std::string& component_name,
                const std::string& component_id,
                const std::string& component_base64_public_key);
  static bool Unregister(const std::string& component_id);

 protected:
  virtual void OnComponentRegistered(const std::string& component_id);
  virtual void OnComponentReady(const std::string& component_id,
                                const base::FilePath& install_dir);

 private:
  std::string component_name_;
  std::string component_id_;
  std::string component_base64_public_key_;
};

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_H_
