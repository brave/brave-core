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
#include "brave/components/omnibox/browser/commander_action.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"

namespace commander {
CommanderProvider::CommanderProvider(AutocompleteProviderClient* client,
                                     AutocompleteProviderListener* listener)
    : AutocompleteProvider(AutocompleteProvider::TYPE_BRAVE_COMMANDER),
      client_(client) {
  if (listener) {
    AddListener(listener);
  }

  if (auto* delegate = client_->GetCommanderDelegate()) {
    observation_.Observe(client_->GetCommanderDelegate());
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
    delegate->UpdateText();
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

  if (!last_input_.starts_with(commander::kCommandPrefix.data())) {
    return;
  }

  int rank = 10000;
  const auto& items = delegate->GetItems();
  for (uint32_t i = 0; i < items.size(); ++i) {
    const auto& option = items[i];
    AutocompleteMatch match(this, rank--, false,
                            AutocompleteMatchType::BOOKMARK_TITLE);
    match.takeover_action =
        base::MakeRefCounted<CommanderAction>(i, delegate->GetResultSetId());

    // This is neat but it would be nice if we could always show it instead of
    // only when we have a result selected.
    match.contents = option.annotation;
    if (!option.annotation.empty()) {
      match.contents_class = {
          ACMatchClassification(0, ACMatchClassification::DIM)};
    }
    match.description =
        base::StrCat({commander::kCommandPrefix, u" ", option.title});
    match.allowed_to_be_default_match = true;
    match.swap_contents_and_description = true;
    // We don't want to change the prompt at all while the user is going through
    // their options.
    match.fill_into_edit = last_input_;
    match.description_class = {
        ACMatchClassification(0, ACMatchClassification::DIM)};

    // All commands have a ":> " prefix added to them, so make sure we take it
    // into account when mapping over the matched ranges.
    const int offset = commander::kCommandPrefix.size() + 1;
    for (size_t j = 0; j < option.matched_ranges.size(); ++j) {
      auto range = option.matched_ranges[j];
      // If the match has no length (as in the case of the empty string match)
      // don't highlight anything - zero length highlights trigger a DCHECK.
      if (range.start() == range.end()) {
        continue;
      }

      // Start the match classification at the start of the matching range.
      match.description_class.push_back(ACMatchClassification(
          range.start() + offset, ACMatchClassification::MATCH));

      // If the end of the range isn't the last character in the string, and
      // this range doesn't intersect with the next one, change the
      // classification back to DIM from the end of this range.
      if (range.end() + offset < match.description.size() &&
          (j + 1 >= option.matched_ranges.size() ||
           option.matched_ranges[j + 1].start() > range.end())) {
        match.description_class.push_back(ACMatchClassification(
            range.end() + offset, ACMatchClassification::DIM));
      }
    }
    matches_.push_back(match);
  }

  NotifyListeners(/* updated_matches= */ true);
}
}  // namespace commander
