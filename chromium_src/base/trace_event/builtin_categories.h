/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_TRACE_EVENT_BUILTIN_CATEGORIES_H_
#define BRAVE_CHROMIUM_SRC_BASE_TRACE_EVENT_BUILTIN_CATEGORIES_H_

// Please add all brave categories here. Take a look at the original
// builtin_categories.h for more details. Please:
// * For small features use the existing 'brave' category.
// * For new big subsystems with a lot of traces create a separate category
// named 'brave.<feature_name>'.
// * Keep the list sorted.
//
// Note: There is no #undef for this macros because it is used in exported
// macros of the chromium builtin_categories.h.
#define BRAVE_INTERNAL_TRACE_LIST_BUILTIN_CATEGORIES(X) \
  X("brave")                                            \
  X("brave.adblock")                                    \
  X("brave.ads")

#include "src/base/trace_event/builtin_categories.h"  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_BASE_TRACE_EVENT_BUILTIN_CATEGORIES_H_
