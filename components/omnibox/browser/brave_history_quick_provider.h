/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_QUICK_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_QUICK_PROVIDER_H_

#include "components/omnibox/browser/history_quick_provider.h"

class BraveHistoryQuickProvider : public HistoryQuickProvider {
 public:
  using HistoryQuickProvider::HistoryQuickProvider;
  BraveHistoryQuickProvider(const BraveHistoryQuickProvider&) = delete;
  BraveHistoryQuickProvider& operator=(const BraveHistoryQuickProvider&) =
      delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~BraveHistoryQuickProvider() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_QUICK_PROVIDER_H_
