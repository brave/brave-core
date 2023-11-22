/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_BROWSING_DATA_UTILS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_BROWSING_DATA_UTILS_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#define NUM_TYPES NUM_TYPES, BRAVE_AI_CHAT
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#include "src/components/browsing_data/core/browsing_data_utils.h"  // IWYU pragma: export

#if BUILDFLAG(ENABLE_AI_CHAT)
#undef NUM_TYPES
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_BROWSING_DATA_UTILS_H_
