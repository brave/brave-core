/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_MOCK_RESOURCE_COMPONENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_MOCK_RESOURCE_COMPONENT_H_

#include <string>

#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_ads/browser/test/fake_component_updater_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads::test {

class MockResourceComponent : public ResourceComponent {
 public:
  MockResourceComponent();

  MockResourceComponent(const MockResourceComponent&) = delete;
  MockResourceComponent& operator=(const MockResourceComponent&) = delete;

  ~MockResourceComponent() override;

  MOCK_METHOD(void,
              RegisterCountryComponent,
              (const std::string& country_code),
              (override));
  MOCK_METHOD(void, UnregisterCountryComponent, (), (override));
  MOCK_METHOD(void,
              RegisterLanguageComponent,
              (const std::string& language_code),
              (override));
  MOCK_METHOD(void, UnregisterLanguageComponent, (), (override));

 private:
  FakeComponentUpdaterDelegate fake_component_updater_delegate_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_MOCK_RESOURCE_COMPONENT_H_
