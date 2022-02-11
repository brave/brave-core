/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_custom_filters_source_provider.h"

#include <utility>
#include <vector>

#include "base/threading/thread_task_runner_handle.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

AdBlockCustomFiltersSourceProvider::AdBlockCustomFiltersSourceProvider(
    PrefService* local_state)
    : local_state_(local_state) {}

AdBlockCustomFiltersSourceProvider::~AdBlockCustomFiltersSourceProvider() {}

std::string AdBlockCustomFiltersSourceProvider::GetCustomFilters() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_)
    return std::string();
  return local_state_->GetString(prefs::kAdBlockCustomFilters);
}

bool AdBlockCustomFiltersSourceProvider::UpdateCustomFilters(
    const std::string& custom_filters) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_)
    return false;
  local_state_->SetString(prefs::kAdBlockCustomFilters, custom_filters);

  auto buffer =
      std::vector<unsigned char>(custom_filters.begin(), custom_filters.end());
  OnDATLoaded(false, buffer);

  return true;
}

void AdBlockCustomFiltersSourceProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto custom_filters = GetCustomFilters();

  auto buffer =
      std::vector<unsigned char>(custom_filters.begin(), custom_filters.end());

  // PostTask so this has an async return to match other loaders
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(cb), false, std::move(buffer)));
}

}  // namespace brave_shields
