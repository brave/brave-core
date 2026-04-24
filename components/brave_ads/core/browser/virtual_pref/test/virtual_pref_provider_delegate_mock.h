/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_VIRTUAL_PREF_TEST_VIRTUAL_PREF_PROVIDER_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_VIRTUAL_PREF_TEST_VIRTUAL_PREF_PROVIDER_DELEGATE_MOCK_H_

#include <string>
#include <string_view>

#include "base/values.h"
#include "brave/components/brave_ads/core/browser/virtual_pref/virtual_pref_provider.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

// Mock for VirtualPrefProvider::Delegate used in unit tests.
class VirtualPrefProviderDelegateMock : public VirtualPrefProvider::Delegate {
 public:
  VirtualPrefProviderDelegateMock();

  VirtualPrefProviderDelegateMock(const VirtualPrefProviderDelegateMock&) =
      delete;
  VirtualPrefProviderDelegateMock& operator=(
      const VirtualPrefProviderDelegateMock&) = delete;

  ~VirtualPrefProviderDelegateMock() override;

  MOCK_METHOD(std::string_view, GetChannel, (), (const, override));

  MOCK_METHOD(std::string, GetDefaultSearchEngineName, (), (const, override));

  MOCK_METHOD(base::DictValue, GetSerpMetrics, (), (const, override));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_VIRTUAL_PREF_TEST_VIRTUAL_PREF_PROVIDER_DELEGATE_MOCK_H_
