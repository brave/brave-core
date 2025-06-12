/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_USER_DATA_IMPORTER_COMMON_IMPORTER_DATA_TYPES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_USER_DATA_IMPORTER_COMMON_IMPORTER_DATA_TYPES_H_

#define BRAVE_IMPORT_ITEM                 \
  EXTENSIONS = 1 << 7, PAYMENTS = 1 << 8, \
  ALL = (1 << 9) - 1  // All the bits should be 1, hence the -1.

#define BRAVE_VISIT_SOURCE VISIT_SOURCE_CHROME_IMPORTED = 4,

#include "src/components/user_data_importer/common/importer_data_types.h"  // IWYU pragma: export
#undef BRAVE_VISIT_SOURCE
#undef BRAVE_IMPORT_ITEM

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_USER_DATA_IMPORTER_COMMON_IMPORTER_DATA_TYPES_H_
