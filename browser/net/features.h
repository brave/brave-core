/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_FEATURES_H_
#define BRAVE_BROWSER_NET_FEATURES_H_

#include "base/feature_list.h"

namespace features {

// Use base::WeakPtr instead of std::shared_ptr for BraveRequestInfo.
// This reduces overhead by avoiding reference counting but requires
// careful ownership management with std::unique_ptr.
BASE_DECLARE_FEATURE(kBraveRequestInfoUseWeakPtr);

}  // namespace features

#endif  // BRAVE_BROWSER_NET_FEATURES_H_
