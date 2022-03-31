/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_default_filters_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/browser/ad_block_component_installer.h"
#include "content/public/browser/browser_task_traits.h"

#define DAT_FILE "rs-ABPFilterParserData.dat"
#define REGIONAL_CATALOG "regional_catalog.json"
const char kAdBlockResourcesFilename[] = "resources.json";

namespace brave_shields {

AdBlockDefaultFiltersProvider::AdBlockDefaultFiltersProvider(
    component_updater::ComponentUpdateService* cus) {
  // Can be nullptr in unit tests
  if (cus) {
    RegisterAdBlockDefaultComponent(
        cus,
        base::BindRepeating(&AdBlockDefaultFiltersProvider::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

AdBlockDefaultFiltersProvider::~AdBlockDefaultFiltersProvider() {}

void AdBlockDefaultFiltersProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;

  // Load the DAT (as a buffer)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData,
                     component_path_.AppendASCII(DAT_FILE)),
      base::BindOnce(&AdBlockDefaultFiltersProvider::OnDATLoaded,
                     weak_factory_.GetWeakPtr(), true));

  // Load the resources (as a string)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kAdBlockResourcesFilename)),
      base::BindOnce(&AdBlockDefaultFiltersProvider::OnResourcesLoaded,
                     weak_factory_.GetWeakPtr()));

  // Load the regional catalog (as a string)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(REGIONAL_CATALOG)),
      base::BindOnce(&AdBlockDefaultFiltersProvider::OnRegionalCatalogLoaded,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockDefaultFiltersProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  if (component_path_.empty()) {
    // If the path is not ready yet, don't run the callback. An update should
    // be pushed soon.
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData,
                     component_path_.AppendASCII(DAT_FILE)),
      base::BindOnce(std::move(cb), true));
}

void AdBlockDefaultFiltersProvider::LoadResources(
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

void AdBlockDefaultFiltersProvider::LoadRegionalCatalog(
    base::OnceCallback<void(const std::string& catalog_json)> cb) {
  if (component_path_.empty()) {
    // If the path is not ready yet, don't run the callback. An update should be
    // pushed soon.
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(REGIONAL_CATALOG)),
      std::move(cb));
}

}  // namespace brave_shields
