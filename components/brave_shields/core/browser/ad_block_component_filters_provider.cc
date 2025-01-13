// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/component_contents_accessor.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "components/component_updater/component_updater_service.h"

constexpr char kListFile[] = "list.txt";

namespace brave_shields {

namespace {

// static
void AddDATBufferToFilterSet(uint8_t permission_mask,
                             DATFileDataBuffer buffer,
                             rust::Box<adblock::FilterSet>* filter_set) {
  (*filter_set)->add_filter_list_with_permissions(buffer, permission_mask);
}

// static
void OnReadDATFileData(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb,
    uint8_t permission_mask,
    std::optional<DATFileDataBuffer> buffer) {
  std::move(cb).Run(
      base::BindOnce(&AddDATBufferToFilterSet, permission_mask,
                     std::move(buffer).value_or(DATFileDataBuffer())));
}

}  // namespace

AdBlockComponentFiltersProvider::AdBlockComponentFiltersProvider(
    component_updater::ComponentUpdateService* cus,
    std::string component_id,
    std::string base64_public_key,
    std::string title,
    uint8_t permission_mask,
    bool is_default_engine)
    : AdBlockFiltersProvider(is_default_engine),
      component_id_(component_id),
      permission_mask_(permission_mask),
      component_updater_service_(cus) {
  // Can be nullptr in unit tests
  if (cus) {
    RegisterAdBlockFiltersComponent(
        cus, base64_public_key, component_id_, title,
        base::BindRepeating(&AdBlockComponentFiltersProvider::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }
}

std::string AdBlockComponentFiltersProvider::GetNameForDebugging() {
  return "AdBlockComponentFiltersProvider";
}

AdBlockComponentFiltersProvider::AdBlockComponentFiltersProvider(
    component_updater::ComponentUpdateService* cus,
    const FilterListCatalogEntry& catalog_entry,
    bool is_default_engine)
    : AdBlockComponentFiltersProvider(cus,
                                      catalog_entry.component_id,
                                      catalog_entry.base64_public_key,
                                      catalog_entry.title,
                                      catalog_entry.permission_mask,
                                      is_default_engine) {}

AdBlockComponentFiltersProvider::~AdBlockComponentFiltersProvider() {}

void AdBlockComponentFiltersProvider::UnregisterComponent() {
  // Can be nullptr in unit tests
  if (component_updater_service_) {
    component_updater_service_->UnregisterComponent(component_id_);
  }
}

void AdBlockComponentFiltersProvider::OnComponentReady(
    scoped_refptr<component_updater::ComponentContentsAccessor> accessor) {
  if (component_accessor_) {
    base::ThreadPool::PostTask(
        FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
        base::GetDeletePathRecursivelyCallback(
            component_accessor_->GetComponentRoot()));
  }
  component_accessor_ = std::move(accessor);

  NotifyObservers(engine_is_default_);
}

bool AdBlockComponentFiltersProvider::IsInitialized() const {
  return !!component_accessor_;
}

base::FilePath AdBlockComponentFiltersProvider::GetFilterSetPath() {
  if (!component_accessor_) {
    return base::FilePath();
  }
  return component_accessor_->GetComponentRoot().AppendASCII(kListFile);
}

void AdBlockComponentFiltersProvider::LoadFilterSet(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) {
  if (!component_accessor_) {
    // If the component is not ready yet, provide a no-op callback immediately.
    // An update will be pushed later to notify about the newly available list.
    std::move(cb).Run(base::DoNothing());
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &component_updater::ComponentContentsAccessor::GetFileAsBytes,
          base::RetainedRef(component_accessor_),
          base::FilePath::FromASCII(kListFile)),
      base::BindOnce(&OnReadDATFileData, std::move(cb), permission_mask_));
}

}  // namespace brave_shields
