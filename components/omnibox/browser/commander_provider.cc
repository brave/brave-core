// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/commander_provider.h"

#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/commander_action.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/prefs/pref_service.h"
#include "third_party/omnibox_proto/groups.pb.h"

namespace commander {
CommanderProvider::CommanderProvider(AutocompleteProviderClient* client,
                                     AutocompleteProviderListener* listener)
    : AutocompleteProvider(AutocompleteProvider::TYPE_BRAVE_COMMANDER),
      client_(client) {
  if (listener) {
    AddListener(listener);
  }

  if (auto* delegate = client_->GetCommanderDelegate()) {
    observation_.Observe(delegate);
  }
}

CommanderProvider::~CommanderProvider() = default;

void CommanderProvider::Start(const AutocompleteInput& input,
                              bool minimal_changes) {
  if (minimal_changes) {
    return;
  }

  matches_.clear();
  last_input_ = input.text();

  if (auto* delegate = client_->GetCommanderDelegate()) {
    delegate->UpdateText(input.text());
  }
}

void CommanderProvider::Stop(bool clear_cached_results,
                             bool due_to_user_inactivity) {
  last_input_.clear();
  AutocompleteProvider::Stop(clear_cached_results, due_to_user_inactivity);
}

void CommanderProvider::OnCommanderUpdated() {
  // This is called the CommanderFrontEndDelegate::Observer, so a delegate must
  // always exist at this point.
  auto* delegate = client_->GetCommanderDelegate();
  CHECK(delegate);

  matches_.clear();

  // If |last_input_| is empty, don't provide any suggestions.
  if (last_input_.empty()) {
    return;
  }

  // We can be triggered explicitly by the prefix, or in a normal search if
  // suggestions are enabled.
  auto has_prefix = last_input_.starts_with(commander::kCommandPrefix.data());
  if (!has_prefix &&
      (!client_->GetPrefs()->GetBoolean(
           omnibox::kCommanderSuggestionsEnabled) ||
       !base::FeatureList::IsEnabled(features::kBraveCommandsInOmnibox))) {
    return;
  }

  const auto& items = delegate->GetItems();

  // If we have a prefix, the commands should be given the maximum ranking, as
  // we want them to be prioritised. If not, add commands only if they're a good
  // match, and dump them at the bottom of our results.
  int rank = (has_prefix ? 1000 : 100) + items.size();
  for (uint32_t i = 0; i < items.size(); ++i) {
    const auto& option = items[i];
    AutocompleteMatch match(this, rank--, false,
                            AutocompleteMatchType::BOOKMARK_TITLE);
    match.RecordAdditionalInfo(kCommanderMatchMarker, true);
    match.takeover_action =
        base::MakeRefCounted<CommanderAction>(i, delegate->GetResultSetId());

    // This is neat but it would be nice if we could always show it instead of
    // only when we have a result selected.
    match.contents = option.annotation;
    if (!option.annotation.empty()) {
      match.contents_class = {
          ACMatchClassification(0, ACMatchClassification::DIM)};
    }
    match.description = option.title;
    match.allowed_to_be_default_match = true;
    match.swap_contents_and_description = true;

    // Only group quick commands if there are potentially other results.
    if (!has_prefix) {
      match.suggestion_group_id = omnibox::GroupId::GROUP_OTHER_NAVS;
    }

    // We don't want to change the prompt at all while the user is going through
    // their options.
    match.fill_into_edit = last_input_;
    match.description_class = {
        ACMatchClassification(0, ACMatchClassification::DIM)};

    for (size_t j = 0; j < option.matched_ranges.size(); ++j) {
      auto range = option.matched_ranges[j];
      // If the match has no length (as in the case of the empty string match)
      // don't highlight anything - zero length highlights trigger a DCHECK.
      if (range.start() == range.end()) {
        continue;
      }

      // If the match starts from the beginning of the text, convert our
      // starting style to MATCH.
      if (range.start() == 0) {
        match.description_class[0].style = ACMatchClassification::MATCH;
      } else {
        // Otherwise, change the style to be match, from this token onwards.
        match.description_class.emplace_back(range.start(),
                                             ACMatchClassification::MATCH);
      }

      // If the end of the range isn't the last character in the string, and
      // this range doesn't intersect with the next one, change the
      // classification back to DIM from the end of this range.
      if (range.end() < match.description.size() &&
          (j + 1 >= option.matched_ranges.size() ||
           option.matched_ranges[j + 1].start() > range.end())) {
        match.description_class.emplace_back(range.end(),
                                             ACMatchClassification::DIM);
      }
    }
    matches_.push_back(match);
  }

  // Only call NotifyListeners if the update was triggered asynchronously, to
  // avoid triggering a DCHECK in AutocompleteController.
  if (!done()) {
    NotifyListeners(/* updated_matches= */ true);
  }
}
}  // namespace commander
