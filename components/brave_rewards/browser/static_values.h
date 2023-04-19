/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_STATIC_VALUES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_STATIC_VALUES_H_

#include <stdint.h>

#include <map>
#include <set>
#include <string>

#include "base/time/time.h"
#include "build/build_config.h"

namespace brave_rewards {

const std::set<std::string> kBitflyerCountries = {
    "JP"  // ID: 19024
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_STATIC_VALUES_H_
