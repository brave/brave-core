// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_subscription_filters_provider.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

AdBlockSubscriptionFiltersProvider::AdBlockSubscriptionFiltersProvider(
    PrefService* local_state,
    base::FilePath list_file,
    base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
        on_metadata_retrieved)
    : AdBlockFiltersProvider(false),
      list_file_(list_file),
      on_metadata_retrieved_(on_metadata_retrieved) {}

AdBlockSubscriptionFiltersProvider::~AdBlockSubscriptionFiltersProvider() =
    default;

void AdBlockSubscriptionFiltersProvider::LoadFilterSet(
    base::OnceCallback<void(std::pair<uint8_t, DATFileDataBuffer>)> cb) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_),
      base::BindOnce(&AdBlockSubscriptionFiltersProvider::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
}

std::string AdBlockSubscriptionFiltersProvider::GetNameForDebugging() {
  return "AdBlockSubscriptionFiltersProvider";
}

void AdBlockSubscriptionFiltersProvider::OnDATFileDataReady(
    base::OnceCallback<void(std::pair<uint8_t, DATFileDataBuffer>)> cb,
    const DATFileDataBuffer& dat_buf) {
  const auto metadata = adblock::read_list_metadata(dat_buf);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(on_metadata_retrieved_, metadata));
  std::move(cb).Run({0, dat_buf});
}

void AdBlockSubscriptionFiltersProvider::OnListAvailable() {
  NotifyObservers(engine_is_default_);
}

}  // namespace brave_shields
