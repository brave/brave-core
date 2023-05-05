/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCALHOST_PERMISSION_ALLOWLIST_BROWSER_LOCALHOST_PERMISSION_ALLOWLIST_SERVICE_H_
#define BRAVE_COMPONENTS_LOCALHOST_PERMISSION_ALLOWLIST_BROWSER_LOCALHOST_PERMISSION_ALLOWLIST_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "base/strings/string_piece.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

namespace localhost_permission_allowlist {

class LocalhostPermissionAllowlistService
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit LocalhostPermissionAllowlistService(
      brave_component_updater::LocalDataFilesService* local_data_files_service);

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  bool CanAskForLocalhostPermission(const GURL& url);
  ~LocalhostPermissionAllowlistService() override;
  void SetTestHosts(std::set<std::string> allowed_hosts) {
    allowed_hosts_ = allowed_hosts;
    is_ready_ = true;
  }
  void OnDATFileDataReady(const std::string& contents);

 private:
  void LoadLocalhostPermissionAllowlist(const base::FilePath& install_dir);
  std::set<std::string> allowed_hosts_;
  bool is_ready_ = false;
  base::WeakPtrFactory<LocalhostPermissionAllowlistService> weak_factory_{this};
};

// Creates the LocalhostPermissionAllowlistService
std::unique_ptr<LocalhostPermissionAllowlistService>
LocalhostPermissionAllowlistServiceFactory(
    brave_component_updater::LocalDataFilesService* local_data_files_service);

}  // namespace localhost_permission_allowlist

#endif  // BRAVE_COMPONENTS_LOCALHOST_PERMISSION_ALLOWLIST_BROWSER_LOCALHOST_PERMISSION_ALLOWLIST_SERVICE_H_
