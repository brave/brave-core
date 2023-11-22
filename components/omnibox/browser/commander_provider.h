// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_PROVIDER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#include "components/omnibox/browser/autocomplete_provider.h"

class AutocompleteProviderClient;
class AutocompleteProviderListener;

namespace commander {

// Used to distinguish matches for command items from regular
// AutocompleteMatches. Other match types are defined in an enum, but to avoid
// patching as much as possible, we're just using this marker in the
// additional_info lookup to determine whether to show our custom icon.
inline constexpr char kCommanderMatchMarker[] = "command-match";

class CommanderProvider
    : public AutocompleteProvider,
      public commander::CommanderFrontendDelegate::Observer {
 public:
  CommanderProvider(AutocompleteProviderClient* client,
                    AutocompleteProviderListener* listener);
  CommanderProvider(const CommanderProvider&) = delete;
  CommanderProvider& operator=(const CommanderProvider&) = delete;

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input, bool minimal_changes) override;
  void Stop(bool clear_cached_results, bool due_to_user_inactivity) override;

 private:
  // Destructor for AutocompleteProvider must be private or protected as it
  // extends |base::RefCountedThreadSafe|.
  ~CommanderProvider() override;

  // commander::CommanderFrontendDelegate::Observer:
  void OnCommanderUpdated() override;

  raw_ptr<AutocompleteProviderClient> client_;

  base::ScopedObservation<commander::CommanderFrontendDelegate,
                          commander::CommanderFrontendDelegate::Observer>
      observation_{this};

  std::u16string last_input_;
  base::WeakPtrFactory<CommanderProvider> weak_ptr_factory_{this};
};
}  // namespace commander

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_PROVIDER_H_
