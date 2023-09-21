/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_

#include "brave/components/ai_chat/common/buildflags/buildflags.h"

#include "src/chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"  // IWYU pragma: export

namespace chrome_browsing_data_remover {
constexpr int BRAVE_DATA_TYPE_EMBEDDER_START_SHIFT_POS = 64 - 24;
constexpr DataType GetBraveDataTypeValue(const int& index) {
  return DATA_TYPE_EMBEDDER_BEGIN
         << (BRAVE_DATA_TYPE_EMBEDDER_START_SHIFT_POS - index - 1);
}

#if BUILDFLAG(ENABLE_AI_CHAT)
constexpr DataType DATA_TYPE_BRAVE_LEO_HISTORY = GetBraveDataTypeValue(0);
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
}  // namespace chrome_browsing_data_remover

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_
