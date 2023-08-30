/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_filters_provider.h"

#include <string>
#include <utility>

#include "base/logging.h"
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
    base::OnceCallback<void(const DATFileDataBuffer& dat_buf)> cb) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_),
      base::BindOnce(&AdBlockSubscriptionFiltersProvider::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
}

void AdBlockSubscriptionFiltersProvider::LoadFilterSet(
    rust::Box<adblock::FilterSet>* filter_set,
    base::OnceCallback<void()> cb) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_),
      base::BindOnce(
          &AdBlockSubscriptionFiltersProvider::OnDATFileDataReadyForFilterSet,
          weak_factory_.GetWeakPtr(), std::move(cb), filter_set));
}

std::string AdBlockSubscriptionFiltersProvider::GetNameForDebugging() {
  return "AdBlockSubscriptionFiltersProvider";
}

void AdBlockSubscriptionFiltersProvider::OnDATFileDataReady(
    base::OnceCallback<void(const DATFileDataBuffer& dat_buf)> cb,
    const DATFileDataBuffer& dat_buf) {
  auto metadata = adblock::read_list_metadata(dat_buf);
  on_metadata_retrieved_.Run(metadata);
  std::move(cb).Run(dat_buf);
}

void AdBlockSubscriptionFiltersProvider::OnDATFileDataReadyForFilterSet(
    base::OnceCallback<void()> cb,
    rust::Box<adblock::FilterSet>* filter_set,
    const DATFileDataBuffer& dat_buf) {
  auto result = (*filter_set)->add_filter_list(dat_buf);
  if (result.result_kind == adblock::ResultKind::Success) {
    on_metadata_retrieved_.Run(result.value);
  } else {
    VLOG(0) << "Subscription list parsing failed: "
            << result.error_message.c_str();
  }
  std::move(cb).Run();
}

void AdBlockSubscriptionFiltersProvider::OnListAvailable() {
  NotifyObservers();
}

}  // namespace brave_shields
