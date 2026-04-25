// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_LOCAL_HISTORY_ZERO_SUGGEST_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_LOCAL_HISTORY_ZERO_SUGGEST_PROVIDER_H_

#include "components/omnibox/browser/local_history_zero_suggest_provider.h"

class BraveLocalHistoryZeroSuggestProvider
    : public LocalHistoryZeroSuggestProvider {
 public:
  static BraveLocalHistoryZeroSuggestProvider* Create(
      AutocompleteProviderClient* client,
      AutocompleteProviderListener* listener);

  using LocalHistoryZeroSuggestProvider::LocalHistoryZeroSuggestProvider;

  BraveLocalHistoryZeroSuggestProvider(
      const BraveLocalHistoryZeroSuggestProvider&) = delete;
  BraveLocalHistoryZeroSuggestProvider& operator=(
      const BraveLocalHistoryZeroSuggestProvider&) = delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~BraveLocalHistoryZeroSuggestProvider() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_LOCAL_HISTORY_ZERO_SUGGEST_PROVIDER_H_
