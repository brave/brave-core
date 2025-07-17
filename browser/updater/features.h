/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UPDATER_FEATURES_H_
#define BRAVE_BROWSER_UPDATER_FEATURES_H_

#include "base/feature_list.h"

namespace brave_updater {

BASE_DECLARE_FEATURE(kBraveUseOmaha4Alpha);

bool ShouldUseOmaha4();

}  // namespace brave_updater

#endif  // BRAVE_BROWSER_UPDATER_FEATURES_H_
