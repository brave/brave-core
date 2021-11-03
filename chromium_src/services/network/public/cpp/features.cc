/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../services/network/public/cpp/features.cc"

#include "base/feature_override.h"

namespace network {
namespace features {

DISABLE_FEATURE_BY_DEFAULT(kTrustTokens);

}  // namespace features
}  // namespace network
