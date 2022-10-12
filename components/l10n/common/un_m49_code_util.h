/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_UN_M49_CODE_UTIL_H_
#define BRAVE_COMPONENTS_L10N_COMMON_UN_M49_CODE_UTIL_H_

#include "base/strings/string_piece_forward.h"

namespace brave_l10n {

// Returns |true| if the given code is a UN M.49 code otherwise returns |false|.
// See https://en.wikipedia.org/wiki/UN_M49.
bool IsUNM49Code(base::StringPiece code);

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_UN_M49_CODE_UTIL_H_
