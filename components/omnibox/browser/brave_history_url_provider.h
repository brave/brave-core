/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_URL_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_URL_PROVIDER_H_

#include "components/omnibox/browser/history_url_provider.h"

class BraveHistoryURLProvider : public HistoryURLProvider {
 public:
  using HistoryURLProvider::HistoryURLProvider;
  BraveHistoryURLProvider(const BraveHistoryURLProvider&) = delete;
  BraveHistoryURLProvider& operator=(const BraveHistoryURLProvider&) = delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~BraveHistoryURLProvider() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_HISTORY_URL_PROVIDER_H_
