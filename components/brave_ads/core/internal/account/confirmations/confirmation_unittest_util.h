/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UNITTEST_UTIL_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct ConfirmationInfo;

// TODO(https://github.com/brave/brave-browser/issues/25205): Deprecate and
// instead use a mocked |TokenGenerator| and |CreateConfirmation|.
absl::optional<ConfirmationInfo> BuildConfirmation();

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UNITTEST_UTIL_H_
