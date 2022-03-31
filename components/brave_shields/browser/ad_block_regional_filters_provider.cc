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
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/browser_task_traits.h"

namespace brave_shields {

AdBlockRegionalFiltersProvider::AdBlockRegionalFiltersProvider(
    component_updater::ComponentUpdateService* cus,
    const adblock::FilterList& catalog_entry)
    : uuid_(catalog_entry.uuid),
      component_id_(catalog_entry.component_id),
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

AdBlockRegionalFiltersProvider::~AdBlockRegionalFiltersProvider() {}

void AdBlockRegionalFiltersProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;

  base::FilePath dat_file_path =
      component_path_.AppendASCII(std::string("rs-") + uuid_)
          .AddExtension(FILE_PATH_LITERAL(".dat"));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, dat_file_path),
      base::BindOnce(&AdBlockRegionalFiltersProvider::OnDATLoaded,
                     weak_factory_.GetWeakPtr(), true));
}

void AdBlockRegionalFiltersProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  if (component_path_.empty()) {
    // If the path is not ready yet, do nothing. An update should be pushed
    // soon.
    return;
  }

  base::FilePath dat_file_path =
      component_path_.AppendASCII(std::string("rs-") + uuid_)
          .AddExtension(FILE_PATH_LITERAL(".dat"));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, dat_file_path),
      base::BindOnce(std::move(cb), true));
}

bool AdBlockRegionalFiltersProvider::Delete() && {
  return component_updater_service_->UnregisterComponent(component_id_);
}

}  // namespace brave_shields
