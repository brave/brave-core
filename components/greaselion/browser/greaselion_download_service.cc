/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_download_service.h"

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path_watcher.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/greaselion/browser/switches.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

#if !defined(OFFICIAL_BUILD)
namespace {
bool StartFilePathWatcher(base::FilePathWatcher* watcher,
                          const base::FilePath& file_path,
                          base::FilePathWatcher::Type type,
                          const base::FilePathWatcher::Callback& callback) {
  if (!watcher) {
    return false;
  }

  return watcher->Watch(file_path, type, callback);
}
}  // namespace
#endif

namespace greaselion {

const char kGreaselionConfigFile[] = "Greaselion.json";
const char kGreaselionConfigFileVersion[] = "1";
const char kRuleNameFormat[] = "greaselion-%zu";
// Greaselion.json keys
const char kPreconditions[] = "preconditions";
const char kURLs[] = "urls";
const char kScripts[] = "scripts";
const char kRunAt[] = "run_at";
const char kMessages[] = "messages";
// Note(petemill): "brave" instead of "browser" version in order
// to preserve some sense of cross-browser targetting of the scripts.
const char kMinimumBraveVersion[] = "minimum_brave_version";
// precondition keys
const char kRewards[] = "rewards-enabled";
const char kTwitterTips[] = "twitter-tips-enabled";
const char kRedditTips[] = "reddit-tips-enabled";
const char kGithubTips[] = "github-tips-enabled";
const char kAutoContribution[] = "auto-contribution-enabled";
const char kAds[] = "ads-enabled";
const char kSupportsMinimumBraveVersion[] =
    "supports-minimum-brave-version";

GreaselionRule::GreaselionRule(const std::string& name) : name_(name) {}

GreaselionRule::GreaselionRule(const GreaselionRule& name) = default;

GreaselionRule& GreaselionRule::operator=(const GreaselionRule& name) = default;

GreaselionPreconditionValue GreaselionRule::ParsePrecondition(
    const base::Value& value) {
  GreaselionPreconditionValue condition = kAny;
  if (value.is_bool()) {
    condition = value.GetBool() ? kMustBeTrue : kMustBeFalse;
  }
  return condition;
}

void GreaselionRule::Parse(base::DictionaryValue* preconditions_value,
                           base::ListValue* urls_value,
                           base::ListValue* scripts_value,
                           const std::string& run_at_value,
                           const std::string& minimum_brave_version_value,
                           const base::FilePath& messages_value,
                           const base::FilePath& resource_dir) {
  if (preconditions_value) {
    for (const auto kv : preconditions_value->DictItems()) {
      GreaselionPreconditionValue condition = ParsePrecondition(kv.second);
      if (kv.first == kRewards) {
        preconditions_.rewards_enabled = condition;
      } else if (kv.first == kTwitterTips) {
        preconditions_.twitter_tips_enabled = condition;
      } else if (kv.first == kRedditTips) {
        preconditions_.reddit_tips_enabled = condition;
      } else if (kv.first == kGithubTips) {
        preconditions_.github_tips_enabled = condition;
      } else if (kv.first == kAutoContribution) {
        preconditions_.auto_contribution_enabled = condition;
      } else if (kv.first == kAds) {
        preconditions_.ads_enabled = condition;
      } else if (kv.first == kSupportsMinimumBraveVersion) {
        preconditions_.supports_minimum_brave_version = condition;
      } else {
        LOG(INFO) << "Greaselion encountered an unknown precondition: "
            << kv.first;
        has_unknown_preconditions_ = true;
      }
    }
  }
  for (const auto& urls_it : urls_value->GetListDeprecated()) {
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
  for (const auto& scripts_it : scripts_value->GetListDeprecated()) {
    base::FilePath script_path = resource_dir.AppendASCII(
        scripts_it.GetString());
    if (script_path.ReferencesParent()) {
      LOG(ERROR) << "Malformed filename in Greaselion configuration";
    } else {
      scripts_.push_back(script_path);
    }
  }
  run_at_ = run_at_value;
  minimum_brave_version_ = minimum_brave_version_value;
  if (!messages_value.empty()) {
    messages_ = resource_dir.Append(messages_value);
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

bool GreaselionRule::Matches(
    GreaselionFeatures state, const base::Version& browser_version) const {
  // Validate against preconditions.
  if (!PreconditionFulfilled(preconditions_.rewards_enabled,
                             state[greaselion::REWARDS]))
    return false;
  if (!PreconditionFulfilled(preconditions_.twitter_tips_enabled,
                             state[greaselion::TWITTER_TIPS]))
    return false;
  if (!PreconditionFulfilled(preconditions_.reddit_tips_enabled,
                             state[greaselion::REDDIT_TIPS]))
    return false;
  if (!PreconditionFulfilled(preconditions_.github_tips_enabled,
                             state[greaselion::GITHUB_TIPS]))
    return false;
  if (!PreconditionFulfilled(preconditions_.auto_contribution_enabled,
                             state[greaselion::AUTO_CONTRIBUTION]))
    return false;
  if (!PreconditionFulfilled(preconditions_.supports_minimum_brave_version,
                          state[greaselion::SUPPORTS_MINIMUM_BRAVE_VERSION]))
    return false;
  if (!PreconditionFulfilled(preconditions_.ads_enabled,
                             state[greaselion::ADS]))
    return false;
  // Validate against browser version.
  if (base::Version::IsValidWildcardString(minimum_brave_version_)) {
    bool rule_version_is_higher_than_browser =
        (browser_version.CompareToWildcardString(minimum_brave_version_) < 0);
    if (rule_version_is_higher_than_browser) {
      return false;
    }
  }
  // Rule matches current state.
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
    dev_mode_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN, base::MayBlock()});
    dev_mode_path_watcher_ = std::make_unique<base::FilePathWatcher>();

    using DevModeLocalFileChangedCallback = base::RepeatingCallback<void(bool)>;
    base::FilePathWatcher::Callback file_path_watcher_callback =
        base::BindRepeating(
            [](scoped_refptr<base::SequencedTaskRunner> main_sequence,
               const DevModeLocalFileChangedCallback& callback,
               const base::FilePath&, bool error) {
              main_sequence->PostTask(FROM_HERE,
                                      base::BindOnce(callback, error));
            },
            base::SequencedTaskRunnerHandle::Get(),
            base::BindRepeating(
                &GreaselionDownloadService::OnDevModeLocalFileChanged,
                weak_factory_.GetWeakPtr()));

    // Start the watcher on a background sequence, reporting all events back to
    // this sequence. base::Unretained is safe because the watcher instance
    // lives on the target sequence and will be destroyed there in a subsequent
    // task.
    dev_mode_task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&StartFilePathWatcher,
                       base::Unretained(dev_mode_path_watcher_.get()),
                       resource_dir_, base::FilePathWatcher::Type::kRecursive,
                       file_path_watcher_callback),
        base::BindOnce(
            [](DevModeLocalFileChangedCallback callback, bool start_result) {
              if (!start_result) {
                callback.Run(/*error=*/true);
              }
            },
            base::BindRepeating(
                &GreaselionDownloadService::OnDevModeLocalFileChanged,
                weak_factory_.GetWeakPtr())));
  }
