/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_SKIP_IF_VERTICAL_TAB \
  if ((true))                      \
    return true;

#include "src/chrome/browser/ui/views/frame/browser_view_layout.cc"

#undef BRAVE_SKIP_IF_VERTICAL_TAB