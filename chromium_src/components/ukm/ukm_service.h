/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_UKM_UKM_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_UKM_UKM_SERVICE_H_

#include "base/gtest_prod_util.h"

class ChromeMetricsServiceClientTest;
class BraveTestRegisterUKMProviders;

FORWARD_DECLARE_TEST(ChromeMetricsServiceClientTest,
                     BraveTestRegisterUKMProviders);

#define cloned_install_subscription_                         \
  cloned_install_subscription_;                              \
  FRIEND_TEST_ALL_PREFIXES(::ChromeMetricsServiceClientTest, \
                           BraveTestRegisterUKMProviders)
#include <components/ukm/ukm_service.h>  // IWYU pragma: export
#undef cloned_install_subscription_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_UKM_UKM_SERVICE_H_
