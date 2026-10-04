/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_BOOKMARK_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_BOOKMARK_PROVIDER_H_

#include "components/omnibox/browser/bookmark_provider.h"

class BraveBookmarkProvider : public BookmarkProvider {
 public:
  using BookmarkProvider::BookmarkProvider;
  BraveBookmarkProvider(const BraveBookmarkProvider&) = delete;
  BraveBookmarkProvider& operator=(const BraveBookmarkProvider&) = delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~BraveBookmarkProvider() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_BOOKMARK_PROVIDER_H_
