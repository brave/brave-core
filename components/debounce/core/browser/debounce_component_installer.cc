// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/core/browser/debounce_component_installer.h"

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace debounce {

DebounceComponentInstaller::DebounceComponentInstaller(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service) {}

DebounceComponentInstaller::~DebounceComponentInstaller() = default;

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
  auto parsed_rules = DebounceRule::ParseRules(contents);
  if (!parsed_rules.has_value()) {
    LOG(WARNING) << parsed_rules.error();
    return;
  }
  rules_.clear();
  host_cache_.clear();
  rules_ = std::move(parsed_rules.value().first);
  host_cache_ = parsed_rules.value().second;
  for (Observer& observer : observers_) {
    observer.OnRulesReady(this);
  }
}

void DebounceComponentInstaller::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  resource_dir_ = install_dir.AppendASCII(kDebounceConfigFileVersion);
  LoadDirectlyFromResourcePath();
}

}  // namespace debounce
