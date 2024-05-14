/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/net/dns/secure_dns_endpoints.h"
#include "net/base/net_export.h"

namespace net {
namespace features {

NET_EXPORT BASE_DECLARE_FEATURE(kBraveEphemeralStorage);
NET_EXPORT BASE_DECLARE_FEATURE(kBraveEphemeralStorageKeepAlive);
NET_EXPORT extern const base::FeatureParam<int>
    kBraveEphemeralStorageKeepAliveTimeInSeconds;
NET_EXPORT BASE_DECLARE_FEATURE(kBraveFirstPartyEphemeralStorage);
NET_EXPORT BASE_DECLARE_FEATURE(kBraveHttpsByDefault);
NET_EXPORT BASE_DECLARE_FEATURE(kBraveFallbackDoHProvider);
NET_EXPORT extern const base::FeatureParam<DohFallbackEndpointType>
    kBraveFallbackDoHProviderEndpoint;
NET_EXPORT BASE_DECLARE_FEATURE(kBravePartitionHSTS);
NET_EXPORT BASE_DECLARE_FEATURE(kBraveTorWindowsHttpsOnly);
NET_EXPORT BASE_DECLARE_FEATURE(kBraveForgetFirstPartyStorage);
NET_EXPORT extern const base::FeatureParam<int>
    kBraveForgetFirstPartyStorageStartupCleanupDelayInSeconds;
NET_EXPORT extern const base::FeatureParam<bool>
    kBraveForgetFirstPartyStorageByDefault;

}  // namespace features
}  // namespace net

#include "src/net/base/features.h"  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_FEATURES_H_
