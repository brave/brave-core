/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_starter_pack_data.h"

// Let the upstream unit test use upstream's list of starter pack engines,
// rather than Brave's overridden list.
#define GetStarterPackEngines GetStarterPackEngines_ChromiumImpl

#include <components/omnibox/browser/autocomplete_controller_unittest.cc>

#undef GetStarterPackEngines
