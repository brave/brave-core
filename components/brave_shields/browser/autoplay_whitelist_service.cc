/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/autoplay_whitelist_service.h"

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
#include "brave/vendor/autoplay-whitelist/autoplay_whitelist_parser.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace brave_shields {

AutoplayWhitelistService::AutoplayWhitelistService()
    : autoplay_whitelist_client_(new AutoplayWhitelistParser()),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AutoplayWhitelistService::~AutoplayWhitelistService() {
  autoplay_whitelist_client_.reset();
}

bool AutoplayWhitelistService::ShouldAllowAutoplay(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string etld_plus_one =
    net::registry_controlled_domains::GetDomainAndRegistry(
      url,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return autoplay_whitelist_client_->matchesHost(etld_plus_one.c_str());
}

void AutoplayWhitelistService::OnDATFileDataReady() {
  if (buffer_.empty()) {
    LOG(ERROR) << "Could not obtain autoplay whitelist data";
    return;
  }
  autoplay_whitelist_client_.reset(new AutoplayWhitelistParser());
  if (!autoplay_whitelist_client_->deserialize(
        reinterpret_cast<char*>(&buffer_.front()))) {
    autoplay_whitelist_client_.reset();
    LOG(ERROR) << "Failed to deserialize autoplay whitelist data";
    return;
  }
}

void AutoplayWhitelistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {

  base::FilePath dat_file_path = install_dir.AppendASCII(
    AUTOPLAY_DAT_FILE_VERSION).AppendASCII(AUTOPLAY_DAT_FILE);

  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&brave_component_updater::GetDATFileData,
                 dat_file_path,
                 &buffer_),
      base::Bind(&AutoplayWhitelistService::OnDATFileDataReady,
                 weak_factory_.GetWeakPtr()));
}

scoped_refptr<base::SequencedTaskRunner>
  AutoplayWhitelistService::GetTaskRunner() {
  // We share the same task runner as ad-block code
  return g_brave_browser_process->ad_block_service()->GetTaskRunner();
}

///////////////////////////////////////////////////////////////////////////////

// The autoplay whitelist factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<AutoplayWhitelistService> AutoplayWhitelistServiceFactory() {
  std::unique_ptr<AutoplayWhitelistService> service =
    std::make_unique<AutoplayWhitelistService>();
  g_brave_browser_process->local_data_files_service()->AddObserver(
    service.get());
  return service;
}

}  // namespace brave_shields
