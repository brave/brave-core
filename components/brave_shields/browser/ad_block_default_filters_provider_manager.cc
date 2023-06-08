/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_default_filters_provider_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_shields {

namespace {

static void OnDATLoaded(
    base::OnceCallback<void(DATFileDataBuffer)> collect_and_merge,
    bool deserialize,
    const DATFileDataBuffer& dat_buf) {
  // This manager should never be used for a provider that returns a serialized
  // DAT. The ability should be removed from the FiltersProvider API when
  // possible.
  CHECK(!deserialize);

  std::move(collect_and_merge).Run(dat_buf);
}

}  // namespace

// static
AdBlockDefaultFiltersProviderManager*
AdBlockDefaultFiltersProviderManager::GetInstance() {
  return base::Singleton<AdBlockDefaultFiltersProviderManager>::get();
}

AdBlockDefaultFiltersProviderManager::AdBlockDefaultFiltersProviderManager() =
    default;

AdBlockDefaultFiltersProviderManager::~AdBlockDefaultFiltersProviderManager() =
    default;

void AdBlockDefaultFiltersProviderManager::AddProvider(
    AdBlockFiltersProvider* provider) {
  auto rv = filters_providers_.insert(provider);
  DCHECK(rv.second);
  provider->AddObserver(this);
}

void AdBlockDefaultFiltersProviderManager::RemoveProvider(
    AdBlockFiltersProvider* provider) {
  auto it = filters_providers_.find(provider);
  DCHECK(it != filters_providers_.end());
  (*it)->RemoveObserver(this);
  filters_providers_.erase(it);
  NotifyObservers();
}

void AdBlockDefaultFiltersProviderManager::OnChanged() {
  NotifyObservers();
}

void AdBlockDefaultFiltersProviderManager::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  if (task_tracker_.HasTrackedTasks()) {
    // There's already an in-progress load, cancel it.
    task_tracker_.TryCancelAll();
  }

  const auto collect_and_merge = base::BarrierCallback<DATFileDataBuffer>(
      filters_providers_.size(),
      base::BindOnce(&AdBlockDefaultFiltersProviderManager::FinishCombinating,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
  for (auto* provider : filters_providers_) {
    task_tracker_.PostTask(
        base::SequencedTaskRunner::GetCurrentDefault().get(), FROM_HERE,
        base::BindOnce(
            &AdBlockFiltersProvider::LoadDAT, provider->AsWeakPtr(),
            base::BindOnce(OnDATLoaded, std::move(collect_and_merge))));
  }
}

void AdBlockDefaultFiltersProviderManager::FinishCombinating(
    base::OnceCallback<void(bool, const DATFileDataBuffer&)> cb,
    const std::vector<DATFileDataBuffer>& results) {
  DATFileDataBuffer combined_list;
  for (const auto& dat_buf : results) {
    combined_list.push_back('\n');
    combined_list.insert(combined_list.end(), dat_buf.begin(), dat_buf.end());
  }
  if (combined_list.size() == 0) {
    // Small workaround for code in
    // AdBlockService::SourceProviderObserver::OnResourcesLoaded that encodes a
    // state using an entirely empty DAT.
    combined_list.push_back('\n');
  }
  std::move(cb).Run(false, combined_list);
}

}  // namespace brave_shields
