/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TEST_BASE_COMPONENTS_UNIT_TEST_H_
#define BRAVE_COMPONENTS_TEST_BASE_COMPONENTS_UNIT_TEST_H_

#include <memory>

namespace content {
class ScopedWebUIControllerFactoryRegistration;
}

// Workaround for components running inside chrome unit test suit. This class
// unregisters problematic factories like ChromeWebUIControllerFactory
class ComponentsUnitTest {
 public:
  ComponentsUnitTest();
  ComponentsUnitTest(const ComponentsUnitTest&) = delete;
  ComponentsUnitTest& operator=(const ComponentsUnitTest&) = delete;
  virtual ~ComponentsUnitTest();

  void SetUp();

 private:
  class StubWebUIWebUIControllerFactory;
  std::unique_ptr<StubWebUIWebUIControllerFactory> factory_;
  std::unique_ptr<content::ScopedWebUIControllerFactoryRegistration>
      factory_registration_;
};

#endif  // BRAVE_COMPONENTS_TEST_BASE_COMPONENTS_UNIT_TEST_H_
