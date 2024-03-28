/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_exceptions/webcompat_exceptions_service.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

#include "base/logging.h"

#define WEBCOMPAT_EXCEPTIONS_JSON_FILE "webcompat-exceptions.json"
#define WEBCOMPAT_EXCEPTIONS_JSON_FILE_VERSION "1"

namespace webcompat_exceptions {

namespace {
using enum WebcompatFeature;

// WebcompatExceptionService keys
const char kInclude[] = "include";
const char kExceptions[] = "exceptions";

constexpr auto kWebcompatNamesToType =
    base::MakeFixedFlatMap<base::StringPiece, WebcompatFeature>({
        {"audio", kAudio},
        {"canvas", kCanvas},
        {"device-memory", kDeviceMemory},
        {"eventsource-pool", kEventSourcePool},
        {"font", kFont},
        {"hardware-concurrency", kHardwareConcurrency},
        {"keyboard", kKeyboard},
        {"language", kLanguage},
        {"media-devices", kMediaDevices},
        {"plugins", kPlugins},
        {"screen", kScreen},
        {"speech-synthesis", kSpeechSynthesis},
        {"usb-device-serial-number", kUsbDeviceSerialNumber},
        {"user-agent", kUserAgent},
        {"webgl", kWebGL},
        {"webgl2", kWebGL2},
        {"websockets-pool", kWebSocketsPool},
    });

}  // namespace

WebcompatRule::WebcompatRule() = default;
WebcompatRule::WebcompatRule(const WebcompatRule& other)
    : url_pattern_set(other.url_pattern_set.Clone()),
      feature_set(other.feature_set) {}
WebcompatRule::~WebcompatRule() = default;

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

WebcompatExceptionsService::WebcompatExceptionsService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

void WebcompatExceptionsService::LoadWebcompatExceptions(
    const base::FilePath& install_dir) {
  base::FilePath txt_file_path =
      install_dir.AppendASCII(WEBCOMPAT_EXCEPTIONS_JSON_FILE_VERSION)
          .AppendASCII(WEBCOMPAT_EXCEPTIONS_JSON_FILE);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     txt_file_path),
      base::BindOnce(&WebcompatExceptionsService::OnJsonFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void WebcompatExceptionsService::AddRule(
    const base::Value::List& include_strings,
    const base::Value::Dict& rule_dict) {
  WebcompatRule rule;
  webcompat_rules_.push_back(rule);
  std::string error;
  bool valid = rule.url_pattern_set.Populate(
      include_strings, URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      false, &error);
  if (!valid) {
    VLOG(1) << error;
  }
  const base::Value* exceptions = rule_dict.Find(kExceptions);
  if (exceptions->is_list()) {
    std::vector<WebcompatFeature> webcompat_types;
    for (const base::Value& exception : exceptions->GetList()) {
      const auto it = kWebcompatNamesToType.find(exception.GetString());
      if (it != kWebcompatNamesToType.end()) {
        rule.feature_set.push_back(it->second);
      }
    }
  } else {
    DLOG(ERROR) << "Malformed exceptions list in "
                << WEBCOMPAT_EXCEPTIONS_JSON_FILE;
  }
  webcompat_rules_.push_back(rule);
}

void WebcompatExceptionsService::OnJsonFileDataReady(
    const std::string& contents) {
  if (contents.empty()) {
    // We don't have the file yet.
    return;
  }
  const auto json_root = base::JSONReader::Read(contents);
  if (json_root == absl::nullopt) {
    DLOG(ERROR) << "Failed to parse " << WEBCOMPAT_EXCEPTIONS_JSON_FILE;
    return;
  }
  if (!json_root->is_list()) {
    DLOG(ERROR) << "Didn't find expected list in "
                << WEBCOMPAT_EXCEPTIONS_JSON_FILE;
    return;
  }
  for (const auto& rule : json_root->GetList()) {
    if (!rule.is_dict()) {
      // Something is wrong with the rule definition; skip it.
      DLOG(ERROR) << "Found a malformed rule in "
                  << WEBCOMPAT_EXCEPTIONS_JSON_FILE;
      continue;
    }
    const auto& rule_dict = rule.GetDict();
    const auto* include = rule_dict.Find(kInclude);
    if (include->is_list()) {
      AddRule(include->GetList(), rule_dict);
    } else if (include->is_string()) {
      DLOG(ERROR) << "Not implemented yet";
    } else {
      DLOG(ERROR) << "Malformed include attribute in "
                  << WEBCOMPAT_EXCEPTIONS_JSON_FILE;
    }
  }
  is_ready_ = true;
}

const WebcompatFeatureSet WebcompatExceptionsService::GetFeatureExceptions(
    const GURL& url) {
  static const WebcompatFeatureSet empty;
  if (!is_ready_) {
    // We don't have the exceptions list loaded yet; return no exceptions.
    return empty;
  }
  // Find out if any rules apply.
  for (const auto& rule : webcompat_rules_) {
    if (rule.url_pattern_set.MatchesURL(url)) {
      return rule.feature_set;
    }
  }
  // No exceptions found
  return empty;
}

bool WebcompatExceptionsService::IsFeatureDisabled(const GURL& url,
                                                   WebcompatFeature feature) {
  WebcompatFeatureSet feature_set = GetFeatureExceptions(url);
  return std::find(feature_set.begin(), feature_set.end(), feature) !=
         feature_set.end();
}

// implementation of LocalDataFilesObserver
void WebcompatExceptionsService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  LoadWebcompatExceptions(install_dir);
}

WebcompatExceptionsService::~WebcompatExceptionsService() {
  exceptional_domains_.clear();
}

std::unique_ptr<WebcompatExceptionsService> WebcompatExceptionsServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  return std::make_unique<WebcompatExceptionsService>(local_data_files_service);
}

}  // namespace webcompat_exceptions
