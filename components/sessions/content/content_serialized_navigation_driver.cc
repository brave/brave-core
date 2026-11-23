// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/common/url_constants.h"
#include "third_party/blink/public/common/page_state/page_state.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace sessions {

namespace {

void RestoreBraveSchemeAsChromeScheme(SerializedNavigationEntry& navigation) {
  const GURL& virtual_url = navigation.virtual_url();
  if (virtual_url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    navigation.set_virtual_url(virtual_url.ReplaceComponents(replacements));
  }
}

#if BUILDFLAG(ENABLE_CONTAINERS)
// Adds the container's virtual URL prefix to the PageState of a container tab,
// so that a session restored with Containers disabled can't navigate to it.
std::string MaybePrefixContainerPageState(
    const SerializedNavigationEntry& navigation,
    std::string page_state) {
  if (base::FeatureList::IsEnabled(containers::features::kContainers) &&
      !navigation.virtual_url_prefix().empty() && !page_state.empty()) {
    page_state = blink::PageState::CreateFromEncodedData(page_state)
                     .PrefixTopURL(navigation.virtual_url_prefix())
                     .ToEncodedData();
  }
  return page_state;
}

// Parses a "containers+<uuid>:<url>" virtual URL loaded from disk or sync into
// its storage partition key, stripping the prefix from the virtual URL and the
// PageState. With Containers disabled the prefix stays, so the URL remains
// unhandleable.
void MaybeRestoreContainerStoragePartitionKey(
    SerializedNavigationEntry& navigation) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return;
  }
  auto result =
      containers::RestoreStoragePartitionKeyFromUrl(navigation.virtual_url());
  if (!result) {
    return;
  }
  navigation.set_virtual_url_prefix(
      navigation.virtual_url().spec().substr(0, result->url_prefix_length));
  navigation.set_virtual_url(result->url);
  navigation.set_storage_partition_key(result->storage_partition_key);
  if (!navigation.encoded_page_state().empty()) {
    navigation.set_encoded_page_state(
        blink::PageState::CreateFromEncodedData(navigation.encoded_page_state())
            .RemoveTopURLPrefix(result->url_prefix_length)
            .ToEncodedData());
  }
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

}  // namespace

// The functions below are forward-declared and called by the chromium_src
// content_serialized_navigation_driver.cc override.

// Called when saving a session, at the end of
// ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle(), right
// before the PageState is written to disk. `navigation` has already been
// populated by ContentSerializedNavigationBuilder::FromNavigationEntry() (see
// BraveSerializeNavigationEntry()), and `page_state` is upstream's sanitized
// PageState. Returns the PageState to persist.
std::string BraveSanitizePageStateForPickle(
    [[maybe_unused]] const SerializedNavigationEntry& navigation,
    std::string page_state) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  page_state = MaybePrefixContainerPageState(navigation, std::move(page_state));
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return page_state;
}

// Called when restoring a session, at the start of
// ContentSerializedNavigationDriver::Sanitize(), right after `navigation` has
// been read from disk or sync and before it is converted into a
// NavigationEntry by ContentSerializedNavigationBuilder::ToNavigationEntry()
// (see BraveRestoreNavigationEntry()).
void BraveSanitizeNavigation(SerializedNavigationEntry& navigation) {
  RestoreBraveSchemeAsChromeScheme(navigation);
#if BUILDFLAG(ENABLE_CONTAINERS)
  MaybeRestoreContainerStoragePartitionKey(navigation);
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

}  // namespace sessions
