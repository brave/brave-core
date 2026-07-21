/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"

#include <optional>

#include "base/auto_reset.h"
#include "base/feature_list.h"
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

// Whether `text` is a Brave Commander command, which must never be treated as a
// paste-and-go navigation.
bool IsBraveCommanderCommand(const std::u16string& text) {
#if BUILDFLAG(ENABLE_COMMANDER)
  return base::FeatureList::IsEnabled(features::kBraveCommander) &&
         text.starts_with(commander::kCommandPrefix);
#else
  return false;
#endif
}

// Seeds the Brave search provider with whether the current input was pasted
// from the clipboard, returning a scoped flag that clears itself once the
// caller's autocomplete run is done.
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

#include <chrome/browser/ui/omnibox/omnibox_edit_model.cc>
