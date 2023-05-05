/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/localhost_permission_allowlist/browser/localhost_permission_allowlist_service.h"

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

#define LOCALHOST_PERMISSION_ALLOWLIST_TXT_FILE \
  "localhost-permission-allow-list.txt"
#define LOCALHOST_PERMISSION_ALLOWLIST_TXT_FILE_VERSION "1"

namespace localhost_permission_allowlist {

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

LocalhostPermissionAllowlistService::LocalhostPermissionAllowlistService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

void LocalhostPermissionAllowlistService::LoadLocalhostPermissionAllowlist(
    const base::FilePath& install_dir) {
  base::FilePath txt_file_path =
      install_dir.AppendASCII(LOCALHOST_PERMISSION_ALLOWLIST_TXT_FILE_VERSION)
          .AppendASCII(LOCALHOST_PERMISSION_ALLOWLIST_TXT_FILE);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     txt_file_path),
      base::BindOnce(&LocalhostPermissionAllowlistService::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void LocalhostPermissionAllowlistService::OnDATFileDataReady(
    const std::string& contents) {
  if (contents.empty()) {
    // We don't have the file yet.
    return;
  }
  std::vector<std::string> lines = base::SplitString(
      contents, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const auto& line : lines) {
    // Ignore lines starting with !.
    // Used for comments.
    if (line[0] == '!') {
      continue;
    }
    allowed_hosts_.insert(std::move(line));
  }
  is_ready_ = true;
  return;
}

bool LocalhostPermissionAllowlistService::CanAskForLocalhostPermission(
    const GURL& url) {
  if (!is_ready_) {
    // We don't have the allowlist loaded yet;
    // by default do the more privacy-friendly thing.
    return false;
  }
  // Allow asking for permission only if the host is on the list.
  return base::Contains(allowed_hosts_, url.host());
}

// implementation of LocalDataFilesObserver
void LocalhostPermissionAllowlistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  LoadLocalhostPermissionAllowlist(install_dir);
}

LocalhostPermissionAllowlistService::~LocalhostPermissionAllowlistService() {
  allowed_hosts_.clear();
}

std::unique_ptr<LocalhostPermissionAllowlistService>
LocalhostPermissionAllowlistServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<LocalhostPermissionAllowlistService>(
      local_data_files_service);
}

}  // namespace localhost_permission_allowlist
