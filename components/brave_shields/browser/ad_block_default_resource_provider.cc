/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_default_resource_provider.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/metrics/field_trial_params.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/browser/ad_block_component_installer.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"

namespace {
constexpr char kAdBlockResourcesFilename[] = "resources.json";

BASE_DECLARE_FEATURE(kAdBlockDefaultResourceUpdateInterval);
constexpr base::FeatureParam<int> kComponentUpdateCheckIntervalMins{
    &kAdBlockDefaultResourceUpdateInterval, "update_interval_mins", 100};
BASE_FEATURE(kAdBlockDefaultResourceUpdateInterval,
             "AdBlockDefaultResourceUpdateInterval",
             base::FEATURE_ENABLED_BY_DEFAULT);

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
  update_check_timer_.Start(
      FROM_HERE, base::Minutes(kComponentUpdateCheckIntervalMins.Get()),
      base::BindRepeating([]() {
        // Separated into two methods as exception component is not available in
        // iOS. So can't check it from CheckAdBlockComponentsUpdate() together.
        CheckAdBlockComponentsUpdate();
        CheckAdBlockExceptionComponentsUpdate();
      }));
}

AdBlockDefaultResourceProvider::~AdBlockDefaultResourceProvider() = default;

void AdBlockDefaultResourceProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;

  // Load the resources (as a string)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kAdBlockResourcesFilename)),
      base::BindOnce(&AdBlockDefaultResourceProvider::OnResourcesLoaded,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockDefaultResourceProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  if (component_path_.empty()) {
    // If the path is not ready yet, run the callback with empty resources to
    // avoid blocking filter data loads.
    std::move(cb).Run("[]");
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kAdBlockResourcesFilename)),
      std::move(cb));
}

}  // namespace brave_shields
