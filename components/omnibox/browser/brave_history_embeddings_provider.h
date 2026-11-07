// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_EMBEDDINGS_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_EMBEDDINGS_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/history_embeddings/core/history_embeddings_search.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/history_provider.h"

class AutocompleteProviderClient;
class AutocompleteProviderListener;
class TemplateURL;

// Shows semantic history search matches in the @history scope.
class BraveHistoryEmbeddingsProvider : public HistoryProvider {
 public:
  BraveHistoryEmbeddingsProvider(AutocompleteProviderClient* client,
                                 AutocompleteProviderListener* listener);

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~BraveHistoryEmbeddingsProvider() override;

  // Callback for querying `HistoryEmbeddingsService::Search()`.
  void OnReceivedSearchResult(
      brave_history_embeddings::SearchResult search_result);

  // Creates a `HISTORY_EMBEDDINGS` match.
  AutocompleteMatch CreateMatch(
      const brave_history_embeddings::ScoredUrlRow& scored_url_row);

  // Assigned in `Start()`, accessed in `OnReceivedSearchResult()` which is only
  // called asyncly after `Start()`, so it's never null when accessed.
  raw_ptr<const TemplateURL> starter_pack_engine_;

  // The last input sent to `HistoryEmbeddingsService::Search()`.
  AutocompleteInput input_;

  base::WeakPtrFactory<BraveHistoryEmbeddingsProvider> weak_factory_{this};
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_EMBEDDINGS_PROVIDER_H_
