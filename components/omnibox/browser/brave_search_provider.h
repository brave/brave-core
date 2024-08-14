/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_PROVIDER_H_

#include "components/omnibox/browser/search_provider.h"

class BraveSearchProvider : public SearchProvider {
 public:
  using SearchProvider::SearchProvider;
  BraveSearchProvider(const BraveSearchProvider&) = delete;
  BraveSearchProvider& operator=(const BraveSearchProvider&) = delete;

  void DoHistoryQuery(bool minimal_changes) override;
  bool IsQueryPotentiallyPrivate() const override;

 protected:
  ~BraveSearchProvider() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SEARCH_PROVIDER_H_
