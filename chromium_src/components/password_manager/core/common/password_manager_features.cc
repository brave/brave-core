/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/password_manager/core/common/password_manager_features.cc"

#include "base/feature_override.h"

namespace password_manager {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{{kPasswordImport,
                                   base::FEATURE_ENABLED_BY_DEFAULT}}});

}  // namespace features
}  // namespace password_manager
