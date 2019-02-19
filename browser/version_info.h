/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_VERSION_INFO_VALUES_H_
#define BRAVE_BROWSER_VERSION_INFO_VALUES_H_

#include <string>

#define CHROMIUM_CR71 strcmp(BRAVE_BROWSER_VERSION, "71.0.3578.99")
#define CHROMIUM_CR72 strcmp(BRAVE_BROWSER_VERSION, "72.0.3626.96")

#if $CHROMIUM_CR72 == 0
  #define TRIM_PUBLISHERS_DB_MEMORY db_.TrimMemory(memory_pressure_level == base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
  #define BASE_TIME_KSECONDSPERHOUR base::Time::kSecondsPerHour
  #define URL_DECODE_URL_MODE
#elif $CHROMIUM_CR71 == 0
  #define MESSAGE_NOTIFIER_TYPE message_center::NotifierId::NotifierType::SYSTEM_COMPONENT
  #define BASE_TIME_KSECONDSPERHOUR 60 * 60
#else
  #define MESSAGE_NOTIFIER_TYPE message_center::NotifierType::SYSTEM_COMPONENT
  #define BASE_TIME_KSECONDSPERHOUR base::Time::kSecondsPerHour
  #define TRIM_PUBLISHERS_DB_MEMORY db_.TrimMemory();
  #define URL_DECODE_URL_MODE url::DecodeURLMode::kUTF8OrIsomorphic,
#endif

namespace version_info {
std::string GetBraveVersionWithoutChromiumMajorVersion();
std::string GetBraveVersionNumberForDisplay();
}  // namespace version_info

#endif  // BRAVE_BROWSER_VERSION_INFO_VALUES_H_
