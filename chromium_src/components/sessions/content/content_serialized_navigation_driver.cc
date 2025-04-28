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
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/core/common/features.h"
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
  // If this is a container tab, add the virtual URL prefix to the PageState.
  // This ensures that if the session is restored with Containers disabled, the
  // browser won't be able to navigate to the URL (the scheme is invalid).
  if (base::FeatureList::IsEnabled(containers::features::kContainers) &&
      !navigation->virtual_url_prefix().empty() && !page_state.empty()) {
    blink::PageState page_state_obj =
        blink::PageState::CreateFromEncodedData(page_state);
    page_state = page_state_obj.PrefixTopURL(navigation->virtual_url_prefix())
                     .ToEncodedData();
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return page_state;
}

void ContentSerializedNavigationDriver::Sanitize(
    SerializedNavigationEntry* navigation) const {
  Sanitize_ChromiumImpl(navigation);

#if BUILDFLAG(ENABLE_CONTAINERS)
  // This method is called when loading a SerializedNavigationEntry from
  // disk/sync, BEFORE it's converted to a NavigationEntry. It's our opportunity
  // to detect container-encoded URLs and prepare them for restoration.
  //
  // This works for both local session restore and cross-device sync. When a
  // container tab is synced to a device with Containers disabled, the URL will
  // remain unhandleable (the prefix won't be removed).
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    std::pair<std::string, std::string> storage_partition_key;
    size_t url_prefix_length;

    // Try to parse the virtual_url as a container-encoded URL.
    // If it has the format "containers+<uuid>:https://...", this returns
    // the original URL and extracts the partition key.
    if (auto restored_virtual_url =
            containers::RestoreStoragePartitionKeyFromUrl(
                navigation->virtual_url(), storage_partition_key,
                url_prefix_length)) {
      // Extract just the prefix part for PageState manipulation.
      // For "containers+work:https://example.com", this extracts
      // "containers+work:" (everything before the original URL).
      navigation->set_virtual_url_prefix(
          navigation->virtual_url().spec().substr(0, url_prefix_length));

      // Update the virtual_url to the original URL without the prefix.
      // "containers+work:https://example.com" -> "https://example.com"
      // This is what will be used to create the NavigationEntry.
      navigation->set_virtual_url(*restored_virtual_url);

      // Store the extracted storage partition key.
      // This will be used by ContentSerializedNavigationBuilder::
      // ToNavigationEntry() to set the correct StoragePartitionConfig
      // when creating the NavigationEntry.
      navigation->set_storage_partition_key(storage_partition_key);

      // Remove the prefix from PageState too.
      if (!navigation->encoded_page_state().empty()) {
        blink::PageState page_state_obj =
            blink::PageState::CreateFromEncodedData(
                navigation->encoded_page_state());
        navigation->set_encoded_page_state(
            page_state_obj.RemoveTopURLPrefix(url_prefix_length)
                .ToEncodedData());
      }
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

}  // namespace sessions
