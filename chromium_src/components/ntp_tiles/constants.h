/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_NTP_TILES_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_NTP_TILES_CONSTANTS_H_

// Needs 12 items for our NTP top site tiles.
#define kMaxNumTiles \
  kMaxNumTiles = 12; \
  const int kMaxNumTiles_Unused

#include "src/components/ntp_tiles/constants.h"  // IWYU pragma: export

#undef kMaxNumTiles

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_NTP_TILES_CONSTANTS_H_
