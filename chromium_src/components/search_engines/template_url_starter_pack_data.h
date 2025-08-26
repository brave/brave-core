/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_STARTER_PACK_DATA_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_STARTER_PACK_DATA_H_

// Add an enum value for any Brave-defined search engine starter packs.
#define kMaxStarterPackId kAskBraveSearch, kMaxStarterPackId

// Rename upstream's GetStarterPackEngines` so that we can provide our own
// implementation. The original function is used in upstream unit tests.
#define GetStarterPackEngines GetStarterPackEngines_ChromiumImpl

#include <components/search_engines/template_url_starter_pack_data.h>  // IWYU pragma: export

#undef GetStarterPackEngines
#undef kMaxStarterPackId

namespace template_url_starter_pack_data {

extern const StarterPackEngine ask_brave_search;

std::vector<std::unique_ptr<TemplateURLData>> GetStarterPackEngines();

}  // namespace template_url_starter_pack_data

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_STARTER_PACK_DATA_H_