#endif
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

GreaselionDownloadService::~GreaselionDownloadService() {
  if (dev_mode_path_watcher_) {
    dev_mode_task_runner_->DeleteSoon(FROM_HERE,
                                      std::move(dev_mode_path_watcher_));
  }
}

void GreaselionDownloadService::OnDevModeLocalFileChanged(bool error) {
  if (error) {
    LOG(ERROR) << "Greaselion encountered an error watching for file changes";
    return;
  }
  LOG(INFO) << "Greaselion found a file change and will now reload all rules";
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

void GreaselionDownloadService::OnDATFileDataReady(std::string contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  rules_.clear();
  if (contents.empty()) {
    LOG(ERROR) << "Could not obtain Greaselion configuration";
    return;
  }
  absl::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    LOG(ERROR) << "Failed to parse Greaselion configuration";
    return;
  }
  base::ListValue* root_list = nullptr;
  root->GetAsList(&root_list);
  for (base::Value& rule_it : root_list->GetListDeprecated()) {
    base::DictionaryValue* rule_dict = nullptr;
    rule_it.GetAsDictionary(&rule_dict);
    base::DictionaryValue* preconditions_value = nullptr;
    rule_dict->GetDictionary(kPreconditions, &preconditions_value);
    base::ListValue* urls_value = nullptr;
    rule_dict->GetList(kURLs, &urls_value);
    base::ListValue* scripts_value = nullptr;
    rule_dict->GetList(kScripts, &scripts_value);
    const std::string* run_at_ptr = rule_it.FindStringPath(kRunAt);
    const std::string run_at_value = run_at_ptr ? *run_at_ptr : "";
    const std::string* minimum_brave_version_ptr = rule_it.FindStringPath(
        kMinimumBraveVersion);
    const std::string minimum_brave_version_value =
        minimum_brave_version_ptr ? *minimum_brave_version_ptr : "";
    const std::string* messages = rule_it.FindStringPath(kMessages);
    base::FilePath messages_path;
    if (messages) {
      messages_path = base::FilePath::FromUTF8Unsafe(messages->c_str());
    }

    std::unique_ptr<GreaselionRule> rule = std::make_unique<GreaselionRule>(
        base::StringPrintf(kRuleNameFormat, rules_.size()));
    rule->Parse(preconditions_value, urls_value, scripts_value, run_at_value,
        minimum_brave_version_value, messages_path, resource_dir_);
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
