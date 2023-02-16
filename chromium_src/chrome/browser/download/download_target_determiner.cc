/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Prompting the user for download location shouldn't be a factor in determining
// the download's danger level.
#define BRAVE_DOWNLOAD_TARGET_DETERMINER_GET_DANGER_LEVEL \
  true) {}                                                \
  if (

#include "src/chrome/browser/download/download_target_determiner.cc"
#undef BRAVE_DOWNLOAD_TARGET_DETERMINER_GET_DANGER_LEVEL
