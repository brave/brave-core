/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/ui/autofill_external_delegate.h"

#include <ranges>
#include <vector>

#include "base/containers/map_util.h"
#include "components/autofill/core/browser/foundations/autofill_driver.h"
#include "components/autofill/core/browser/foundations/autofill_manager.h"
#include "components/autofill/core/browser/suggestions/suggestion.h"

// This patch allows us to add the additional autofill suggestions.
// BraveAddSuggestions accepts the form classification, the field data and the
// reference to chromium suggestions.

#define BRAVE_AUTOFILL_EXTERNAL_DELEGATE_ATTEMPT_TO_DISPLAY_AUTOFILL_SUGGESTIONS \
  {                                                                              \
    manager_->client().BraveAddSuggestions(                                      \
        manager_->client().ClassifyAsPasswordForm(                               \
            *manager_, query_form_.global_id(), query_field_.global_id()),       \
        query_field_, suggestions);                                              \
  }

#define DidAcceptSuggestion(...) DidAcceptSuggestion_ChromiumImpl(__VA_ARGS__)

#include <components/autofill/core/browser/ui/autofill_external_delegate.cc>

#undef BRAVE_AUTOFILL_EXTERNAL_DELEGATE_ATTEMPT_TO_DISPLAY_AUTOFILL_SUGGESTIONS
#undef DidAcceptSuggestion

namespace autofill {

void AutofillExternalDelegate::DidAcceptSuggestion(
    const Suggestion& suggestion,
    const SuggestionMetadata& metadata) {
  if (manager_->client().BraveHandleSuggestion(suggestion,
                                               query_field_.global_id())) {
    return;
  }

  DidAcceptSuggestion_ChromiumImpl(suggestion, metadata);
}

}  // namespace autofill
