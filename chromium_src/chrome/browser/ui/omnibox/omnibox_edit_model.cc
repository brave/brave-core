/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"

#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/omnibox/browser/brave_search_provider.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/vector_icons/vector_icons.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#endif

namespace {
[[nodiscard]] std::optional<base::AutoReset<bool>>
SetInputIsPastedFromClipboard(OmniboxController* omnibox_controller,
                              bool is_input_pasted) {
  CHECK(omnibox_controller);

  if (auto* autocomplete_controller =
          omnibox_controller->autocomplete_controller()) {
    if (auto* search_provider = autocomplete_controller->search_provider()) {
      return search_provider->AsBraveSearchProvider()
          ->SetInputIsPastedFromClipboard(is_input_pasted);
    }
  }
  return std::nullopt;
}
}  // namespace

#define CanPasteAndGo CanPasteAndGo_Chromium
#define PasteAndGo PasteAndGo_Chromium
#define GetSuperGIcon GetSuperGIcon_Unused
#define BRAVE_OMNIBOX_EDIT_MODEL_START_AUTOCOMPLETE \
  auto pasted = SetInputIsPastedFromClipboard(      \
      controller_, paste_state_ != PasteState::kNone);

#include <chrome/browser/ui/omnibox/omnibox_edit_model.cc>

#undef GetSuperGIcon
#undef CanPasteAndGo
#undef PasteAndGo
#undef BRAVE_OMNIBOX_EDIT_MODEL_START_AUTOCOMPLETE

bool OmniboxEditModel::CanPasteAndGo(const std::u16string& text) const {
#if BUILDFLAG(ENABLE_COMMANDER)
  if (base::FeatureList::IsEnabled(features::kBraveCommander) &&
      text.starts_with(commander::kCommandPrefix)) {
    return false;
  }
#endif
  return CanPasteAndGo_Chromium(text);
}

void OmniboxEditModel::PasteAndGo(const std::u16string& text,
                                  base::TimeTicks match_selection_timestamp) {
  if (view_) {
    view_->RevertAll();
  }

  PasteAndGo_Chromium(text, match_selection_timestamp);
}

// Chromium dynamically updates search engine's favicon when the user visits the
// search engine (see SearchEngineTabHelper::OnFaviconUpdated). However, Google
// search has different favicons for regular search vs shopping search. Because
// of this, if Google is the default search engine the omnibox would switch
// between the two favicons depending on which search was used last. To avoid
// this Chrome uses prepackaged icons returned by the method below. We don't
// have the same icons since those are Chrome specific, so we are going to use a
// generic Google color icon here for both light and dark modes.
ui::ImageModel OmniboxEditModel::GetSuperGIcon(int image_size,
                                               bool dark_mode) const {
  return ui::ImageModel::FromVectorIcon(vector_icons::kGoogleColorIcon,
                                        gfx::kPlaceholderColor, image_size);
}
