/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/sharing_hub/sharing_hub_bubble_controller_desktop_impl.h"

#define ShouldOfferOmniboxIcon ShouldOfferOmniboxIcon_ChromiumImpl

#include "src/chrome/browser/ui/sharing_hub/sharing_hub_bubble_controller_desktop_impl.cc"

#undef ShouldOfferOmniboxIcon

namespace sharing_hub {

bool SharingHubBubbleControllerDesktopImpl::ShouldOfferOmniboxIcon() {
  const GURL url = GetWebContents().GetLastCommittedURL();

  // To disable share icons in internal pages.
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  return ShouldOfferOmniboxIcon_ChromiumImpl();
}

}  // namespace sharing_hub
