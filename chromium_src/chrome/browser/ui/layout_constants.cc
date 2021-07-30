/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetLayoutConstant GetLayoutConstant_ChromiumImpl
#include "../../../../../chrome/browser/ui/layout_constants.cc"
#undef GetLayoutConstant

#include "brave/browser/ui/brave_layout_constants.h"

int GetLayoutConstant(LayoutConstant constant) {
  // get brave overriden value
  const absl::optional<int> braveOption = GetBraveLayoutConstant(constant);
  if (braveOption) {
    return braveOption.value();
  }
  // get chromium value
  return GetLayoutConstant_ChromiumImpl(constant);
}
