// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_LEO_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_LEO_PROVIDER_H_

#include "components/omnibox/browser/autocomplete_provider.h"

class AutocompleteProviderClient;

class LeoProvider : public AutocompleteProvider {
 public:
  static bool IsMatchFromLeoProvider(const AutocompleteMatch& match);

  explicit LeoProvider(AutocompleteProviderClient* client);

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input, bool minimal_changes) override;
  void Stop(bool clear_cached_results, bool due_to_user_inactivity) override;

 private:
  // Destructor for AutocompleteProvider must be private or protected as it
  // extends |base::RefCountedThreadSafe|.
  ~LeoProvider() override;

  raw_ptr<AutocompleteProviderClient> client_;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_LEO_PROVIDER_H_
