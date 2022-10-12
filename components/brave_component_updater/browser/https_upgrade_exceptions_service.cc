/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/https_upgrade_exceptions_service.h"

#include <fstream>
#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

#define HTTPS_UPGRADE_EXCEPTIONS_DAT_FILE "https-upgrade-exceptions-list.dat"
#define HTTPS_UPGRADE_EXCEPTIONS_DAT_FILE_VERSION "1"

namespace brave_component_updater {

HttpsUpgradeExceptionsService::HttpsUpgradeExceptionsService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service), is_ready_(false) {}

void HttpsUpgradeExceptionsService::LoadHTTPSUpgradeExceptions(
    const base::FilePath& install_dir) {
  base::FilePath path =
      install_dir.AppendASCII(HTTPS_UPGRADE_EXCEPTIONS_DAT_FILE_VERSION)
          .AppendASCII(HTTPS_UPGRADE_EXCEPTIONS_DAT_FILE);
  std::ifstream file_stream(path.AsUTF8Unsafe());
  if (!file_stream.is_open()) {
    return;
  }
  exceptional_domains_.clear();
  for (std::string line; std::getline(file_stream, line);) {
    if (!line.empty()) {
      exceptional_domains_.insert(line);
    }
  }
  is_ready_ = true;
  return;
}

bool HttpsUpgradeExceptionsService::CanUpgradeToHTTPS(const GURL& url) {
  if (!is_ready_) {
    return false;
  }
  const std::string& domain = url.host();
  // Allow upgrade only if the domain is not on the exceptions list.
  return exceptional_domains_.find(domain) == exceptional_domains_.end();
}

// implementation of LocalDataFilesObserver
void HttpsUpgradeExceptionsService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  LoadHTTPSUpgradeExceptions(install_dir);
}

HttpsUpgradeExceptionsService::~HttpsUpgradeExceptionsService() {
  exceptional_domains_.clear();
}

std::unique_ptr<HttpsUpgradeExceptionsService>
HttpsUpgradeExceptionsServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<HttpsUpgradeExceptionsService>(
      local_data_files_service);
}

}  // namespace brave_component_updater
