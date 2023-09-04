/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/containers/contains.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/common/url_constants.h"

#define GetSanitizedPageStateForPickle \
  GetSanitizedPageStateForPickle_ChromiumImpl
#include "src/components/sessions/content/content_serialized_navigation_driver.cc"
#undef GetSanitizedPageStateForPickle

namespace sessions {

std::string ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle(
    const sessions::SerializedNavigationEntry* navigation) const {
  // Extension can override below three chrome urls.
  // https://source.chromium.org/chromium/chromium/src/+/main:chrome/common/extensions/api/chrome_url_overrides.idl
  constexpr std::array<const char*, 3> kAllowedChromeUrlsOverridingHostList = {
      "newtab", "history", "bookmarks"};

  const auto& virtual_url = navigation->virtual_url();
  if (virtual_url.SchemeIs(content::kChromeUIScheme)) {
    // If empty string is returned, chrome url overriding is ignored.
    if (base::Contains(kAllowedChromeUrlsOverridingHostList,
                       virtual_url.host())) {
      // chrome url can be re-written when it's restored during the tab but
      // re-written url is ignored when encoded page state is empty.
      // In ContentSerializedNavigationBuilder::ToNavigationEntry(), re-written
      // url created by NavigationEntry's ctor is ignored by creating new page
      // state with navigation's virtual_url. Sanitize all but make url info
      // persisted. Use original_request_url as it's used when NavigationEntry
      // is created.
      return blink::PageState::CreateFromURL(navigation->original_request_url())
          .ToEncodedData();
    }
    return std::string();
  }

  return GetSanitizedPageStateForPickle_ChromiumImpl(navigation);
}

}  // namespace sessions
