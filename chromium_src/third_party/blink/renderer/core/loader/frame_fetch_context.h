/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_FRAME_FETCH_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_FRAME_FETCH_CONTEXT_H_

#include "third_party/blink/renderer/platform/loader/fetch/fetch_context.h"

#define DoesLCPPHaveAnyHintData                                  \
  NotUsedOverride() {                                            \
    return false;                                                \
  }                                                              \
  String GetCacheIdentifierIfCrossSiteSubframe() const override; \
                                                                 \
 private:                                                        \
  mutable scoped_refptr<const SecurityOrigin>                    \
      top_frame_origin_for_cache_identifier_;                    \
  mutable String cache_identifier_if_cross_site_subframe_;       \
                                                                 \
 public:                                                         \
  bool DoesLCPPHaveAnyHintData

#include "src/third_party/blink/renderer/core/loader/frame_fetch_context.h"  // IWYU pragma: export

#undef DoesLCPPHaveAnyHintData

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_FRAME_FETCH_CONTEXT_H_
