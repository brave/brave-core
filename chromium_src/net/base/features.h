/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_FEATURES_H_

#include "base/feature_list.h"
#include "net/base/net_export.h"

namespace net {
namespace features {

NET_EXPORT extern const base::Feature kBraveEphemeralStorage;

}  // namespace features
}  // namespace net

#include "../../../../net/base/features.h"

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_FEATURES_H_
