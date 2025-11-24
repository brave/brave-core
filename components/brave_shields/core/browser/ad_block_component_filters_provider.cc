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
#include "base/rand_util.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "components/component_updater/component_updater_service.h"

constexpr char kListFile[] = "list.txt";

namespace brave_shields {

namespace {

void AddNothingToFilterSet(rust::Box<adblock::FilterSet>*) {}

// static
void AddDATBufferToFilterSet(uint8_t permission_mask,
                             DATFileDataBuffer buffer,
                             const perfetto::Flow& flow,
                             rust::Box<adblock::FilterSet>* filter_set) {
  TRACE_EVENT("brave.adblock",
              "AddDATBufferToFilterSet_AdBlockComponentFiltersProvider", flow);
  (*filter_set)->add_filter_list_with_permissions(buffer, permission_mask);
}

// static
void OnReadDATFileData(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb,
    uint8_t permission_mask,
    const perfetto::Flow& flow,
    DATFileDataBuffer buffer) {
  TRACE_EVENT("brave.adblock",
              "OnReadDATFileData_AdBlockComponentFiltersProvider", flow);
  std::move(cb).Run(
      base::BindOnce(&AddDATBufferToFilterSet, permission_mask, buffer, flow));
}

}  // namespace

AdBlockComponentFiltersProvider::AdBlockComponentFiltersProvider(
    component_updater::ComponentUpdateService* cus,
    AdBlockFiltersProviderManager* manager,
    std::string component_id,
    std::string base64_public_key,
    std::string title,
    uint8_t permission_mask,
    bool is_default_engine)
    : AdBlockFiltersProvider(is_default_engine, manager),
      component_id_(component_id),
      permission_mask_(permission_mask),
      component_updater_service_(cus) {
  // Can be nullptr in unit tests
  if (cus) {
    TRACE_EVENT("brave.adblock", "AdBlockComponentFiltersProvider::Register",
                perfetto::Flow::FromPointer(this), "component_id",
                component_id_);
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
    AdBlockFiltersProviderManager* manager,
    const FilterListCatalogEntry& catalog_entry,
    bool is_default_engine)
    : AdBlockComponentFiltersProvider(cus,
                                      manager,
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
    const base::FilePath& path) {
  TRACE_EVENT(
      "brave.adblock", "AdBlockComponentFiltersProvider::OnComponentReady",
      perfetto::TerminatingFlow::FromPointer(this), "path", path.value());
  base::FilePath old_path = component_path_;
  component_path_ = path;

  NotifyObservers(engine_is_default_);

  if (!old_path.empty()) {
    base::ThreadPool::PostTask(
        FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
        base::BindOnce(IgnoreResult(&base::DeletePathRecursively), old_path));
  }
}

bool AdBlockComponentFiltersProvider::IsInitialized() const {
  return !component_path_.empty();
}

base::FilePath AdBlockComponentFiltersProvider::GetFilterSetPath() {
  if (component_path_.empty()) {
    // Since we know it's empty return it as is.
    return component_path_;
  }

  return component_path_.AppendASCII(kListFile);
}

void AdBlockComponentFiltersProvider::LoadFilterSet(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) {
  base::FilePath list_file_path = GetFilterSetPath();

  const auto flow = perfetto::Flow::ProcessScoped(base::RandUint64());
  TRACE_EVENT("brave.adblock", "AdBlockComponentFiltersProvider::LoadFilterSet",
              flow);

  if (list_file_path.empty()) {
    // If the path is not ready yet, provide a no-op callback immediately. An
    // update will be pushed later to notify about the newly available list.
    std::move(cb).Run(base::BindOnce(AddNothingToFilterSet));
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_path),
      base::BindOnce(&OnReadDATFileData, std::move(cb), permission_mask_,
                     flow));
}

}  // namespace brave_shields
