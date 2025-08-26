/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "ui/gfx/vector_icon_types.h"

// Defined in brave/browser/ui/views/location_bar.
const gfx::VectorIcon& GetAskBraveSearchStarterPackIcon();

// Sets the icon that is displayed when a Brave-defined starter pack engine is
// active.
#define kAiMode kAskBraveSearch) {                              \
    vector_icon = &GetAskBraveSearchStarterPackIcon();          \
    /* NOLINTNEXTLINE */                                        \
  } else if (template_url && template_url->starter_pack_id() == \
                                 template_url_starter_pack_data::kAiMode

#include <chrome/browser/ui/views/location_bar/selected_keyword_view.cc>

#undef kAiMode
