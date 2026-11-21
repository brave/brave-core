// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/autocomplete_match.h"

#include "brave/components/omnibox/browser/commander_provider.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "components/grit/brave_components_strings.h"
#include "components/search_engines/template_url_starter_pack_data.h"

#if !BUILDFLAG(IS_IOS)
namespace {

// Brave's icon for `match`, or null to use upstream's.
const gfx::VectorIcon* GetBraveVectorIcon(const AutocompleteMatch& match,
                                          const TemplateURL* turl) {
  // TODO: `GetAdditionalInfoForDebugging()` shouldn't be used for non-debugging
  // purposes.
  if (!match.GetAdditionalInfoForDebugging(commander::kCommanderMatchMarker)
           .empty()) {
    return &kLeoCaratRightIcon;
  }
  if (match.type == AutocompleteMatch::Type::kStarterPack && turl &&
      turl->GetBuiltinEngineType() ==
          KEYWORD_MODE_STARTER_PACK_ASK_BRAVE_SEARCH) {
    return &kLeoMessageBubbleAskIcon;
  }
  return nullptr;
}

}  // namespace
#endif  // !BUILDFLAG(IS_IOS)

#include <components/omnibox/browser/autocomplete_match.cc>  // IWYU pragma: export
