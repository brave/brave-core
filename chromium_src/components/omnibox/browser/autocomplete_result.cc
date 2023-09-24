/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/autocomplete_result.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/omnibox/browser/open_here_action.h"
#include "components/omnibox/common/omnibox_features.h"

#define ConvertOpenTabMatches ConvertOpenTabMatches_Chromium

#include "src/components/omnibox/browser/autocomplete_result.cc"

#undef ConvertOpenTabMatches

// Move match to |index|.
void AutocompleteResult::ReorderMatch(const ACMatches::iterator& it,
                                      int index) {
  DCHECK_LT(index, static_cast<int>(size()));
  const bool move_to_end = (index < 0);
  const auto pos = move_to_end ? std::prev(end()) : std::next(begin(), index);
  // If |match| is already at the desired position, there's nothing to do.
  if (it == pos) {
    return;
  }

  // Rotate |match| to be at the desired position.
  if (pos < it) {
    std::rotate(pos, it, std::next(it));
  } else {
    std::rotate(it, std::next(it), std::next(pos));
  }
}

void AutocompleteResult::RemoveMatch(const ACMatches::iterator& it) {
  matches_.erase(it);
}

void AutocompleteResult::ConvertOpenTabMatches(
    AutocompleteProviderClient* client,
    const AutocompleteInput* input) {
  AutocompleteResult::ConvertOpenTabMatches_Chromium(client, input);

  if (base::FeatureList::IsEnabled(omnibox::kOmniboxTabSwitchByDefault)) {
    // Set the takeover action for all tab matches to be TAB_SWITCH
    for (auto& match : matches_) {
      // If we don't have a tab match there's nothing to do.
      if (!match.has_tab_match.has_value()) {
        continue;
      }

      // If the match already has a takeover action, don't reset it.
      if (match.takeover_action) {
        continue;
      }

      // Otherwise, find the tab switch action (if it exists) and set it to the
      // takeover_action. Add a new "New Tab Action" which will open a new
      // instance of the page.
      for (size_t i = 0; i < match.actions.size(); ++i) {
        auto action = match.actions[i];
        if (action->ActionId() != OmniboxActionId::TAB_SWITCH) {
          continue;
        }
        match.contents = action->GetLabelStrings().hint;
        match.contents_class = {
            ACMatchClassification(0, ACMatchClassification::Style::URL)};
        match.takeover_action = std::move(action);
        match.actions[i] =
            base::MakeRefCounted<OpenHereAction>(match.destination_url);
        break;
      }
    }
  }
}
