/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_SKUS_COMMON_FEATURES_H_

namespace base {
struct Feature;
}  // namespace base

namespace skus {
namespace features {

// If enabled, this will expose JavaScript methods to domains to pages in a
// Brave curated allow-list. The first use-case is on `account.brave.com`
// where the browser can participate in the credentialing process. When the
// page calls the methods (from JavaScript), it'll invoke the browser's
// implementation which allows for safe credential interception.
extern const base::Feature kSkusFeature;

}  // namespace features
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_COMMON_FEATURES_H_
