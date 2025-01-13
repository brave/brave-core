// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/component_contents_accessor.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"

constexpr char kListCatalogFile[] = "list_catalog.json";

namespace brave_shields {

AdBlockFilterListCatalogProvider::AdBlockFilterListCatalogProvider(
    component_updater::ComponentUpdateService* cus) {
  // Can be nullptr in unit tests
  if (cus) {
    RegisterAdBlockFilterListCatalogComponent(
        cus,
        base::BindRepeating(&AdBlockFilterListCatalogProvider::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

AdBlockFilterListCatalogProvider::~AdBlockFilterListCatalogProvider() = default;

void AdBlockFilterListCatalogProvider::AddObserver(
    AdBlockFilterListCatalogProvider::Observer* observer) {
  observers_.AddObserver(observer);
}

void AdBlockFilterListCatalogProvider::RemoveObserver(
    AdBlockFilterListCatalogProvider::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded(
    const std::string& catalog_json) {
  for (auto& observer : observers_) {
    observer.OnFilterListCatalogLoaded(catalog_json);
  }
}

void AdBlockFilterListCatalogProvider::OnComponentReady(
    scoped_refptr<component_updater::ComponentContentsAccessor> accessor) {
  component_accessor_ = std::move(accessor);

  LoadFilterListCatalog(base::BindOnce(
      &AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded,
      weak_factory_.GetWeakPtr()));
}

void AdBlockFilterListCatalogProvider::LoadFilterListCatalog(
    base::OnceCallback<void(const std::string& catalog_json)> cb) {
  if (!component_accessor_) {
    // If the component is not ready yet, don't run the callback. An update
    // should be pushed soon.
    return;
  }

  auto on_load = base::BindOnce(
      [](base::OnceCallback<void(const std::string& catalog_json)> cb,
         std::optional<std::string> data) {
        std::move(cb).Run(std::move(data).value_or(std::string()));
      },
      std::move(cb));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &component_updater::ComponentContentsAccessor::GetFileAsString,
          base::RetainedRef(component_accessor_),
          base::FilePath::FromASCII(kListCatalogFile)),
      std::move(on_load));
}

}  // namespace brave_shields
