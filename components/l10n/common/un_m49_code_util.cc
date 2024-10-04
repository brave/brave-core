/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/un_m49_code_util.h"

#include <string_view>

#include "base/containers/contains.h"
#include "brave/components/l10n/common/un_m49_code_constants.h"

namespace brave_l10n {

bool IsUNM49Code(std::string_view code) {
  return base::Contains(kUnM49Codes, code);
}

}  // namespace brave_l10n
