// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_FEATURES_H_

namespace base {
struct Feature;
}  // namespace base

namespace brave_shields {
namespace features {
extern const base::Feature kBraveAdblockCnameUncloaking;
extern const base::Feature kBraveAdblockCosmeticFiltering;
extern const base::Feature kBraveAdblockCosmeticFilteringNative;
extern const base::Feature kBraveAdblockCspRules;
extern const base::Feature kBraveDomainBlock;
extern const base::Feature kBraveExtensionNetworkBlocking;
}  // namespace features
}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_FEATURES_H_
