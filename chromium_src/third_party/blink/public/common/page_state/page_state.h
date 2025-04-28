/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_PAGE_STATE_PAGE_STATE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_PAGE_STATE_PAGE_STATE_H_

#define RemoveScrollOffset()                               \
  RemoveScrollOffset() const;                              \
  PageState PrefixTopURL(const std::string& prefix) const; \
  PageState RemoveTopURLPrefix(const size_t& prefix_length)

#include <third_party/blink/public/common/page_state/page_state.h>  // IWYU pragma: export

#undef RemoveScrollOffset

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_PAGE_STATE_PAGE_STATE_H_
