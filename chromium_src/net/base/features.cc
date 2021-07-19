/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../net/base/features.cc"

namespace net {
namespace features {

const base::Feature kBraveEphemeralStorage{"EphemeralStorage",
                                           base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kBraveEphemeralStorageKeepAlive{
    "BraveEphemeralStorageKeepAlive", base::FEATURE_ENABLED_BY_DEFAULT};

const base::FeatureParam<int> kBraveEphemeralStorageKeepAliveTimeInSeconds = {
    &kBraveEphemeralStorageKeepAlive,
    "BraveEphemeralStorageKeepAliveTimeInSeconds", 30};

const base::Feature kBraveFirstPartyEphemeralStorage{
    "BraveFirstPartyEphemeralStorage", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace net
