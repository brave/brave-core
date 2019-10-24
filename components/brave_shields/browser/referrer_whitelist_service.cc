/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;
using content::BrowserThread;

namespace brave_shields {

ReferrerWhitelistService::ReferrerWhitelistService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service),
      weak_factory_(this),
      weak_factory_io_thread_(this) {
}

ReferrerWhitelistService::~ReferrerWhitelistService() {
}

ReferrerWhitelistService::ReferrerWhitelist::ReferrerWhitelist() = default;
ReferrerWhitelistService::ReferrerWhitelist::ReferrerWhitelist(
  const ReferrerWhitelist& other) = default;
ReferrerWhitelistService::ReferrerWhitelist::~ReferrerWhitelist() = default;

bool ReferrerWhitelistService::IsWhitelisted(
    const GURL& first_party_origin, const GURL& subresource_url) const {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    return IsWhitelisted(
        referrer_whitelist_io_thread_, first_party_origin, subresource_url);
  } else {
    return IsWhitelisted(
        referrer_whitelist_, first_party_origin, subresource_url);
  }
}

bool ReferrerWhitelistService::IsWhitelisted(
    const std::vector<ReferrerWhitelist>& whitelist,
    const GURL& first_party_origin,
    const GURL& subresource_url) const {
  for (auto rw : whitelist) {
    if (rw.first_party_pattern.MatchesURL(first_party_origin)) {
      for (auto subresource_pattern : rw.subresource_pattern_list) {
        if (subresource_pattern.MatchesURL(subresource_url)) {
          return true;
        }
      }
    }
  }
  return false;
}

void ReferrerWhitelistService::OnDATFileDataReady(std::string contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  referrer_whitelist_.clear();
  if (contents.empty()) {
    LOG(ERROR) << "Could not obtain referrer whitelist data";
    return;
  }
  base::Optional<base::Value> root = base::JSONReader::Read(contents);
  contents.clear();
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

  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&ReferrerWhitelistService::OnDATFileDataReadyOnIOThread,
                     weak_factory_io_thread_.GetWeakPtr(),
                     referrer_whitelist_));
}

void ReferrerWhitelistService::OnDATFileDataReadyOnIOThread(
    std::vector<ReferrerWhitelist> whitelist) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  referrer_whitelist_io_thread_ = std::move(whitelist);
}

void ReferrerWhitelistService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::FilePath dat_file_path = install_dir
      .AppendASCII(REFERRER_DAT_FILE_VERSION)
      .AppendASCII(REFERRER_DAT_FILE);

  base::PostTaskAndReplyWithResult(
      local_data_files_service()->GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     dat_file_path),
      base::BindOnce(&ReferrerWhitelistService::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<ReferrerWhitelistService> ReferrerWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<ReferrerWhitelistService>(local_data_files_service);
}

}  // namespace brave_shields
