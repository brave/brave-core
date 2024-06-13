/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_HASH_DETECTION_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_HASH_DETECTION_H_

#include <string>

#include "brave/components/web_discovery/browser/regex_util.h"

namespace web_discovery {

bool IsHashLikely(RegexUtil& regex_util,
                  std::string value,
                  double probability_multiplier = 1.0);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_HASH_DETECTION_H_
