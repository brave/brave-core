/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/extension_whitelist_service.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/task/task_runner_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/vendor/extension-whitelist/extension_whitelist_parser.h"

namespace brave_component_updater {

ExtensionWhitelistService::ExtensionWhitelistService(
    LocalDataFilesService* local_data_files_service,
    const std::vector<std::string>& whitelist)
    : LocalDataFilesObserver(local_data_files_service),
      extension_whitelist_client_(new ExtensionWhitelistParser()),
      whitelist_(whitelist),
      weak_factory_(this) {
}

ExtensionWhitelistService::~ExtensionWhitelistService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  extension_whitelist_client_.reset();
}

bool ExtensionWhitelistService::IsWhitelisted(
    const std::string& extension_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return extension_whitelist_client_->isWhitelisted(extension_id.c_str());
}

bool ExtensionWhitelistService::IsBlacklisted(
    const std::string& extension_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return extension_whitelist_client_->isBlacklisted(extension_id.c_str());
}

bool ExtensionWhitelistService::IsVetted(const std::string& id) const {
  if (std::find(whitelist_.begin(), whitelist_.end(), id) !=
      whitelist_.end())
    return true;

  return IsWhitelisted(id);
}

void ExtensionWhitelistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::FilePath dat_file_path =
      install_dir.AppendASCII(EXTENSION_DAT_FILE_VERSION)
          .AppendASCII(EXTENSION_DAT_FILE);

  base::PostTaskAndReplyWithResult(
      local_data_files_service()->GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(
          &brave_component_updater::LoadDATFileData<ExtensionWhitelistParser>,
          dat_file_path),
      base::BindOnce(&ExtensionWhitelistService::OnGetDATFileData,
                     weak_factory_.GetWeakPtr()));
}

void ExtensionWhitelistService::OnGetDATFileData(GetDATFileDataResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (result.second.empty()) {
    LOG(ERROR) << "Could not obtain extension whitelist data";
    return;
  }
  if (!result.first.get()) {
    LOG(ERROR) << "Failed to deserialize extension whitelist data";
    return;
  }

  extension_whitelist_client_ = std::move(result.first);
  buffer_ = std::move(result.second);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<ExtensionWhitelistService> ExtensionWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service,
    const std::vector<std::string>& whitelist) {
  return std::make_unique<ExtensionWhitelistService>(local_data_files_service,
                                                     whitelist);
}

}  // namespace brave_component_updater
