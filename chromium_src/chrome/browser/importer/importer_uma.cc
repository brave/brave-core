/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/importer/importer_uma.h"

#define TYPE_FIREFOX       \
  TYPE_CHROME:             \
  case TYPE_EDGE_CHROMIUM: \
  case TYPE_VIVALDI:       \
  case TYPE_OPERA:         \
  break;                   \
  case TYPE_FIREFOX
#include "src/chrome/browser/importer/importer_uma.cc"
