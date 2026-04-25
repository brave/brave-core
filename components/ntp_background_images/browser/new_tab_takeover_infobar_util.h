/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NEW_TAB_TAKEOVER_INFOBAR_UTIL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NEW_TAB_TAKEOVER_INFOBAR_UTIL_H_

class PrefService;

namespace ntp_background_images {

// TODO(https://github.com/brave/brave-browser/issues/45926): Decouple New Tab
// Takeover infobar from ntp_background_images component.

bool ShouldDisplayNewTabTakeoverInfobar(const PrefService* prefs);

void RecordNewTabTakeoverInfobarWasDisplayed(PrefService* prefs);

void SuppressNewTabTakeoverInfobar(PrefService* prefs);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NEW_TAB_TAKEOVER_INFOBAR_UTIL_H_
