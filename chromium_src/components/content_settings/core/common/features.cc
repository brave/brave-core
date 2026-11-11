/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/features.h"

#include "base/feature_list.h"

#include <components/content_settings/core/common/features.cc>

namespace content_settings {

// Brave implements a strictier policy to not leak blocked permissions into
// incognito profiles. This feature (when enabled) restores the original
// Chromium implementation which makes INHERIT_IF_LESS_PERMISSIVE inherit
// blocked permissions in incognito profile.
// TODO(https://github.com/brave/brave-browser/issues/59219): Remove this
// override and the feature if we decide not to add a corresponding setting
// in brave://settings.
BASE_FEATURE(kAllowIncognitoPermissionInheritance,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace content_settings
