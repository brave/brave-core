/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_VIRTUAL_PREF_PROVIDER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_VIRTUAL_PREF_PROVIDER_DELEGATE_H_

#include <string>
#include <string_view>

#include "base/values.h"
#include "brave/components/brave_ads/core/browser/virtual_pref/virtual_pref_provider.h"

namespace brave_ads::test {

// No-op implementation of `VirtualPrefProvider::Delegate` for unit tests.
class FakeVirtualPrefProviderDelegate : public VirtualPrefProvider::Delegate {
 public:
  FakeVirtualPrefProviderDelegate();

  FakeVirtualPrefProviderDelegate(const FakeVirtualPrefProviderDelegate&) =
      delete;
  FakeVirtualPrefProviderDelegate& operator=(
      const FakeVirtualPrefProviderDelegate&) = delete;

  ~FakeVirtualPrefProviderDelegate() override;

  // VirtualPrefProvider::Delegate:
  std::string_view GetChannel() const override;
  std::string GetDefaultSearchEngineName() const override;
  base::DictValue GetSerpMetrics() const override;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_VIRTUAL_PREF_PROVIDER_DELEGATE_H_
