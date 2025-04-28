/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sessions/content/content_serialized_navigation_driver.h"

#include <string>

#include "base/containers/fixed_flat_set.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/common/url_constants.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#define GetSanitizedPageStateForPickle \
  GetSanitizedPageStateForPickle_ChromiumImpl
#define Sanitize Sanitize_ChromiumImpl

#include <components/sessions/content/content_serialized_navigation_driver.cc>

#undef GetSanitizedPageStateForPickle
#undef Sanitize

namespace sessions {

namespace {

// Extension can override below three chrome urls.
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/common/extensions/api/chrome_url_overrides.idl
constexpr auto kAllowedChromeUrlsOverridingHostList =
    base::MakeFixedFlatSet<std::string_view>(
        {"newtab", "history", "bookmarks"});

}  // namespace

std::string ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle(
    const sessions::SerializedNavigationEntry* navigation) const {
  const auto& virtual_url = navigation->virtual_url();
  if (virtual_url.SchemeIs(content::kChromeUIScheme)) {
    // If empty string is returned, chrome url overriding is ignored.
    if (kAllowedChromeUrlsOverridingHostList.contains(virtual_url.host())) {
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

  std::string page_state =
      GetSanitizedPageStateForPickle_ChromiumImpl(navigation);
#if BUILDFLAG(ENABLE_CONTAINERS)
  std::string scheme_prefix;
  if (auto restored_virtual_url =
          containers::ContainedTabHandlerRegistry::GetInstance()
              .RestoreNavigationVirtualUrl(navigation->virtual_url(),
                                           scheme_prefix)) {
    blink::PageState page_state_obj =
        blink::PageState::CreateFromEncodedData(page_state);
    page_state = page_state_obj.PrefixTopURL(scheme_prefix).ToEncodedData();
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  return page_state;
}

void ContentSerializedNavigationDriver::Sanitize(
    SerializedNavigationEntry* navigation) const {
  Sanitize_ChromiumImpl(navigation);

#if BUILDFLAG(ENABLE_CONTAINERS)
  std::string scheme_prefix;
  if (auto restored_virtual_url =
          containers::ContainedTabHandlerRegistry::GetInstance()
              .RestoreNavigationVirtualUrl(navigation->virtual_url(),
                                           scheme_prefix)) {
    navigation->set_virtual_url(*restored_virtual_url);
    blink::PageState page_state_obj = blink::PageState::CreateFromEncodedData(
        navigation->encoded_page_state());
    navigation->set_encoded_page_state(
        page_state_obj.RemoveTopURLPrefix(scheme_prefix).ToEncodedData());
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

}  // namespace sessions
