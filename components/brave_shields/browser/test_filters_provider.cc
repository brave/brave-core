/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/test_filters_provider.h"

#include <string>
#include <utility>

#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

namespace brave_shields {

namespace {

void AddDATBufferToFilterSet(uint8_t permission_mask,
                             DATFileDataBuffer buffer,
                             rust::Box<adblock::FilterSet>* filter_set) {
  (*filter_set)->add_filter_list_with_permissions(buffer, permission_mask);
}

}  // namespace

TestFiltersProvider::TestFiltersProvider(const std::string& rules,
                                         const std::string& resources)
    : AdBlockFiltersProvider(true), rules_(rules), resources_(resources) {}
TestFiltersProvider::TestFiltersProvider(const std::string& rules,
                                         const std::string& resources,
                                         bool engine_is_default,
                                         uint8_t permission_mask)
    : AdBlockFiltersProvider(engine_is_default),
      rules_(rules),
      resources_(resources),
      permission_mask_(permission_mask) {}

TestFiltersProvider::~TestFiltersProvider() = default;

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

void TestFiltersProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  std::move(cb).Run(resources_);
}

}  // namespace brave_shields
