/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_DATA_TYPES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_DATA_TYPES_H_

#define BRAVE_IMPORT_ITEM \
  STATS              = 1 << 7, \
  LEDGER             = 1 << 8, \
  WINDOWS            = 1 << 9, \
  EXTENSIONS         = 1 << 10, \
  ALL                = (1 << 11) - 1  // All the bits should be 1, hence the -1.

#define BRAVE_VISIT_SOURCE \
  VISIT_SOURCE_CHROME_IMPORTED = 4, \
  VISIT_SOURCE_BRAVE_IMPORTED = 5,

#include "../../../../../chrome/common/importer/importer_data_types.h"

#undef BRAVE_IMPORT_ITEM
#undef BRAVE_VISIT_SOURCE

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_DATA_TYPES_H_
