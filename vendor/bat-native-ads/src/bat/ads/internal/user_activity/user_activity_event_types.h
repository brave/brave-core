/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_EVENT_TYPES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_EVENT_TYPES_H_

#include <cstdint>

namespace ads {

// WARNING: don't change these numbers. They are provided by the variations
// service, so will need the same values to match the enums

enum class UserActivityEventType : int8_t {
  /* 00 */ kInitializedAds = 0,
  /* 01 */ kBrowserDidBecomeActive,
  /* 02 */ kBrowserDidEnterBackground,
  /* 03 */ kClickedBackOrForwardNavigationButtons,
  /* 04 */ kClickedBookmark,
  /* 05 */ kClickedHomePageButton,
  /* 06 */ kClickedLink,
  /* 07 */ kClickedReloadButton,
  /* 08 */ kClosedTab,
  /* 09 */ kFocusedOnExistingTab,
  /* 0A */ kGeneratedKeyword,
  /* 0B */ kNewNavigation,
  /* 0C */ kOpenedLinkFromExternalApplication,
  /* 0D */ kOpenedNewTab,
  /* 0E */ kPlayedMedia,
  /* 0F */ kStoppedPlayingMedia,
  /* 10 */ kSubmittedForm,
  /* 11 */ kTabUpdated,
  /* 12 */ kTypedAndSelectedNonUrl,
  /* 13 */ kTypedKeywordOtherThanDefaultSearchProvider,
  /* 14 */ kTypedUrl,
  /* 15 */ kUsedAddressBar,
  /* 16 */ kBrowserWindowIsActive,
  /* 17 */ kBrowserWindowIsInactive
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_EVENT_TYPES_H_
