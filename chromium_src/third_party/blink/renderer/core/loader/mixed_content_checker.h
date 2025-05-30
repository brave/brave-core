/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MIXED_CONTENT_CHECKER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MIXED_CONTENT_CHECKER_H_

#include <optional>

// We are hiding IsMixedContent(const String& origin_protocol, const KURL&)
// because we want to enforce mixed content checks on .onion origins.
// Publicly available protocol-only overload of this method allows to skip
// .onion specific checks which we don't want.

#define UpgradeInsecureRequest                                             \
  NotUsed();                                                               \
                                                                           \
 private:                                                                  \
  static bool IsMixedContent(const String& origin_protocol, const KURL&);  \
  static std::optional<bool> IsMixedContentForOnion(const SecurityOrigin*, \
                                                    const KURL& url);      \
                                                                           \
 public:                                                                   \
  static void UpgradeInsecureRequest(                                      \
      ResourceRequest&,                                                    \
      const FetchClientSettingsObject* fetch_client_settings_object,       \
      ExecutionContext* execution_context_for_logging,                     \
      mojom::RequestContextFrameType,                                      \
      WebContentSettingsClient* settings_client, LocalFrame* frame);       \
  static void UpgradeInsecureRequest_ChromiumImpl

#include "src/third_party/blink/renderer/core/loader/mixed_content_checker.h"  // IWYU pragma: export

#undef UpgradeInsecureRequest

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MIXED_CONTENT_CHECKER_H_
