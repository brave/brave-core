/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/user_education/views/help_bubble_view.h"

// MdIPHBubbleButton sets its text color but it's overwritten by
// MdTextButton::UpdateTextColor(). Provide empty override method
// to prevent it.
#define UpdateBackgroundColor   \
  UpdateTextColor() override {} \
  void UpdateBackgroundColor

#include "src/components/user_education/views/help_bubble_view.cc"

#undef UpdateBackgroundColor
