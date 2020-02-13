/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_download_service.h"

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path_watcher.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/greaselion/browser/switches.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace greaselion {

const char kGreaselionConfigFile[] = "Greaselion.json";
const char kGreaselionConfigFileVersion[] = "1";
const char kRuleNameFormat[] = "greaselion-%zu";
// Greaselion.json keys
const char kPreconditions[] = "preconditions";
const char kURLs[] = "urls";
const char kScripts[] = "scripts";
// precondition keys
const char kRewards[] = "rewards-enabled";
const char kTwitterTips[] = "twitter-tips-enabled";

GreaselionPreconditionValue GreaselionRule::ParsePrecondition(
    base::DictionaryValue* root,
    const char* key) {
  base::Value* node = nullptr;
  GreaselionPreconditionValue value = kAny;
  if (root) {
    node = root->FindKeyOfType(key, base::Value::Type::BOOLEAN);
    if (node)
      value = node->GetBool() ? kMustBeTrue : kMustBeFalse;
  }
  return value;
}

GreaselionRule::GreaselionRule(const std::string& name)
    : name_(name), weak_factory_(this) {}

void GreaselionRule::Parse(base::DictionaryValue* preconditions_value,
                           base::ListValue* urls_value,
                           base::ListValue* scripts_value,
                           const base::FilePath& resource_dir) {
  preconditions_.rewards_enabled =
      ParsePrecondition(preconditions_value, kRewards);
  preconditions_.twitter_tips_enabled =
      ParsePrecondition(preconditions_value, kTwitterTips);
  for (const auto& urls_it : urls_value->GetList()) {
    std::string pattern_string = urls_it.GetString();
    URLPattern pattern;
    pattern.SetValidSchemes(URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS);
    if (pattern.Parse(pattern_string) != URLPattern::ParseResult::kSuccess) {
      LOG(ERROR) << "Malformed pattern in Greaselion configuration";
      url_patterns_.clear();
      return;
    }
    url_patterns_.push_back(pattern_string);
  }
  for (const auto& scripts_it : scripts_value->GetList()) {
    base::FilePath script_path = resource_dir.AppendASCII(
        scripts_it.GetString());
    if (script_path.ReferencesParent()) {
      LOG(ERROR) << "Malformed filename in Greaselion configuration";
    } else {
      scripts_.push_back(script_path);
    }
  }
}

GreaselionRule::~GreaselionRule() = default;

bool GreaselionRule::PreconditionFulfilled(
    GreaselionPreconditionValue precondition,
    bool value) const {
  switch (precondition) {
    case kMustBeTrue:
      return value;
    case kMustBeFalse:
      return !value;
    default:
      return true;
  }
}

bool GreaselionRule::Matches(GreaselionFeatures state) const {
  if (!PreconditionFulfilled(preconditions_.rewards_enabled,
                             state[greaselion::REWARDS]))
    return false;
  if (!PreconditionFulfilled(preconditions_.twitter_tips_enabled,
                             state[greaselion::TWITTER_TIPS]))
    return false;
  return true;
}

GreaselionDownloadService::GreaselionDownloadService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service), weak_factory_(this) {
#if !defined(OFFICIAL_BUILD)
  // Force local path
  base::FilePath forced_local_path(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
          switches::kGreaselionDataPath));
  if (!forced_local_path.empty()) {
    is_dev_mode_ = true;
    resource_dir_ = forced_local_path;
    LoadDirectlyFromResourcePath();
    dev_mode_path_watcher_ = std::make_unique<base::FilePathWatcher>();
    if (!dev_mode_path_watcher_->Watch(resource_dir_,
        true /*recursive*/,
        base::Bind(&GreaselionDownloadService::OnDevModeLocalFileChanged,
            weak_factory_.GetWeakPtr()))) {
      LOG(ERROR) << "Greaselion could not watch filesystem for changes"
          << " at path " << resource_dir_.LossyDisplayName();
    }
  }
#endif
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

void GreaselionDownloadService::OnDevModeLocalFileChanged(
    const base::FilePath& path, bool error) {
  if (error) {
    LOG(ERROR) << "Greaselion got an error watching for file changes."
        << " Stopping watching.";
    dev_mode_path_watcher_.reset();
    return;
  }
  LOG(INFO) << "Greaselion found a file change and will now reload all rules.";
  LoadDirectlyFromResourcePath();
}

void GreaselionDownloadService::LoadDirectlyFromResourcePath() {
  base::FilePath dat_file_path =
      resource_dir_.AppendASCII(kGreaselionConfigFile);
  base::PostTaskAndReplyWithResult(
      GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                    dat_file_path),
      base::BindOnce(&GreaselionDownloadService::OnDATFileDataReady,
                    weak_factory_.GetWeakPtr()));
}
GreaselionDownloadService::~GreaselionDownloadService() {}

void GreaselionDownloadService::OnDATFileDataReady(std::string contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  rules_.clear();
  if (contents.empty()) {
    LOG(ERROR) << "Could not obtain Greaselion configuration";
    return;
  }
  base::Optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    LOG(ERROR) << "Failed to parse Greaselion configuration: " << contents;
    return;
  }
  base::ListValue* root_list = nullptr;
  root->GetAsList(&root_list);
  for (base::Value& rule_it : root_list->GetList()) {
    base::DictionaryValue* rule_dict = nullptr;
    rule_it.GetAsDictionary(&rule_dict);
    base::DictionaryValue* preconditions_value = nullptr;
    rule_dict->GetDictionary(kPreconditions, &preconditions_value);
    base::ListValue* urls_value = nullptr;
    rule_dict->GetList(kURLs, &urls_value);
    base::ListValue* scripts_value = nullptr;
    rule_dict->GetList(kScripts, &scripts_value);
    std::unique_ptr<GreaselionRule> rule = std::make_unique<GreaselionRule>(
        base::StringPrintf(kRuleNameFormat, rules_.size()));
    rule->Parse(preconditions_value, urls_value, scripts_value, resource_dir_);
    rules_.push_back(std::move(rule));
  }
  for (Observer& observer : observers_)
    observer.OnRulesReady(this);
}

void GreaselionDownloadService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  if (is_dev_mode_) {
    return;
  }
  resource_dir_ = install_dir.AppendASCII(kGreaselionConfigFileVersion);
  LoadDirectlyFromResourcePath();
}

std::vector<std::unique_ptr<GreaselionRule>>*
GreaselionDownloadService::rules() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return &rules_;
}

scoped_refptr<base::SequencedTaskRunner>
GreaselionDownloadService::GetTaskRunner() {
  return local_data_files_service()->GetTaskRunner();
}

///////////////////////////////////////////////////////////////////////////////

// The factory
std::unique_ptr<GreaselionDownloadService> GreaselionDownloadServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<GreaselionDownloadService>(local_data_files_service);
}

}  // namespace greaselion
