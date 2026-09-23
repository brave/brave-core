/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_PROVIDER_H_

#include <optional>
#include <string>

#include "base/auto_reset.h"
#include "brave/components/omnibox/buildflags/buildflags.h"
#include "components/omnibox/browser/search_provider.h"

class AutocompleteInput;
class AutocompleteProviderClient;
class AutocompleteProviderListener;

class BraveSearchProvider : public SearchProvider {
 public:
  BraveSearchProvider(AutocompleteProviderClient* client,
                      AutocompleteProviderListener* listener);
  BraveSearchProvider(const BraveSearchProvider&) = delete;
  BraveSearchProvider& operator=(const BraveSearchProvider&) = delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;
  void DoHistoryQuery(bool minimal_changes) override;
  void UpdateMatches() override;
  bool IsQueryPotentiallyPrivate() const override;
  BraveSearchProvider* AsBraveSearchProvider() override;

  [[nodiscard]] base::AutoReset<bool> SetInputIsPastedFromClipboard(
      bool is_pasted);
  bool IsInputPastedFromClipboard() const;

 protected:
  ~BraveSearchProvider() override;

 private:
#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
  // Returns the answer when `input` is arithmetic we can evaluate exactly, and
  // the long-number check would withhold it from the suggest server.
  std::optional<std::u16string> MaybeEvaluateLocally(
      const AutocompleteInput& input) const;
#endif

  bool input_is_pasted_from_clipboard_ = false;

  // Set for the current input when we answered it ourselves. Computed in
  // `Start()` because `IsQueryPotentiallyPrivate()` reads it from within
  // `SearchProvider::Start()`.
  std::optional<std::u16string> calculator_answer_;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_PROVIDER_H_
