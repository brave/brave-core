/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_custom_filters_provider.h"

#include <utility>
#include <vector>

#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

namespace {

// Custom filters get all permissions granted, i.e. all bits of the mask set,
// i.e. the maximum possible uint8_t.
const uint8_t kCustomFiltersPermissionLevel = UINT8_MAX;

}  // namespace

AdBlockCustomFiltersProvider::AdBlockCustomFiltersProvider(
    PrefService* local_state)
    : AdBlockFiltersProvider(false), local_state_(local_state) {}

AdBlockCustomFiltersProvider::~AdBlockCustomFiltersProvider() {}

void AdBlockCustomFiltersProvider::HideElementOnHost(
    const std::string& css_selector,
    const std::string& host) {
  std::string custom_filters = GetCustomFilters();
  UpdateCustomFilters(custom_filters + '\n' + host + "##" + css_selector +
                      '\n');
}

std::string AdBlockCustomFiltersProvider::GetNameForDebugging() {
  return "AdBlockCustomFiltersProvider";
}

void AdBlockCustomFiltersProvider::CreateSiteExemption(
    const std::string& host) {
  std::string custom_filters = GetCustomFilters();
  UpdateCustomFilters(custom_filters + '\n' + "@@||" + host +
                      "^$first-party\n");
}

std::string AdBlockCustomFiltersProvider::GetCustomFilters() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_)
    return std::string();
  return local_state_->GetString(prefs::kAdBlockCustomFilters);
}

bool AdBlockCustomFiltersProvider::UpdateCustomFilters(
    const std::string& custom_filters) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_)
    return false;
  local_state_->SetString(prefs::kAdBlockCustomFilters, custom_filters);

  NotifyObservers();

  return true;
}

void AdBlockCustomFiltersProvider::LoadFilterSet(
    rust::Box<adblock::FilterSet>* filter_set,
    base::OnceCallback<void()> cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto custom_filters = GetCustomFilters();

  auto buffer =
      std::vector<unsigned char>(custom_filters.begin(), custom_filters.end());
  (*filter_set)
      ->add_filter_list_with_permissions(buffer, kCustomFiltersPermissionLevel);

  // PostTask so this has an async return to match other loaders
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(cb)));
}

// The custom filters provider can provide its filters immediately after being
// observed.
void AdBlockCustomFiltersProvider::AddObserver(
    AdBlockFiltersProvider::Observer* observer) {
  AdBlockFiltersProvider::AddObserver(observer);
  NotifyObservers();
}

}  // namespace brave_shields
