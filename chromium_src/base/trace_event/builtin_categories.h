// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_CHROMIUM_SRC_BASE_TRACE_EVENT_BUILTIN_CATEGORIES_H_
#define BRAVE_CHROMIUM_SRC_BASE_TRACE_EVENT_BUILTIN_CATEGORIES_H_

// Please add all brave categories here. For small features use the existing
// 'brave' category. For new big subsystems with a lot of traces create an
// separated category named 'brave.<feature_name>`.
//
// Note: The macros shouldn't be undefined because it can be used in exported
// macros of the chromium builtin_categories.h.
#define BRAVE_INTERNAL_TRACE_LIST_BUILTIN_CATEGORIES(X) \
  X("brave")                                            \
  X("brave.adblock")

#include "../../../../base/trace_event/builtin_categories.h"

#endif  // BRAVE_CHROMIUM_SRC_BASE_TRACE_EVENT_BUILTIN_CATEGORIES_H_
