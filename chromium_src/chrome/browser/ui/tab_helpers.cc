/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

#define BRAVE_TAB_HELPERS \
  brave::AttachTabHelpers(web_contents);

#include "../../../../../chrome/browser/ui/tab_helpers.cc"
#undef BRAVE_TAB_HELPERS
