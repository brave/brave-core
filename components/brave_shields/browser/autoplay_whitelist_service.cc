/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/autoplay_whitelist_service.h"

#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/vendor/autoplay-whitelist/autoplay_whitelist_parser.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace brave_shields {

AutoplayWhitelistService::AutoplayWhitelistService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service),
      autoplay_whitelist_client_(new AutoplayWhitelistParser()),
      weak_factory_(this) {}

AutoplayWhitelistService::~AutoplayWhitelistService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  autoplay_whitelist_client_.reset();
}

bool AutoplayWhitelistService::ShouldAllowAutoplay(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string etld_plus_one =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return autoplay_whitelist_client_->matchesHost(etld_plus_one.c_str());
}

void AutoplayWhitelistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::FilePath dat_file_path =
      install_dir.AppendASCII(AUTOPLAY_DAT_FILE_VERSION)
          .AppendASCII(AUTOPLAY_DAT_FILE);

  base::PostTaskAndReplyWithResult(
      local_data_files_service()->GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(
          &brave_component_updater::LoadDATFileData<AutoplayWhitelistParser>,
          dat_file_path),
      base::BindOnce(&AutoplayWhitelistService::OnGetDATFileData,
                     weak_factory_.GetWeakPtr()));
}

void AutoplayWhitelistService::OnGetDATFileData(GetDATFileDataResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (result.second.empty()) {
    LOG(ERROR) << "Could not obtain autoplay whitelist data";
    return;
  }
  if (!result.first.get()) {
    LOG(ERROR) << "Failed to deserialize autoplay whitelist data";
    return;
  }

  autoplay_whitelist_client_ = std::move(result.first);
  buffer_ = std::move(result.second);
}

///////////////////////////////////////////////////////////////////////////////
// The autoplay whitelist factory
std::unique_ptr<AutoplayWhitelistService> AutoplayWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<AutoplayWhitelistService>(local_data_files_service);
}

}  // namespace brave_shields
