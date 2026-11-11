// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/search_engines/features.h"

namespace brave::features {

// Whether or not search engines are managed by Chromium's
// `TemplateURLService` instead of the legacy Swift implementation.
BASE_FEATURE(kUseChromiumSearchEngines, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave::features
