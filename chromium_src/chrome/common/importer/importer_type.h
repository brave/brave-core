/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_TYPE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_TYPE_H_

#define TYPE_FIREFOX                                           \
  TYPE_CHROME = 1, TYPE_EDGE_CHROMIUM = 10, TYPE_VIVALDI = 11, \
  TYPE_OPERA = 12, TYPE_FIREFOX
#include "src/chrome/common/importer/importer_type.h"
#undef TYPE_FIREFOX

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_TYPE_H_
