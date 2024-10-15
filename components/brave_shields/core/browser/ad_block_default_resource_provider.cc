// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"

namespace {

constexpr char kAdBlockResourcesFilename[] = "resources.json";

}  // namespace

namespace brave_shields {

AdBlockDefaultResourceProvider::AdBlockDefaultResourceProvider(
    component_updater::ComponentUpdateService* cus) {
  // Can be nullptr in unit tests
  if (!cus) {
    return;
  }

  RegisterAdBlockDefaultResourceComponent(
      cus,
      base::BindRepeating(&AdBlockDefaultResourceProvider::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

AdBlockDefaultResourceProvider::~AdBlockDefaultResourceProvider() = default;

base::FilePath AdBlockDefaultResourceProvider::GetResourcesPath() {
  if (component_path_.empty()) {
    // Since we know it's empty return it as is.
    return component_path_;
  }

  return component_path_.AppendASCII(kAdBlockResourcesFilename);
}

void AdBlockDefaultResourceProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;
  base::FilePath resources_path = GetResourcesPath();

  if (resources_path.empty()) {
    // This should not happen, but if it does, we should not proceed.
    return;
  }

  // Load the resources (as a string)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     resources_path),
      base::BindOnce(&AdBlockDefaultResourceProvider::NotifyResourcesLoaded,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockDefaultResourceProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  base::FilePath resources_path = GetResourcesPath();
  if (resources_path.empty()) {
    // If the path is not ready yet, run the callback with empty resources to
    // avoid blocking filter data loads.
    std::move(cb).Run("[]");
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     resources_path),
      std::move(cb));
}

}  // namespace brave_shields
