/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/keyed_service/core/dependency_manager.h"

#define DisallowKeyedServiceFactoryRegistration \
  DisallowKeyedServiceFactoryRegistration_ChromiumImpl

#include "src/components/keyed_service/core/dependency_manager.cc"

#undef DisallowKeyedServiceFactoryRegistration

void DependencyManager::DisallowKeyedServiceFactoryRegistration(
    const std::string& registration_function_name_error_message) {
#if !BUILDFLAG(IS_IOS)
  DisallowKeyedServiceFactoryRegistration_ChromiumImpl(
      registration_function_name_error_message);
#endif
}
