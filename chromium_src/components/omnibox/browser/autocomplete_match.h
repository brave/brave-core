// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_MATCH_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_MATCH_H_

#include "build/build_config.h"
#include "components/omnibox/browser/buildflags.h"

#if (!BUILDFLAG(IS_ANDROID) || BUILDFLAG(ENABLE_VR)) && !BUILDFLAG(IS_IOS)
#define GetVectorIcon                                                      \
  GetVectorIcon_Chromium(bool is_bookmark, const TemplateURL* turl) const; \
  const gfx::VectorIcon& GetVectorIcon
#endif

#include "src/components/omnibox/browser/autocomplete_match.h"  // IWYU pragma: export

#undef GetVectorIcon

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_MATCH_H_
