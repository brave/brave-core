/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"

#include "base/logging.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"

using brave_component_updater::BraveComponent;

namespace brave_shields {

AdBlockCustomFiltersService::AdBlockCustomFiltersService(
    BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate) {}

AdBlockCustomFiltersService::~AdBlockCustomFiltersService() {}

bool AdBlockCustomFiltersService::Init() {
  return UpdateCustomFilters(GetCustomFilters());
}

std::string AdBlockCustomFiltersService::GetCustomFilters() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = delegate()->local_state();
  if (!local_state)
    return std::string();
  return local_state->GetString(prefs::kAdBlockCustomFilters);
}

bool AdBlockCustomFiltersService::UpdateCustomFilters(
    const std::string& custom_filters) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = delegate()->local_state();
  if (!local_state)
    return false;
  local_state->SetString(prefs::kAdBlockCustomFilters, custom_filters);

  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AdBlockCustomFiltersService::UpdateCustomFiltersOnFileTaskRunner,
          base::Unretained(this), custom_filters));

  return true;
}

void AdBlockCustomFiltersService::UpdateCustomFiltersOnFileTaskRunner(
    const std::string& custom_filters) {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());
  ad_block_client_.reset(new adblock::Engine(custom_filters.c_str()));
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockCustomFiltersService> AdBlockCustomFiltersServiceFactory(
    BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockCustomFiltersService>(delegate);
}

}  // namespace brave_shields
