/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_default_source_provider.h"

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

AdBlockDefaultSourceProvider::AdBlockDefaultSourceProvider(
    component_updater::ComponentUpdateService* cus,
    base::RepeatingCallback<void(const std::string& regional_catalog)>
        regional_catalog_available_cb)
    : regional_catalog_available_cb_(std::move(regional_catalog_available_cb)) {
  // Can be nullptr in unit tests
  if (cus) {
    RegisterAdBlockDefaultComponent(
        cus,
        base::BindRepeating(&AdBlockDefaultSourceProvider::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

AdBlockDefaultSourceProvider::~AdBlockDefaultSourceProvider() {}

void AdBlockDefaultSourceProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData,
                     component_path_.AppendASCII(DAT_FILE)),
      base::BindOnce(&AdBlockDefaultSourceProvider::ProvideNewDAT,
                     weak_factory_.GetWeakPtr()));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kAdBlockResourcesFilename)),
      base::BindOnce(&AdBlockDefaultSourceProvider::ProvideNewResources,
                     weak_factory_.GetWeakPtr()));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(REGIONAL_CATALOG)),
      base::BindOnce(regional_catalog_available_cb_));
}

void AdBlockDefaultSourceProvider::Load(
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

void AdBlockDefaultSourceProvider::Load(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  if (component_path_.empty()) {
    // If the path is not ready yet, don't run the callback. An update should be
    // pushed soon.
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kAdBlockResourcesFilename)),
      std::move(cb));
}

}  // namespace brave_shields
