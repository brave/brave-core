/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_custom_filters_source_provider.h"

#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_shields {

AdBlockCustomFiltersSourceProvider::AdBlockCustomFiltersSourceProvider(
    PrefService* local_state)
    : local_state_(local_state) {}

AdBlockCustomFiltersSourceProvider::~AdBlockCustomFiltersSourceProvider() {}

std::string AdBlockCustomFiltersSourceProvider::GetCustomFilters() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!local_state_)
    return std::string();
  return local_state_->GetString(prefs::kAdBlockCustomFilters);
}

bool AdBlockCustomFiltersSourceProvider::UpdateCustomFilters(
    const std::string& custom_filters) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!local_state_)
    return false;
  local_state_->SetString(prefs::kAdBlockCustomFilters, custom_filters);

  auto buffer =
      std::vector<unsigned char>(custom_filters.begin(), custom_filters.end());
  ProvideNewListSource(buffer);

  return true;
}

void RespondWithCustomFilters(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb,
    std::string custom_filters) {
  auto buffer =
      std::vector<unsigned char>(custom_filters.begin(), custom_filters.end());
  std::move(cb).Run(false, buffer);
}

void AdBlockCustomFiltersSourceProvider::Load(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  content::GetUIThreadTaskRunner({})->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockCustomFiltersSourceProvider::GetCustomFilters,
                     base::Unretained(this)),
      base::BindOnce(&RespondWithCustomFilters, std::move(cb)));
}

}  // namespace brave_shields
