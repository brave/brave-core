/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_PREFS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_PREFS_UTIL_H_

#include <string>

#include "base/values.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {
namespace prefs {

const base::Value* GetValue(PrefService* prefs, const std::string& path);

}  // namespace prefs
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_PREFS_UTIL_H_
