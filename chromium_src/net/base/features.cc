/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/net/base/features.cc"

#include "base/feature_override.h"

namespace net {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kNoncedPartitionedCookies, base::FEATURE_DISABLED_BY_DEFAULT},
    // Enable NIK-partitioning by default.
    {kPartitionConnectionsByNetworkIsolationKey,
     base::FEATURE_ENABLED_BY_DEFAULT},
    {kPartitionedCookies, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPartitionExpectCTStateByNetworkIsolationKey,
     base::FEATURE_ENABLED_BY_DEFAULT},
    {kPartitionHttpServerPropertiesByNetworkIsolationKey,
     base::FEATURE_ENABLED_BY_DEFAULT},
    {kPartitionSSLSessionsByNetworkIsolationKey,
     base::FEATURE_ENABLED_BY_DEFAULT},
    {kSplitHostCacheByNetworkIsolationKey, base::FEATURE_ENABLED_BY_DEFAULT},
}});

const base::Feature kBraveEphemeralStorage{"EphemeralStorage",
                                           base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kBraveEphemeralStorageKeepAlive{
    "BraveEphemeralStorageKeepAlive", base::FEATURE_ENABLED_BY_DEFAULT};

const base::FeatureParam<int> kBraveEphemeralStorageKeepAliveTimeInSeconds = {
    &kBraveEphemeralStorageKeepAlive,
    "BraveEphemeralStorageKeepAliveTimeInSeconds", 30};

const base::Feature kBraveFirstPartyEphemeralStorage{
    "BraveFirstPartyEphemeralStorage", base::FEATURE_ENABLED_BY_DEFAULT};

// Partition Blob storage in ephemeral context.
const base::Feature kBravePartitionBlobStorage{
    "BravePartitionBlobStorage", base::FEATURE_ENABLED_BY_DEFAULT};

// Partition HSTS state storage by top frame site.
const base::Feature kBravePartitionHSTS{"BravePartitionHSTS",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features
}  // namespace net
