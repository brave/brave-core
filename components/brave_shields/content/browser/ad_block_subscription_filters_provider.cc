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
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/adblock/rs/src/lib.rs.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

AdBlockSubscriptionFiltersProvider::AdBlockSubscriptionFiltersProvider(
    PrefService* local_state,
    AdBlockFiltersProviderManager* manager,
    base::FilePath list_file,
    base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
        on_metadata_retrieved)
    : AdBlockFiltersProvider(false, manager),
      list_file_(list_file),
      on_metadata_retrieved_(on_metadata_retrieved) {}

AdBlockSubscriptionFiltersProvider::~AdBlockSubscriptionFiltersProvider() =
    default;

void AdBlockSubscriptionFiltersProvider::LoadFilters(
    base::OnceCallback<
        void(std::vector<unsigned char> filter_buffer,
             uint8_t permission_mask,
             base::OnceCallback<void(adblock::FilterListMetadata)> on_metadata)>
        cb) {
  const auto flow = perfetto::Flow::FromPointer(this);
  TRACE_EVENT("brave.adblock",
              "AdBlockSubscriptionFiltersProvider::LoadFilterSet", flow);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_),
      base::BindOnce(&AdBlockSubscriptionFiltersProvider::OnDATFileDataReady,
                     weak_factory_.GetWeakPtr(), std::move(cb), flow));
}

std::string AdBlockSubscriptionFiltersProvider::GetNameForDebugging() {
  return "AdBlockSubscriptionFiltersProvider";
}

void AdBlockSubscriptionFiltersProvider::OnDATFileDataReady(
    base::OnceCallback<void(
        std::vector<unsigned char> filter_buffer,
        uint8_t permission_mask,
        base::OnceCallback<void(adblock::FilterListMetadata)> on_metadata)> cb,
    const perfetto::Flow& flow,
    const DATFileDataBuffer& dat_buf) {
  TRACE_EVENT("brave.adblock",
              "AdBlockSubscriptionFiltersProvider::OnDATFileDataReady", flow);
  std::move(cb).Run(
      dat_buf, 0,
      base::BindOnce(
          [](scoped_refptr<base::SequencedTaskRunner> task_runner,
             base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
                 on_metadata,
             adblock::FilterListMetadata metadata) {
            task_runner->PostTask(FROM_HERE,
                                  base::BindOnce(on_metadata, metadata));
          },
          base::SingleThreadTaskRunner::GetCurrentDefault(),
          on_metadata_retrieved_));
}

void AdBlockSubscriptionFiltersProvider::OnListAvailable() {
  NotifyObservers(engine_is_default_);
}

}  // namespace brave_shields
