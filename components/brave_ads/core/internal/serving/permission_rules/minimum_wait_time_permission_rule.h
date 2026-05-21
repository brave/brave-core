/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_MINIMUM_WAIT_TIME_PERMISSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_MINIMUM_WAIT_TIME_PERMISSION_RULE_H_

#include "base/containers/span.h"
#include "base/time/time.h"

namespace brave_ads {

bool HasMinimumWaitTimePermission(base::span<const base::Time> history,
                                  base::TimeDelta time_constraint);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_MINIMUM_WAIT_TIME_PERMISSION_RULE_H_
