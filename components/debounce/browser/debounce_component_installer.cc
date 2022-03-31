/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/debounce/browser/debounce_component_installer.h"

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace debounce {

const char kDebounceConfigFile[] = "debounce.json";
const char kDebounceConfigFileVersion[] = "1";

DebounceComponentInstaller::DebounceComponentInstaller(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

DebounceComponentInstaller::~DebounceComponentInstaller() {}

void DebounceComponentInstaller::LoadDirectlyFromResourcePath() {
  base::FilePath dat_file_path = resource_dir_.AppendASCII(kDebounceConfigFile);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     dat_file_path),
      base::BindOnce(&DebounceComponentInstaller::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void DebounceComponentInstaller::OnDATFileDataReady(
    const std::string& contents) {
  if (contents.empty()) {
    VLOG(1) << "Could not obtain debounce configuration";
    return;
  }
  absl::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    VLOG(1) << "Failed to parse debounce configuration";
    return;
  }
  rules_.clear();
  host_cache_.clear();
  std::vector<std::string> hosts;
  base::JSONValueConverter<DebounceRule> converter;
  for (base::Value& it : root->GetList()) {
    std::unique_ptr<DebounceRule> rule = std::make_unique<DebounceRule>();
    if (!converter.Convert(it, rule.get()))
      continue;
    for (const URLPattern& pattern : rule->include_pattern_set()) {
      if (!pattern.host().empty()) {
        std::string etldp1 =
            net::registry_controlled_domains::GetDomainAndRegistry(
                pattern.host(),
                net::registry_controlled_domains::PrivateRegistryFilter::
                    INCLUDE_PRIVATE_REGISTRIES);
        hosts.push_back(std::move(etldp1));
      }
    }
    rules_.push_back(std::move(rule));
  }
  host_cache_ = std::move(hosts);
  for (Observer& observer : observers_)
    observer.OnRulesReady(this);
}

void DebounceComponentInstaller::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  resource_dir_ = install_dir.AppendASCII(kDebounceConfigFileVersion);
  LoadDirectlyFromResourcePath();
}

}  // namespace debounce
