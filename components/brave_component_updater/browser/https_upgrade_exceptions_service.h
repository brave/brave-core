/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

namespace brave_component_updater {

class HttpsUpgradeExceptionsService : public LocalDataFilesObserver {
 public:
  explicit HttpsUpgradeExceptionsService(
      LocalDataFilesService* local_data_files_service);

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  bool CanUpgradeToHTTPS(const GURL& url);
  ~HttpsUpgradeExceptionsService() override;
  void SetIsReadyForTesting() { is_ready_ = true; }

 private:
  void LoadHTTPSUpgradeExceptions(const base::FilePath& install_dir);
  std::set<std::string> exceptional_domains_;
  bool is_ready_;
};

// Creates the HttpsUpgradeExceptionsService
std::unique_ptr<HttpsUpgradeExceptionsService>
HttpsUpgradeExceptionsServiceFactory(
    LocalDataFilesService* local_data_files_service);

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_
