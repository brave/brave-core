/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// other_node should remain hidden
#define BRAVE_IS_VISIBLE    \
  if (type() == OTHER_NODE) \
    return false;
#include "../../../../../components/bookmarks/browser/bookmark_node.cc"
#undef BRAVE_IS_VISIBLE
