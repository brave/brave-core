/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"
#include "brave/components/containers/buildflags/buildflags.h"

#include <third_party/blink/common/page_state/page_state.cc>

namespace blink {

#if BUILDFLAG(ENABLE_CONTAINERS)
// Prepends a prefix to the top-level URL stored in PageState.
//
// This is used by Brave Containers to encode StoragePartitionConfig information
// into the serialized page state. The virtual URL prefix (e.g.,
// "containers+550e8400-e29b-41d4-a716-446655440000:") is prepended to the
// actual URL during session serialization (when saving the session).
//
// This is critical for preventing navigation when the Containers feature is
// disabled on session restore: PageState is Blink's internal serialized state,
// and Blink will use the URL stored in PageState for restoration. By prefixing
// the URL in PageState during save, we ensure that if the session is later
// restored with the feature disabled, Blink will attempt to navigate to the
// unhandleable "containers+uuid:" scheme, resulting in a blank page rather than
// incorrectly loading the site in the default storage partition.
//
// Returns a new PageState with the modified URL, or the original PageState if
// the operation fails or the PageState is empty.
PageState PageState::PrefixTopURL(const std::string& prefix) const {
  if (data_.empty()) {
    return *this;
  }

  ExplodedPageState state;
  if (!DecodePageState(data_, &state)) {
    return *this;
  }

  if (!state.top.url_string) {
    return *this;
  }

  state.top.url_string =
      base::StrCat({base::UTF8ToUTF16(prefix), *state.top.url_string});

  return ToPageState(state);
}

// Removes a prefix of specified length from the top-level URL in PageState.
//
// This is the inverse of PrefixTopURL().
//
// Called during session restore after the StoragePartitionConfig has been
// extracted from the virtual URL and validated. When the Containers feature is
// enabled, this method removes the safety prefix so Blink sees the correct,
// user-visible URL (e.g., "https://example.com") and can properly render the
// page with the correct storage partition. If the feature is disabled, this
// method is not called, leaving the unhandleable "containers+uuid:" URL in
// PageState, which results in a blank page and prevents data leakage by
// avoiding storage partition mixing.
//
// Returns a new PageState with the prefix removed, or the original PageState if
// the operation fails or the PageState is empty.
PageState PageState::RemoveTopURLPrefix(size_t prefix_length) const {
  if (data_.empty()) {
    return *this;
  }

  ExplodedPageState state;
  if (!DecodePageState(data_, &state)) {
    return *this;
  }

  if (!state.top.url_string) {
    return *this;
  }

  if (prefix_length > state.top.url_string->size()) {
    return *this;
  }

  state.top.url_string = state.top.url_string->substr(prefix_length);

  return ToPageState(state);
}
#endif

}  // namespace blink
