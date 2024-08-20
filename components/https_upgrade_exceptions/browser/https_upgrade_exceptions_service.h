/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_
#define BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "url/gurl.h"

namespace https_upgrade_exceptions {

class HttpsUpgradeExceptionsService
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit HttpsUpgradeExceptionsService(
      brave_component_updater::LocalDataFilesService* local_data_files_service);

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  bool CanUpgradeToHTTPS(const GURL& url);
  ~HttpsUpgradeExceptionsService() override;
  void SetIsReadyForTesting() { is_ready_ = true; }
  void OnDATFileDataReady(const std::string& contents);

 private:
  void LoadHTTPSUpgradeExceptions(const base::FilePath& install_dir);
  std::set<std::string> exceptional_domains_;
  bool is_ready_ = false;
  base::WeakPtrFactory<HttpsUpgradeExceptionsService> weak_factory_{this};
};

// Creates the HttpsUpgradeExceptionsService
std::unique_ptr<HttpsUpgradeExceptionsService>
HttpsUpgradeExceptionsServiceFactory(
    brave_component_updater::LocalDataFilesService* local_data_files_service);

}  // namespace https_upgrade_exceptions

#endif  // BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_
