// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_MATCH_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_MATCH_H_

#define GetVectorIcon                                                      \
  GetVectorIcon_Chromium(bool is_bookmark, const TemplateURL* turl) const; \
  const gfx::VectorIcon& GetVectorIcon

#include "src/components/omnibox/browser/autocomplete_match.h"  // IWYU pragma: export

#undef GetVectorIcon

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_MATCH_H_
