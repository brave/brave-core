/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_filters_provider.h"

#include <string>
#include <utility>

#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"
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

void AdBlockSubscriptionFiltersProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
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
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb,
    const DATFileDataBuffer& dat_buf) {
  auto metadata = adblock::read_list_metadata(dat_buf);
  on_metadata_retrieved_.Run(metadata);
  std::move(cb).Run(false, dat_buf);
}

void AdBlockSubscriptionFiltersProvider::OnListAvailable() {
  NotifyObservers(engine_is_default_);
}

}  // namespace brave_shields
