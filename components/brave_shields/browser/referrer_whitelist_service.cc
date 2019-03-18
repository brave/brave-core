/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"

namespace brave_shields {

ReferrerWhitelistService::ReferrerWhitelistService()
    : weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

ReferrerWhitelistService::~ReferrerWhitelistService() {
}

bool ReferrerWhitelistService::IsWhitelisted(const GURL& firstPartyOrigin,
                                               const GURL& subresourceUrl) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return false; // TODO
}

void ReferrerWhitelistService::OnDATFileDataReady() {
  if (contents_.empty()) {
    LOG(ERROR) << "Could not obtain referrer whitelist data";
    return;
  }
  root_ = base::JSONReader::Read(contents_);
  if (!root_) {
    LOG(ERROR) << "Failed to parse referrer whitelist data";
    return;
  }
}

void ReferrerWhitelistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {

  base::FilePath dat_file_path = install_dir.AppendASCII(
    REFERRER_DAT_FILE_VERSION).AppendASCII(REFERRER_DAT_FILE);

  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&GetDATFileAsString, dat_file_path, &contents_),
      base::Bind(&ReferrerWhitelistService::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

scoped_refptr<base::SequencedTaskRunner>
  ReferrerWhitelistService::GetTaskRunner() {
  // We share the same task runner as ad-block code
  return g_brave_browser_process->ad_block_service()->GetTaskRunner();
}

///////////////////////////////////////////////////////////////////////////////

// The referrer whitelist factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<ReferrerWhitelistService> ReferrerWhitelistServiceFactory() {
  std::unique_ptr<ReferrerWhitelistService> service =
    std::make_unique<ReferrerWhitelistService>();
  g_brave_browser_process->local_data_files_service()->AddObserver(
    service.get());
  return service;
}

}  // namespace brave_shields
