/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MIXED_CONTENT_CHECKER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MIXED_CONTENT_CHECKER_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

// We are hiding IsMixedContent(const String& origin_protocol, const KURL&)
// because we want to enforce mixed content checks on .onion origins.
// Publically available protocol-only overload of this method allows to skip
// .onion specific checks which we don't want.

#define UpgradeInsecureRequest                                              \
  NotUsed();                                                                \
                                                                            \
 private:                                                                   \
  static bool IsMixedContent(const String& origin_protocol, const KURL&);   \
  static absl::optional<bool> IsMixedContentForOnion(const SecurityOrigin*, \
                                                     const KURL& url);      \
                                                                            \
 public:                                                                    \
  static void UpgradeInsecureRequest

#include "src/third_party/blink/renderer/core/loader/mixed_content_checker.h"

#undef UpgradeInsecureRequest

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MIXED_CONTENT_CHECKER_H_
