/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_user_agent/browser/brave_user_agent_service.h"

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

#define BRAVE_USER_AGENT_EXCEPTIONS_TXT_FILE "brave-checks.txt"
#define BRAVE_USER_AGENT_EXCEPTIONS_TXT_FILE_VERSION "1"

namespace brave_user_agent {

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

BraveUserAgentService::BraveUserAgentService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

void BraveUserAgentService::LoadBraveUserAgentedDomains(
    const base::FilePath& install_dir) {
  base::FilePath txt_file_path =
      install_dir.AppendASCII(BRAVE_USER_AGENT_EXCEPTIONS_TXT_FILE_VERSION)
          .AppendASCII(BRAVE_USER_AGENT_EXCEPTIONS_TXT_FILE);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     txt_file_path),
      base::BindOnce(&BraveUserAgentService::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void BraveUserAgentService::OnDATFileDataReady(const std::string& contents) {
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

bool BraveUserAgentService::CanShowBrave(const GURL& url) {
  if (!is_ready_) {
    // We don't have the exceptions list loaded yet. To avoid breakage,
    // show Brave for any website.
    return true;
  }
  // Show Brave only if the domain is not on the exceptions list.
  return !base::Contains(exceptional_domains_, url.host());
}

// implementation of LocalDataFilesObserver
void BraveUserAgentService::OnComponentReady(const std::string& component_id,
                                             const base::FilePath& install_dir,
                                             const std::string& manifest) {
  LoadBraveUserAgentedDomains(install_dir);
}

BraveUserAgentService::~BraveUserAgentService() {
  exceptional_domains_.clear();
}

std::unique_ptr<BraveUserAgentService> BraveUserAgentServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<BraveUserAgentService>(local_data_files_service);
}

}  // namespace brave_user_agent
