// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/chromium_src/components/omnibox/browser/autocomplete_provider.h"

// This is a bit of a hack to just make COMMANDER use the same metrics/friendly
// name as BOOKMARK. It's easier than patching things and we don't use the
// metrics and the name is mostly useful for debugging.
#define TYPE_BOOKMARK   \
  TYPE_BRAVE_COMMANDER: \
  case TYPE_BRAVE_LEO:  \
  case TYPE_BOOKMARK
#include "src/components/omnibox/browser/autocomplete_provider.cc"
#undef TYPE_BOOKMARK
