/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/localhost_permission/localhost_permission_component.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/files/file_path.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

#define LOCALHOST_PERMISSION_TXT_FILE "localhost-permission-allow-list.txt"
#define LOCALHOST_PERMISSION_TXT_FILE_VERSION "1"

namespace {
std::string GetDomain(const GURL& url) {
  return net::registry_controlled_domains::GetDomainAndRegistry(
      url.host(), net::registry_controlled_domains::PrivateRegistryFilter::
                      EXCLUDE_PRIVATE_REGISTRIES);
}
}  // namespace

namespace localhost_permission {

LocalhostPermissionComponent::LocalhostPermissionComponent(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

LocalhostPermissionComponent::~LocalhostPermissionComponent() = default;

void LocalhostPermissionComponent::LoadLocalhostPermissionAllowlist(
    const base::FilePath& install_dir) {
  base::FilePath txt_file_path =
      install_dir.AppendASCII(LOCALHOST_PERMISSION_TXT_FILE_VERSION)
          .AppendASCII(LOCALHOST_PERMISSION_TXT_FILE);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     txt_file_path),
      base::BindOnce(&LocalhostPermissionComponent::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void LocalhostPermissionComponent::SetAllowedDomainsForTesting(
    const base::flat_set<std::string>& allowed_domains) {
  allowed_domains_ = allowed_domains;
}

void LocalhostPermissionComponent::OnDATFileDataReady(
    const std::string& contents) {
  if (contents.empty()) {
    // We don't have the file yet.
    return;
  }
  std::vector<std::string> lines = base::SplitString(
      contents, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const auto& line : lines) {
    // Ignore lines starting with #.
    // Used for comments.
    if (line[0] == '#') {
      continue;
    }
    // Construct a GURL from the line, and store the eTLD+1.
    const auto url = GURL("https://" + line);
    if (url.is_valid()) {
      allowed_domains_.insert(GetDomain(url));
    }
  }
  is_ready_ = true;
  return;
}

bool LocalhostPermissionComponent::CanAskForLocalhostPermission(
    const GURL& url) {
  // Allow asking for permission only if the URL
  // matches an eTLD+1 on the allowlist.
  return base::Contains(allowed_domains_, GetDomain(url));
}

// implementation of LocalDataFilesObserver
void LocalhostPermissionComponent::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  LoadLocalhostPermissionAllowlist(install_dir);
}

}  // namespace localhost_permission
