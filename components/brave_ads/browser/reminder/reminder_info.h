/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_INFO_H_

#include <string>

namespace brave_ads {

struct ReminderInfo {
  std::string placement_id;
  std::u16string title;
  std::u16string body;
  std::string target_url;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_INFO_H_
