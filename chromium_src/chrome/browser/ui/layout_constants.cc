/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/browser/ui/brave_layout_constants.h"

// Forward declaration
int GetLayoutConstant_ChromiumImpl(LayoutConstant constant);

#define LayoutConstant LayoutConstant constant) {                            \
    const std::optional<int> braveOption = GetBraveLayoutConstant(constant); \
    if (braveOption) {                                                       \
      return braveOption.value();                                            \
    }                                                                        \
                                                                             \
    return GetLayoutConstant_ChromiumImpl(constant);                         \
  }                                                                          \
                                                                             \
  int GetLayoutConstant_ChromiumImpl(LayoutConstant

#include "src/chrome/browser/ui/layout_constants.cc"
#undef LayoutConstant
