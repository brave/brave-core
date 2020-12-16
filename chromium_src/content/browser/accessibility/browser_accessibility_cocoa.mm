/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/debug/dump_without_crashing.h"

#define BRAVE_FORPARAMETER                \
  if (startOffset < 0 || endOffset < 0) { \
    base::debug::DumpWithoutCrashing();   \
    return nil;                           \
  }

#include "../../../../../../../content/browser/accessibility/browser_accessibility_cocoa.mm"
