/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sessions/content/content_serialized_navigation_driver.h"

#include <string>

#include "brave/components/containers/buildflags/buildflags.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#define GetSanitizedPageStateForPickle \
  GetSanitizedPageStateForPickle_ChromiumImpl
#define Sanitize Sanitize_ChromiumImpl

#include <components/sessions/content/content_serialized_navigation_driver.cc>

#undef Sanitize
#undef GetSanitizedPageStateForPickle

namespace sessions {

std::string ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle(
    const sessions::SerializedNavigationEntry* navigation) const {
  const auto& virtual_url = navigation->virtual_url();
  if (virtual_url.SchemeIs(content::kChromeUIScheme)) {
    // Persisting no page state (including for chrome:// hosts that can be
    // overridden by an extension, e.g. newtab/history/bookmarks) is safe:
    // ContentSerializedNavigationBuilder::ToNavigationEntry() synthesizes
    // PageState from the entry's rewritten URL when page state is empty, so
    // it always reflects whatever URL (extension override or not) the
    // virtual_url currently resolves to. Baking in a URL here instead (e.g.
    // the original chrome://newtab/ URL) can go stale relative to that
    // rewritten origin and trip
    // NavigationControllerImpl::PopulateSingleNavigationApiHistoryEntryVector's
    // origin/URL consistency DCHECK.
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

  // Restore previous saved urls with brave:// scheme as chrome://
  const auto& virtual_url = navigation->virtual_url();
  if (virtual_url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    navigation->set_virtual_url(virtual_url.ReplaceComponents(replacements));
  }

#if BUILDFLAG(ENABLE_CONTAINERS)
  // This method is called when loading a SerializedNavigationEntry from
  // disk/sync, BEFORE it's converted to a NavigationEntry. It's our opportunity
  // to detect container-encoded URLs and prepare them for restoration.
  //
  // This works for both local session restore and cross-device sync. When a
  // container tab is synced to a device with Containers disabled, the URL will
  // remain unhandleable (the prefix won't be removed).
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    // Try to parse the virtual_url as a container-encoded URL.
    // If it has the format "containers+<uuid>:https://...", this returns
    // the original URL and extracts the partition key.
    if (auto result = containers::RestoreStoragePartitionKeyFromUrl(
            navigation->virtual_url())) {
      // Extract just the prefix part for PageState manipulation.
      // For "containers+work:https://example.com", this extracts
      // "containers+work:" (everything before the original URL).
      navigation->set_virtual_url_prefix(
          navigation->virtual_url().spec().substr(0,
                                                  result->url_prefix_length));

      // Update the virtual_url to the original URL without the prefix.
      // "containers+work:https://example.com" -> "https://example.com"
      // This is what will be used to create the NavigationEntry.
      navigation->set_virtual_url(result->url);

      // Store the extracted storage partition key.
      // This will be used by ContentSerializedNavigationBuilder::
      // ToNavigationEntry() to set the correct StoragePartitionConfig
      // when creating the NavigationEntry.
      navigation->set_storage_partition_key(result->storage_partition_key);

      // Remove the prefix from PageState too.
      if (!navigation->encoded_page_state().empty()) {
        blink::PageState page_state_obj =
            blink::PageState::CreateFromEncodedData(
                navigation->encoded_page_state());
        navigation->set_encoded_page_state(
            page_state_obj.RemoveTopURLPrefix(result->url_prefix_length)
                .ToEncodedData());
      }
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

}  // namespace sessions
