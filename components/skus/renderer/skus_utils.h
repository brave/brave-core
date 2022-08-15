/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_UTILS_H_
#define BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_UTILS_H_

namespace blink {
class WebSecurityOrigin;
}  // namespace blink

namespace skus {
bool IsSafeOrigin(const blink::WebSecurityOrigin& origin);
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_UTILS_H_
