/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/net/base/features.cc"

#include "base/feature_override.h"
#include "brave/net/dns/secure_dns_endpoints.h"

namespace net {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kEnableWebTransportDraft07, base::FEATURE_DISABLED_BY_DEFAULT},
    // Enable NIK-partitioning by default.
    {kPartitionConnectionsByNetworkIsolationKey,
     base::FEATURE_ENABLED_BY_DEFAULT},
    {kTopLevelTpcdOriginTrial, base::FEATURE_DISABLED_BY_DEFAULT},
    {kTpcdMetadataGrants, base::FEATURE_DISABLED_BY_DEFAULT},
    {kWaitForFirstPartySetsInit, base::FEATURE_DISABLED_BY_DEFAULT},
}});

BASE_FEATURE(kBraveEphemeralStorage,
             "EphemeralStorage",
             base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kBraveEphemeralStorageKeepAlive,
             "BraveEphemeralStorageKeepAlive",
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<int> kBraveEphemeralStorageKeepAliveTimeInSeconds = {
    &kBraveEphemeralStorageKeepAlive,
    "BraveEphemeralStorageKeepAliveTimeInSeconds", 30};

BASE_FEATURE(kBraveFirstPartyEphemeralStorage,
             "BraveFirstPartyEphemeralStorage",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Partition HSTS state storage by top frame site.
BASE_FEATURE(kBravePartitionHSTS,
             "BravePartitionHSTS",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables HTTPS-Only Mode in Private Windows with Tor by default.
BASE_FEATURE(kBraveTorWindowsHttpsOnly,
             "BraveTorWindowsHttpsOnly",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enabled HTTPS by Default.
BASE_FEATURE(kBraveHttpsByDefault,
             "HttpsByDefault",
             base::FEATURE_ENABLED_BY_DEFAULT);

// When enabled, use a fallback DNS over HTTPS (DoH)
// provider when the current DNS provider does not offer Secure DNS.
BASE_FEATURE(kBraveFallbackDoHProvider,
             "BraveFallbackDoHProvider",
             base::FEATURE_DISABLED_BY_DEFAULT);

constexpr base::FeatureParam<DohFallbackEndpointType>::Option
    kBraveFallbackDoHProviderEndpointOptions[] = {
        {DohFallbackEndpointType::kNone, "none"},
        {DohFallbackEndpointType::kQuad9, "quad9"},
        {DohFallbackEndpointType::kWikimedia, "wikimedia"},
        {DohFallbackEndpointType::kCloudflare, "cloudflare"}};
constexpr base::FeatureParam<DohFallbackEndpointType>
    kBraveFallbackDoHProviderEndpoint{
        &kBraveFallbackDoHProvider, "BraveFallbackDoHProviderEndpoint",
        DohFallbackEndpointType::kNone,
        &kBraveFallbackDoHProviderEndpointOptions};

// Add "Forget by default" cookie blocking mode which cleanups storage after a
// website is closed.
BASE_FEATURE(kBraveForgetFirstPartyStorage,
             "BraveForgetFirstPartyStorage",
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<int>
    kBraveForgetFirstPartyStorageStartupCleanupDelayInSeconds = {
        &kBraveForgetFirstPartyStorage,
        "BraveForgetFirstPartyStorageStartupCleanupDelayInSeconds", 5};

const base::FeatureParam<bool> kBraveForgetFirstPartyStorageByDefault = {
    &kBraveForgetFirstPartyStorage, "BraveForgetFirstPartyStorageByDefault",
    false};

}  // namespace features
}  // namespace net
