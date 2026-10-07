/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/public/common/content_features.cc>

namespace features {

// This is intended as a kill switch for the Idle Detection feature. To enable
// this feature, the experimental web platform features flag should be set,
// or the site should obtain an Origin Trial token.
BASE_FEATURE(kIdleDetection, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
