// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/test/test_filters_provider.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

namespace {

void AddDATBufferToFilterSet(uint8_t permission_mask,
                             DATFileDataBuffer buffer,
                             rust::Box<adblock::FilterSet>* filter_set) {
  (*filter_set)->add_filter_list_with_permissions(buffer, permission_mask);
}

}  // namespace

TestFiltersProvider::TestFiltersProvider(const std::string& rules)
    : TestFiltersProvider(rules, true, 0) {}
TestFiltersProvider::TestFiltersProvider(const std::string& rules,
                                         bool engine_is_default,
                                         uint8_t permission_mask)
    : AdBlockFiltersProvider(
          engine_is_default,
          nullptr),  // must be set later by RegisterToFiltersProviderManager
      rules_(rules),
      permission_mask_(permission_mask),
      is_initialized_(false) {}

TestFiltersProvider::~TestFiltersProvider() = default;

void TestFiltersProvider::RegisterAsSourceProvider(
    AdBlockService* ad_block_service) {
  RegisterAsSourceProvider(ad_block_service->filters_provider_manager());
}

void TestFiltersProvider::RegisterAsSourceProvider(
    AdBlockFiltersProviderManager* filters_provider_manager) {
  AdBlockFiltersProvider::filters_provider_manager_ = filters_provider_manager;
  AdBlockFiltersProvider::filters_provider_manager_->AddProvider(
      this, engine_is_default_);
  Initialize();
}

std::string TestFiltersProvider::GetNameForDebugging() {
  return "TestFiltersProvider";
}

void TestFiltersProvider::LoadFilterSet(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) {
  auto buffer = std::vector<unsigned char>(rules_.begin(), rules_.end());
  std::move(cb).Run(
      base::BindOnce(&AddDATBufferToFilterSet, permission_mask_, buffer));
}

void TestFiltersProvider::Initialize() {
  CHECK(!is_initialized_);
  is_initialized_ = true;
  NotifyObservers(engine_is_default_);
}

bool TestFiltersProvider::IsInitialized() const {
  return is_initialized_;
}

}  // namespace brave_shields
