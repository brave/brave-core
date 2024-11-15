/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/files/file_path.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

#define HTTPS_UPGRADE_EXCEPTIONS_TXT_FILE "https-upgrade-exceptions-list.txt"
#define HTTPS_UPGRADE_EXCEPTIONS_TXT_FILE_VERSION "1"

namespace https_upgrade_exceptions {

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

HttpsUpgradeExceptionsService::HttpsUpgradeExceptionsService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

void HttpsUpgradeExceptionsService::LoadHTTPSUpgradeExceptions(
    const base::FilePath& install_dir) {
  base::FilePath txt_file_path =
      install_dir.AppendASCII(HTTPS_UPGRADE_EXCEPTIONS_TXT_FILE_VERSION)
          .AppendASCII(HTTPS_UPGRADE_EXCEPTIONS_TXT_FILE);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     txt_file_path),
      base::BindOnce(&HttpsUpgradeExceptionsService::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void HttpsUpgradeExceptionsService::OnDATFileDataReady(
    const std::string& contents) {
  if (contents.empty()) {
    // We don't have the file yet.
    return;
  }
  std::vector<std::string> lines = base::SplitString(
      contents, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (auto&& line : lines) {
    exceptional_domains_.insert(std::move(line));
  }
  is_ready_ = true;
  return;
}

bool HttpsUpgradeExceptionsService::CanUpgradeToHTTPS(const GURL& url) {
  if (!is_ready_) {
    // We don't have the exceptions list loaded yet. To avoid breakage,
    // don't upgrade any websites yet.
    return false;
  }
  // Allow upgrade only if the domain is not on the exceptions list.
  return !base::Contains(exceptional_domains_, url.host());
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

}  // namespace https_upgrade_exceptions
