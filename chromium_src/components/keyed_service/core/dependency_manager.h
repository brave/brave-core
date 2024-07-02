/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_KEYED_SERVICE_CORE_DEPENDENCY_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_KEYED_SERVICE_CORE_DEPENDENCY_MANAGER_H_

#define DisallowKeyedServiceFactoryRegistration                     \
  DisallowKeyedServiceFactoryRegistration(                          \
      const std::string& registration_function_name_error_message); \
  void DisallowKeyedServiceFactoryRegistration_ChromiumImpl

#include "src/components/keyed_service/core/dependency_manager.h"  // IWYU pragma: export

#undef DisallowKeyedServiceFactoryRegistration

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_KEYED_SERVICE_CORE_DEPENDENCY_MANAGER_H_
