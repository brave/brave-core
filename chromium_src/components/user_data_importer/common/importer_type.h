/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_USER_DATA_IMPORTER_COMMON_IMPORTER_TYPE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_USER_DATA_IMPORTER_COMMON_IMPORTER_TYPE_H_

#define TYPE_FIREFOX                                           \
  TYPE_CHROME = 1, TYPE_EDGE_CHROMIUM = 10, TYPE_VIVALDI = 11, \
  TYPE_OPERA = 12, TYPE_YANDEX = 13, TYPE_WHALE = 14, TYPE_FIREFOX
#include "src/components/user_data_importer/common/importer_type.h"  // IWYU pragma: export
#undef TYPE_FIREFOX

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_USER_DATA_IMPORTER_COMMON_IMPORTER_TYPE_H_
