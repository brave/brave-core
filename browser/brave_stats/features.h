/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_FEATURES_H_
#define BRAVE_BROWSER_BRAVE_STATS_FEATURES_H_

#include "base/feature_list.h"

namespace brave_stats {
namespace features {

// See https://github.com/brave/brave-browser/issues/42111 for more info.
BASE_DECLARE_FEATURE(kHeadlessRefcode);

bool IsHeadlessClientRefcodeEnabled();

}  // namespace features
}  // namespace brave_stats

#endif  // BRAVE_BROWSER_BRAVE_STATS_FEATURES_H_
