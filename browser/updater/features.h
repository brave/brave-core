/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UPDATER_FEATURES_H_
#define BRAVE_BROWSER_UPDATER_FEATURES_H_

#include <optional>

#include "base/feature_list.h"
#include "base/time/time.h"

namespace brave_updater {

BASE_DECLARE_FEATURE(kBraveUseOmaha4Alpha);

bool ShouldUseOmaha4();

// For tests:
bool ShouldUseOmaha4(base::Time now, std::optional<bool>& state);

}  // namespace brave_updater

#endif  // BRAVE_BROWSER_UPDATER_FEATURES_H_
