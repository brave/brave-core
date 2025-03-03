/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_UTIL_H_

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

class GURL;

namespace brave_ads {

base::Value::Dict BuildReminder(mojom::ReminderType mojom_reminder_type);

[[nodiscard]] bool IsReminder(const std::string& placement_id);

GURL GetReminderTargetUrl();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_UTIL_H_
