/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SHARING_HUB_SHARING_HUB_BUBBLE_CONTROLLER_DESKTOP_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SHARING_HUB_SHARING_HUB_BUBBLE_CONTROLLER_DESKTOP_IMPL_H_

// This header must be included before the macro definition, so the virtual
// declaration of "ShouldOfferOmniboxIcon" will not be changed using the
// property of header guard.
#include "chrome/browser/ui/sharing_hub/sharing_hub_bubble_controller.h"

#define ShouldOfferOmniboxIcon           \
  ShouldOfferOmniboxIcon_ChromiumImpl(); \
  bool ShouldOfferOmniboxIcon

#include "src/chrome/browser/ui/sharing_hub/sharing_hub_bubble_controller_desktop_impl.h"  // IWYU pragma: export

#undef ShouldOfferOmniboxIcon

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SHARING_HUB_SHARING_HUB_BUBBLE_CONTROLLER_DESKTOP_IMPL_H_
