/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

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
  if (navigation->virtual_url().SchemeIs(content::kChromeUIScheme))
    return std::string();
  return GetSanitizedPageStateForPickle_ChromiumImpl(navigation);
}

}  // namespace sessions
