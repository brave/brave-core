/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_H_

// BraveSetTargetHeight() sets the target height and computed height w/o
// triggering a layout pass.
#define SetTargetHeight             \
  BraveSetTargetHeight(int height); \
  void SetTargetHeight

#include "src/components/infobars/core/infobar.h"  // IWYU pragma: export

#undef SetTargetHeight

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_H_
