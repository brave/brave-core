/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/debounce/browser/debounce_download_service.h"

#include <memory>
#include <utility>

#include "base/base64url.h"
#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"
#include "url/origin.h"
#include "url/url_util.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace debounce {

const char kDebounceConfigFile[] = "debounce.json";
const char kDebounceConfigFileVersion[] = "1";
// debounce.json keys
const char kInclude[] = "include";
const char kExclude[] = "exclude";
const char kAction[] = "action";
const char kParam[] = "param";

DebounceRule::DebounceRule() {
  clear();
}

DebounceRule::~DebounceRule() = default;

void DebounceRule::clear() {
  include_pattern_set_.ClearPatterns();
  exclude_pattern_set_.ClearPatterns();
  action_ = kDebounceNoAction;
  param_ = "";
}

bool DebounceRule::ParseDebounceAction(base::StringPiece value,
                                       DebounceAction* field) {
  if (value == "redirect")
    *field = kDebounceRedirectToParam;
  else if (value == "base64,redirect")
    *field = kDebounceBase64DecodeAndRedirectToParam;
  else
    *field = kDebounceNoAction;
  return true;
}

bool DebounceRule::GetURLPatternSetFromValue(
    const base::Value* value,
    extensions::URLPatternSet* result) {
  // Debouncing only affects HTTP or HTTPS URLs, regardless of how the rules are
  // written. (Also, don't write rules for other URL schemes, because they won't
  // work and you're just wasting everyone's time.)
  int valid_schemes = URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS;
  std::string error;
  const base::ListValue* list_value;
  value->GetAsList(&list_value);
  bool valid = result->Populate(*list_value, valid_schemes, false, &error);
  if (!valid)
    LOG(ERROR) << error;
  return valid;
}

void DebounceRule::RegisterJSONConverter(
    base::JSONValueConverter<DebounceRule>* converter) {
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kInclude, &DebounceRule::include_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kExclude, &DebounceRule::exclude_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomField<DebounceAction>(
      kAction, &DebounceRule::action_, &ParseDebounceAction);
  converter->RegisterStringField(kParam, &DebounceRule::param_);
}

bool DebounceRule::Apply(const GURL& original_url, GURL* final_url) {
  // If URL matches an explicitly excluded pattern, this rule does not apply.
  if (exclude_pattern_set_.MatchesURL(original_url))
    return false;
  // If URL does not match an explicitly included pattern, this rule does not
  // apply.
  if (!include_pattern_set_.MatchesURL(original_url))
    return false;

  if (action_ == kDebounceRedirectToParam ||
      action_ == kDebounceBase64DecodeAndRedirectToParam) {
    std::string unescaped_value;
    if (!net::GetValueForKeyInQuery(original_url, param_, &unescaped_value))
      return false;
    GURL new_url;
    if (action_ == kDebounceBase64DecodeAndRedirectToParam) {
      std::string base64_decoded_value;
      if (!base::Base64UrlDecode(unescaped_value,
                                 base::Base64UrlDecodePolicy::IGNORE_PADDING,
                                 &base64_decoded_value))
        return false;
      new_url = GURL(base64_decoded_value);
    } else {
      new_url = GURL(unescaped_value);
    }

    // Failsafe: ensure we got a valid URL out of the param.
    if (!new_url.is_valid() || !new_url.SchemeIsHTTPOrHTTPS())
      return false;

    // Failsafe: never redirect to the same site.
    url::Origin original_origin = url::Origin::Create(original_url);
    url::Origin debounced_origin = url::Origin::Create(new_url);
    if (original_origin.IsSameOriginWith(debounced_origin))
      return false;

    *final_url = new_url;
    return true;
  }

  // Unknown actions always return false, to allow for future updates to the
  // rules file which may be pushed to users before a new version of the code
  // that parses it.
  return false;
}

DebounceDownloadService::DebounceDownloadService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service), weak_factory_(this) {}

DebounceDownloadService::~DebounceDownloadService() {}

void DebounceDownloadService::LoadDirectlyFromResourcePath() {
  base::FilePath dat_file_path = resource_dir_.AppendASCII(kDebounceConfigFile);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     dat_file_path),
      base::BindOnce(&DebounceDownloadService::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void DebounceDownloadService::OnDATFileDataReady(std::string contents) {
  rules_.clear();
  host_cache_.clear();
  if (contents.empty()) {
    LOG(ERROR) << "Could not obtain debounce configuration";
    return;
  }
  absl::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    LOG(ERROR) << "Failed to parse debounce configuration";
    return;
  }
  base::ListValue* root_list = nullptr;
  root->GetAsList(&root_list);
  std::vector<std::string> hosts;
  for (base::Value& it : root_list->GetList()) {
    std::unique_ptr<DebounceRule> rule = std::make_unique<DebounceRule>();
    base::JSONValueConverter<DebounceRule> converter;
    converter.Convert(it, rule.get());
    for (const URLPattern& pattern : rule->include_pattern_set_) {
      if (!pattern.host().empty()) {
        hosts.push_back(pattern.host());
      }
    }
    rules_.push_back(std::move(rule));
  }
  host_cache_ = std::move(hosts);
  for (Observer& observer : observers_)
    observer.OnRulesReady(this);
}

void DebounceDownloadService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  resource_dir_ = install_dir.AppendASCII(kDebounceConfigFileVersion);
  LoadDirectlyFromResourcePath();
}

std::vector<std::unique_ptr<DebounceRule>>* DebounceDownloadService::rules() {
  return &rules_;
}

base::flat_set<std::string>* DebounceDownloadService::host_cache() {
  return &host_cache_;
}

}  // namespace debounce
