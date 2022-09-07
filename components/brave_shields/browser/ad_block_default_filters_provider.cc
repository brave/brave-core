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

AdBlockDefaultFiltersProvider::~AdBlockDefaultFiltersProvider() = default;

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

}  // namespace brave_shields
