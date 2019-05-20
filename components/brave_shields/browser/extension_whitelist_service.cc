/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/extension_whitelist_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/local_data_files_service.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/vendor/extension-whitelist/extension_whitelist_parser.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace brave_shields {

ExtensionWhitelistService::ExtensionWhitelistService()
    : extension_whitelist_client_(new ExtensionWhitelistParser()),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

ExtensionWhitelistService::~ExtensionWhitelistService() {
  extension_whitelist_client_.reset();
}

bool ExtensionWhitelistService::IsWhitelisted(const std::string& extension_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return extension_whitelist_client_->isWhitelisted(extension_id.c_str());
}

bool ExtensionWhitelistService::IsBlacklisted(const std::string& extension_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return extension_whitelist_client_->isBlacklisted(extension_id.c_str());
}

void ExtensionWhitelistService::OnDATFileDataReady() {
  if (buffer_.empty()) {
    LOG(ERROR) << "Could not obtain extension whitelist data";
    return;
  }
  extension_whitelist_client_.reset(new ExtensionWhitelistParser());
  if (!extension_whitelist_client_->deserialize(
        reinterpret_cast<char*>(&buffer_.front()))) {
    extension_whitelist_client_.reset();
    LOG(ERROR) << "Failed to deserialize extension whitelist data";
    return;
  }
}

void ExtensionWhitelistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {

  base::FilePath dat_file_path = install_dir.AppendASCII(
    EXTENSION_DAT_FILE_VERSION).AppendASCII(EXTENSION_DAT_FILE);

  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&brave_component_updater::GetDATFileData,
                 dat_file_path,
                 &buffer_),
      base::Bind(&ExtensionWhitelistService::OnDATFileDataReady,
                 weak_factory_.GetWeakPtr()));
}

scoped_refptr<base::SequencedTaskRunner>
  ExtensionWhitelistService::GetTaskRunner() {
  // We share the same task runner as ad-block code
  return g_brave_browser_process->ad_block_service()->GetTaskRunner();
}

///////////////////////////////////////////////////////////////////////////////

// The extension whitelist factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<ExtensionWhitelistService> ExtensionWhitelistServiceFactory() {
  std::unique_ptr<ExtensionWhitelistService> service =
    std::make_unique<ExtensionWhitelistService>();
  g_brave_browser_process->local_data_files_service()->AddObserver(
    service.get());
  return service;
}

}  // namespace brave_shields
