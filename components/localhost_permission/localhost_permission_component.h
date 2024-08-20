/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCALHOST_PERMISSION_LOCALHOST_PERMISSION_COMPONENT_H_
#define BRAVE_COMPONENTS_LOCALHOST_PERMISSION_LOCALHOST_PERMISSION_COMPONENT_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "url/gurl.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace localhost_permission {

// This class is responsible for loading the localhost permission allowlist
// from the brave component updater, and checking if a particular GURL
// is on the allowlist.
class LocalhostPermissionComponent
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit LocalhostPermissionComponent(
      brave_component_updater::LocalDataFilesService* local_data_files_service);
  ~LocalhostPermissionComponent() override;

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  bool CanAskForLocalhostPermission(const GURL& url);
  void SetAllowedDomainsForTesting(
      const base::flat_set<std::string>& allowed_domains);

 private:
  void OnDATFileDataReady(const std::string& contents);
  void LoadLocalhostPermissionAllowlist(const base::FilePath& install_dir);

  base::flat_set<std::string> allowed_domains_;
  bool is_ready_ = false;
  base::WeakPtrFactory<LocalhostPermissionComponent> weak_factory_{this};
};

}  // namespace localhost_permission

#endif  // BRAVE_COMPONENTS_LOCALHOST_PERMISSION_LOCALHOST_PERMISSION_COMPONENT_H_
