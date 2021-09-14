/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/debug/alias.h"
#include "base/debug/dump_without_crashing.h"

// Assumed to be a temporary fix for
// https://github.com/brave/brave-browser/issues/13778
// and
// https://github.com/brave/brave-browser/issues/13837
#define BRAVE_ACCESSIBILITY_ATTRIBUTED_STRING_FOR_RANGE             \
  id value = [self AXValue];                                        \
  if (![value isKindOfClass:[NSString class]]) {                    \
    ax::mojom::Role role = _node->GetData().role;                   \
    base::debug::Alias(&role);                                      \
    LOG(ERROR) << __PRETTY_FUNCTION__;                              \
    LOG(ERROR) << "Trying to get a range from a non-string object." \
               << " Role: " << static_cast<int>(role)               \
               << " Name: " << _node->GetName();                    \
    base::debug::DumpWithoutCrashing();                             \
    return nil;                                                     \
  }                                                                 \
  NSString* str = value;                                            \
  if (range.location == NSNotFound ||                               \
      range.location + range.length > str.length) {                 \
    base::debug::DumpWithoutCrashing();                             \
    return nil;                                                     \
  }

#include "../../../../../ui/accessibility/platform/ax_platform_node_cocoa.mm"
