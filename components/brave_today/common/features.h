/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_FEATURES_H_

namespace base {
struct Feature;
}  // namespace base

namespace brave_today {
namespace features {

extern const base::Feature kBraveNewsFeature;
extern const base::Feature kBraveNewsCardPeekFeature;
extern const base::Feature kBraveNewsSubscribeButtonFeature;

}  // namespace features
}  // namespace brave_today

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_FEATURES_H_
