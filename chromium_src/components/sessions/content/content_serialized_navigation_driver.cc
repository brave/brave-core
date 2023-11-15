/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
  // Sanitize all brave:// pages expect for original request URL info persisted.
  // We cannot return an empty string directly because upstream will create a
  // new page state using virtual_url if encoded_page_state is empty during
  // session restore, in which case virtual URL (brave://*) will be the URL to
  // load when restoring sessions rather than the original request URL
  // (chrome://*). This is also needed for supporting chrome URL override done
  // by extensions during session restore, because the overriden URL will be
  // ignored if encoded_page_state is empty.
  const auto& virtual_url = navigation->virtual_url();
  if (virtual_url.SchemeIs(content::kBraveUIScheme)) {
    return blink::PageState::CreateFromURL(navigation->original_request_url())
        .ToEncodedData();
  }

  return GetSanitizedPageStateForPickle_ChromiumImpl(navigation);
}

}  // namespace sessions
