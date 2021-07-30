/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_LAYOUT_CONSTANTS_H_
#define BRAVE_BROWSER_UI_BRAVE_LAYOUT_CONSTANTS_H_

#include "chrome/browser/ui/layout_constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

absl::optional<int> GetBraveLayoutConstant(LayoutConstant constant);

#endif  // BRAVE_BROWSER_UI_BRAVE_LAYOUT_CONSTANTS_H_
