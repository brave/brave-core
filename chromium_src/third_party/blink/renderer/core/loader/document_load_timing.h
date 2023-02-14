/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_LOAD_TIMING_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_LOAD_TIMING_H_

#define MonotonicTimeToZeroBasedDocumentTime                   \
  MonotonicTimeToZeroBasedDocumentTime(base::TimeTicks) const; \
  base::TimeDelta MonotonicTimeToZeroBasedDocumentTime_ChromiumImpl

#include "src/third_party/blink/renderer/core/loader/document_load_timing.h"  // IWYU pragma: export

#undef MonotonicTimeToZeroBasedDocumentTime

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_LOAD_TIMING_H_
