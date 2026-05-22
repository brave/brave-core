/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <chrome/browser/enterprise/data_protection/data_protection_features.cc>

namespace enterprise_data_protection {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kEnableForceDownloadToCloud, base::FEATURE_DISABLED_BY_DEFAULT},
    {kEnableForceDownloadToOneDrive, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace enterprise_data_protection
