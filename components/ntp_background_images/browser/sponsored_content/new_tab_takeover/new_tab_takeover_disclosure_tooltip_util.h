/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_UTIL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_UTIL_H_

class PrefService;

namespace ntp_background_images {

bool ShouldAutoShowNewTabTakeoverDisclosureTooltip(const PrefService* prefs);

void RecordNewTabTakeoverDisclosureTooltipWasDisplayed(PrefService* prefs);

void SuppressNewTabTakeoverDisclosureTooltip(PrefService* prefs);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_UTIL_H_
