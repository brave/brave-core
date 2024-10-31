/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_

#include "src/chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"  // IWYU pragma: export

namespace chrome_browsing_data_remover {
constexpr DataType GetBraveDataTypeValue(const int index) {
  return DataType(1) << (63 - index);
}

constexpr DataType DATA_TYPE_BRAVE_LEO_HISTORY = GetBraveDataTypeValue(0);

}  // namespace chrome_browsing_data_remover

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_
