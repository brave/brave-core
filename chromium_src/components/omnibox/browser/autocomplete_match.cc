// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/autocomplete_match.h"

#include "brave/components/omnibox/browser/commander_provider.h"
#include "brave/components/vector_icons/vector_icons.h"

#if (!BUILDFLAG(IS_ANDROID) || BUILDFLAG(ENABLE_VR)) && !BUILDFLAG(IS_IOS)
const gfx::VectorIcon& AutocompleteMatch::GetVectorIcon(
    bool is_bookmark,
    const TemplateURL* turl) const {
  // TODO: `GetAdditionalInfoForDebugging()` shouldn't be used for non-debugging
  // purposes.
  if (!GetAdditionalInfoForDebugging(commander::kCommanderMatchMarker)
           .empty()) {
    return kLeoCaratRightIcon;
  }
  return GetVectorIcon_Chromium(is_bookmark, turl);
}

#define GetVectorIcon GetVectorIcon_Chromium
#endif

#include "src/components/omnibox/browser/autocomplete_match.cc"  // IWYU pragma: export

#undef GetVectorIcon
