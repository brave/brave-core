/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCALHOST_PERMISSION_LOCALHOST_PERMISSION_SERVICE_H_
#define BRAVE_COMPONENTS_LOCALHOST_PERMISSION_LOCALHOST_PERMISSION_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/strings/string_piece.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

namespace localhost_permission {

class LocalhostPermissionService
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit LocalhostPermissionService(
      brave_component_updater::LocalDataFilesService* local_data_files_service);

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  bool CanAskForLocalhostPermission(const GURL& url);
  ~LocalhostPermissionService() override;
  void OnDATFileDataReady(const std::string& contents);

 private:
  void LoadLocalhostPermissionAllowlist(const base::FilePath& install_dir);
  base::flat_set<std::string> allowed_domains_;
  bool is_ready_ = false;
  base::WeakPtrFactory<LocalhostPermissionService> weak_factory_{this};
};

}  // namespace localhost_permission

#endif  // BRAVE_COMPONENTS_LOCALHOST_PERMISSION_LOCALHOST_PERMISSION_SERVICE_H_
