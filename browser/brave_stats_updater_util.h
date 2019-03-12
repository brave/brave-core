/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_UPDATER_UTIL_H_
#define BRAVE_BROWSER_BRAVE_STATS_UPDATER_UTIL_H_

#include <string>

#include "base/time/time.h"

namespace brave {

std::string GetDateAsYMD(const base::Time& time);


std::string GetChannelName();

std::string GetPlatformIdentifier();

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_STATS_UPDATER_UTIL_H_
