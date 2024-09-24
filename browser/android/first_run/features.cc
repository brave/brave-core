/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/first_run/features.h"

#include "base/feature_list.h"

namespace first_run {
namespace features {

BASE_FEATURE(kAndroidForceDefaultBrowserPrompt,
             "AndroidForceDefaultBrowserPrompt",
             base::FEATURE_DISABLED_BY_DEFAULT);

}
}  // namespace first_run
