/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../components/password_manager/core/common/password_manager_features.cc"

#include "base/feature_override.h"

namespace password_manager {
namespace features {

ENABLE_FEATURE_BY_DEFAULT(kPasswordImport);

}  // namespace features
}  // namespace password_manager
