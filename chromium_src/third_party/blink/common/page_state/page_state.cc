/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"

#include <third_party/blink/common/page_state/page_state.cc>

namespace blink {

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

PageState PageState::RemoveTopURLPrefix(const size_t& prefix_length) const {
  if (data_.empty()) {
    return *this;
  }

  ExplodedPageState state;
  if (!DecodePageState(data_, &state)) {
    return *this;
  }

  state.top.url_string = state.top.url_string->substr(prefix_length);

  return ToPageState(state);
}

}  // namespace blink
