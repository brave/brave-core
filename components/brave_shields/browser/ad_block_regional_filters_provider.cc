/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_filters_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/browser/ad_block_component_installer.h"
#include "brave/components/brave_shields/browser/filter_list_catalog_entry.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/browser_task_traits.h"

constexpr char kListFile[] = "list.txt";

namespace brave_shields {

AdBlockRegionalFiltersProvider::AdBlockRegionalFiltersProvider(
    component_updater::ComponentUpdateService* cus,
    const FilterListCatalogEntry& catalog_entry)
    : component_id_(catalog_entry.component_id),
      component_updater_service_(cus) {
  // Can be nullptr in unit tests
  if (cus) {
    RegisterAdBlockRegionalComponent(
        component_updater_service_, catalog_entry.base64_public_key,
        component_id_, catalog_entry.title,
        base::BindRepeating(&AdBlockRegionalFiltersProvider::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

AdBlockRegionalFiltersProvider::~AdBlockRegionalFiltersProvider() = default;

void AdBlockRegionalFiltersProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;

  base::FilePath list_file_path = component_path_.AppendASCII(kListFile);

  // Load the list as a string
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_path),
      base::BindOnce(&AdBlockRegionalFiltersProvider::OnDATLoaded,
                     weak_factory_.GetWeakPtr(), false));
}

void AdBlockRegionalFiltersProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  if (component_path_.empty()) {
    // If the path is not ready yet, don't run the callback. An update should
    // be pushed soon.
    return;
  }

  base::FilePath list_file_path = component_path_.AppendASCII(kListFile);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_path),
      base::BindOnce(std::move(cb), false));
}

bool AdBlockRegionalFiltersProvider::Delete() && {
  return component_updater_service_->UnregisterComponent(component_id_);
}

}  // namespace brave_shields
