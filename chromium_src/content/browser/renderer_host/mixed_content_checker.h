/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_MIXED_CONTENT_CHECKER_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_MIXED_CONTENT_CHECKER_H_

#define InWhichFrameIsContentMixed                                             \
  NotUsed();                                                                   \
                                                                               \
  static bool DoesOriginSchemeRestrictMixedContent(const url::Origin& origin); \
                                                                               \
  RenderFrameHostImpl* InWhichFrameIsContentMixed

#include "src/content/browser/renderer_host/mixed_content_checker.h"  // IWYU pragma: export

#undef InWhichFrameIsContentMixed

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_MIXED_CONTENT_CHECKER_H_
