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
#include "brave/components/brave_component_updater/browser/dat_file_util.h"

namespace brave_shields {

ReferrerWhitelistService::ReferrerWhitelistService()
    : weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

ReferrerWhitelistService::~ReferrerWhitelistService() {
}

ReferrerWhitelistService::ReferrerWhitelist::ReferrerWhitelist() = default;
ReferrerWhitelistService::ReferrerWhitelist::ReferrerWhitelist(
  const ReferrerWhitelist& other) = default;
ReferrerWhitelistService::ReferrerWhitelist::~ReferrerWhitelist() = default;

bool ReferrerWhitelistService::IsWhitelisted(
    const GURL& firstPartyOrigin, const GURL& subresourceUrl) const {
  for (auto rw : referrer_whitelist_) {
    if (rw.first_party_pattern.MatchesURL(firstPartyOrigin)) {
      for (auto subresource_pattern : rw.subresource_pattern_list) {
        if (subresource_pattern.MatchesURL(subresourceUrl)) {
          return true;
        }
      }
    }
  }
  return false;
}

void ReferrerWhitelistService::OnDATFileDataReady() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  referrer_whitelist_.clear();
  if (file_contents_.empty()) {
    LOG(ERROR) << "Could not obtain referrer whitelist data";
    return;
  }
  base::Optional<base::Value> root = base::JSONReader::Read(file_contents_);
  file_contents_.clear();
  if (!root) {
    LOG(ERROR) << "Failed to parse referrer whitelist data";
    return;
  }
  base::DictionaryValue* root_dict = nullptr;
  root->GetAsDictionary(&root_dict);
  base::ListValue* whitelist = nullptr;
  root_dict->GetList("whitelist", &whitelist);
  for (base::Value& origins : whitelist->GetList()) {
    base::DictionaryValue* origins_dict = nullptr;
    origins.GetAsDictionary(&origins_dict);
    for (const auto& it : origins_dict->DictItems()) {
      ReferrerWhitelist rw;
      rw.first_party_pattern = URLPattern(
        URLPattern::SCHEME_HTTP|URLPattern::SCHEME_HTTPS, it.first);
      URLPatternList subresource_pattern_list;
      for (base::Value& subresource_value : it.second.GetList()) {
        rw.subresource_pattern_list.push_back(URLPattern(
          URLPattern::SCHEME_HTTP|URLPattern::SCHEME_HTTPS,
          subresource_value.GetString()));
      }
      referrer_whitelist_.push_back(rw);
    }
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
      base::Bind(&brave_component_updater::GetDATFileAsString,
                 dat_file_path,
                 &file_contents_),
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
