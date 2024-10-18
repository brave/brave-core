/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"

namespace blink {

namespace {

// These template specialisations are required because not all `T`s passed to
// `IsOnion` have a `Host` method that returns a `String`. Recently,
// `KURL.Host()` has been converted to return a `StringView`, which doesn't
// offer an `EndsWith` method.
template <typename T>
String GetHost(const T& obj) {
  return obj.Host();
}

template <>
String GetHost(const KURL& obj) {
  return obj.Host().ToString();
}

template <typename T>
bool IsOnion(const T& obj) {
  constexpr const char kOnion[] = ".onion";
  return GetHost(obj).EndsWith(kOnion) &&
         (obj.Protocol() == url::kHttpsScheme ||
          obj.Protocol() == url::kHttpScheme ||
          obj.Protocol() == url::kWsScheme ||
          obj.Protocol() == url::kWssScheme);
}

}  // namespace
}  // namespace blink

#define BRAVE_MIXED_CONTENT_CHECKER_IS_MIXED_CONTENT              \
  if (auto result = IsMixedContentForOnion(security_origin, url)) \
    return *result;

#define UpgradeInsecureRequest UpgradeInsecureRequest_ChromiumImpl

#include "src/third_party/blink/renderer/core/loader/mixed_content_checker.cc"

#undef UpgradeInsecureRequest

#undef BRAVE_MIXED_CONTENT_CHECKER_IS_MIXED_CONTENT

namespace blink {
// static
std::optional<bool> MixedContentChecker::IsMixedContentForOnion(
    const SecurityOrigin* security_origin,
    const KURL& resource_url) {
  if (!IsOnion(*security_origin)) {
    return std::nullopt;
  }
  if (IsOnion(resource_url)) {
    // onion -> onion: not blocked
    return false;
  }
  // Treat .onions as https://
  // onion -> https: not blocked
  // onion -> http: blocked
  return IsMixedContent(url::kHttpsScheme, resource_url);
}

void MixedContentChecker::UpgradeInsecureRequest(
    ResourceRequest& resource_request,
    const FetchClientSettingsObject* fetch_client_settings_object,
    ExecutionContext* execution_context_for_logging,
    mojom::RequestContextFrameType frame_type,
    WebContentSettingsClient* settings_client) {
  // Do not upgrade resource from .onion sites because we treat them as secure.
  if (IsOnion(resource_request.Url()))
    return;
  UpgradeInsecureRequest_ChromiumImpl(
      resource_request, fetch_client_settings_object,
      execution_context_for_logging, frame_type, settings_client);
}

}  // namespace blink
