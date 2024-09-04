/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_ENVIRONMENT_ENVIRONMENT_TYPES_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_ENVIRONMENT_ENVIRONMENT_TYPES_TEST_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::test {

std::string ToString(mojom::EnvironmentType mojom_environment_type);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_ENVIRONMENT_ENVIRONMENT_TYPES_TEST_UTIL_H_
