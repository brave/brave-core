/* Copyright (c) 2017 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_RESOURCE_BUNDLE_HELPER_H_
#define BRAVE_COMMON_RESOURCE_BUNDLE_HELPER_H_

namespace brave {
void InitializeResourceBundle();
bool SubprocessNeedsResourceBundle();
void InitializeBlockedThemedLottieImages();
}

#endif  // BRAVE_COMMON_RESOURCE_BUNDLE_HELPER_H_
