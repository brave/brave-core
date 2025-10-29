/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_TYPE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_TYPE_H_

// Add enum values for for any Brave-defined search engine starter packs.
#define SEARCH_ENGINE_MAX \
  SEARCH_ENGINE_STARTER_PACK_ASK_BRAVE_SEARCH = 100, SEARCH_ENGINE_MAX
#define KEYWORD_MODE_ENGINE_TYPE_MAX \
  KEYWORD_MODE_STARTER_PACK_ASK_BRAVE_SEARCH = 200, KEYWORD_MODE_ENGINE_TYPE_MAX

#include <components/search_engines/search_engine_type.h>  // IWYU pragma: export

#undef KEYWORD_MODE_ENGINE_TYPE_MAX
#undef SEARCH_ENGINE_MAX

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_TYPE_H_
