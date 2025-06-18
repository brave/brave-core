/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/importer/importer_uma.h"

#define TYPE_FIREFOX                           \
  TYPE_CHROME:                                 \
  case user_data_importer::TYPE_EDGE_CHROMIUM: \
  case user_data_importer::TYPE_VIVALDI:       \
  case user_data_importer::TYPE_OPERA:         \
  case user_data_importer::TYPE_YANDEX:        \
  case user_data_importer::TYPE_WHALE:         \
  break;                                       \
  case user_data_importer::TYPE_FIREFOX
#include "src/chrome/browser/importer/importer_uma.cc"
#undef TYPE_FIREFOX
