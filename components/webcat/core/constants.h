/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_CONSTANTS_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_CONSTANTS_H_

#include <string_view>

namespace webcat {

inline constexpr std::string_view kWebcatEnsTextRecordKey = "webcat";
inline constexpr std::string_view kWebcatUdRecordKey = "dweb.webcat";
inline constexpr std::string_view kDefaultIpfsGatewayHost = "ipfs.inbrowser.link";
inline constexpr int kDefaultIpfsFetchTimeoutSeconds = 10;
inline constexpr int kDefaultCacheTtlSeconds = 3600;
inline constexpr size_t kDefaultMaxCacheSize = 100;
inline constexpr size_t kMaxBundleSizeBytes = 1024 * 1024;

enum class OriginState {
  kUnverified,
  kBundleFetched,
  kVerified,
  kFailed,
};

enum class WebcatError {
  kNone,
  kBundleNotFound,
  kBundleTooLarge,
  kBundleParseError,
  kCidIntegrityFailed,
  kSignatureVerificationFailed,
  kManifestStructureInvalid,
  kCspValidationFailed,
  kContentIntegrityFailed,
  kManifestExpired,
};

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_CONSTANTS_H_
