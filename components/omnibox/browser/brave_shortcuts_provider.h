/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SHORTCUTS_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SHORTCUTS_PROVIDER_H_

#include "components/omnibox/browser/shortcuts_provider.h"

class BraveShortcutsProvider : public ShortcutsProvider {
 public:
  using ShortcutsProvider::ShortcutsProvider;

  BraveShortcutsProvider(const BraveShortcutsProvider&) = delete;
  BraveShortcutsProvider& operator=(const BraveShortcutsProvider&) = delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~BraveShortcutsProvider() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_SHORTCUTS_PROVIDER_H_
