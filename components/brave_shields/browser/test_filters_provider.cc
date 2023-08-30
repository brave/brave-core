/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/test_filters_provider.h"

#include <string>
#include <utility>

#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

namespace brave_shields {

TestFiltersProvider::TestFiltersProvider(const std::string& rules,
                                         const std::string& resources)
    : AdBlockFiltersProvider(true), rules_(rules), resources_(resources) {}
TestFiltersProvider::TestFiltersProvider(const std::string& rules,
                                         const std::string& resources,
                                         bool engine_is_default)
    : AdBlockFiltersProvider(engine_is_default),
      rules_(rules),
      resources_(resources) {}

TestFiltersProvider::~TestFiltersProvider() = default;

std::string TestFiltersProvider::GetNameForDebugging() {
  return "TestFiltersProvider";
}

void TestFiltersProvider::LoadDATBuffer(
    base::OnceCallback<void(const DATFileDataBuffer& dat_buf)> cb) {
  auto buffer = std::vector<unsigned char>(rules_.begin(), rules_.end());
  std::move(cb).Run(buffer);
}

void TestFiltersProvider::LoadFilterSet(
    rust::Box<adblock::FilterSet>* filter_set,
    base::OnceCallback<void()> cb) {
  auto buffer = std::vector<unsigned char>(rules_.begin(), rules_.end());
  (*filter_set)->add_filter_list(buffer);
  std::move(cb).Run();
}

void TestFiltersProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  std::move(cb).Run(resources_);
}

}  // namespace brave_shields
