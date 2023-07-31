/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_

#include "brave/components/ai_chat/common/buildflags/buildflags.h"

#include "src/chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"  // IWYU pragma: export
namespace chrome_browsing_data_remover {
#if BUILDFLAG(ENABLE_AI_CHAT)
constexpr DataType DATA_TYPE_BRAVE_LEO_HISTORY = DATA_TYPE_EMBEDDER_BEGIN << 13;
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
}

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_CONSTANTS_H_
