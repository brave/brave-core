/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_UTILS_H_
#define BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_UTILS_H_

class GURL;

namespace blink {
class WebSecurityOrigin;
}  // namespace blink

namespace skus {
// This version is used in a renderer process where blink is initialized.
// For example, if you are in a render frame observer where you get the origin
// via `render_frame()->GetWebFrame()->GetSecurityOrigin()`.
//
// NOTE: You'll get DCHECK/CHECK errors for trying to create a
// `blink::WebString` if you're not in a blink context (tests are fine).
//
// See //third_party/blink/renderer/platform/weborigin/security_origin.cc
bool IsSafeOrigin(const blink::WebSecurityOrigin& origin);

// This version is safe for use elsewhere. The internal `IsSameOriginWith`
// check is different than the version above.
//
// See //url/origin.cc
bool IsSafeOrigin(const GURL& origin);
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_UTILS_H_
