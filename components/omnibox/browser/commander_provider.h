// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_PROVIDER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/commander/common/commander_frontend_delegate.h"
#include "brave/components/commander/common/commander_item_model.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"

namespace commander {
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

 private:
  ~CommanderProvider() override;
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
