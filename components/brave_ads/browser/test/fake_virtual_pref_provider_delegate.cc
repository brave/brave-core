/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_virtual_pref_provider_delegate.h"

namespace brave_ads::test {

FakeVirtualPrefProviderDelegate::FakeVirtualPrefProviderDelegate() = default;

FakeVirtualPrefProviderDelegate::~FakeVirtualPrefProviderDelegate() = default;

std::string_view FakeVirtualPrefProviderDelegate::GetChannel() const {
  return "";
}

std::string FakeVirtualPrefProviderDelegate::GetDefaultSearchEngineName()
    const {
  return "";
}

base::DictValue FakeVirtualPrefProviderDelegate::GetSerpMetrics() const {
  return {};
}

}  // namespace brave_ads::test
