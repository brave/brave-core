/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <services/network/public/cpp/features.cc>

namespace network::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kBrowsingTopics, base::FEATURE_DISABLED_BY_DEFAULT},
    {kInterestGroupStorage, base::FEATURE_DISABLED_BY_DEFAULT},
    {kLocalNetworkAccessChecksWebSockets, base::FEATURE_ENABLED_BY_DEFAULT},
    {kSharedStorageAPI, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace network::features
