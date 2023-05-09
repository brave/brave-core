// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/brave_omnibox_client.h"

#include "components/omnibox/browser/omnibox_controller.h"

class BraveOmniboxController : public OmniboxController {
 public:
  BraveOmniboxController(OmniboxView* view,
                         OmniboxEditModelDelegate* edit_model_delegate,
                         std::unique_ptr<OmniboxClient> client)
      : OmniboxController(view, edit_model_delegate, std::move(client)) {}
  BraveOmniboxController(
      OmniboxEditModelDelegate* edit_model_delegate,
      std::unique_ptr<AutocompleteController> autocomplete_controller,
      std::unique_ptr<OmniboxClient> client)
      : OmniboxController(edit_model_delegate,
                          std::move(autocomplete_controller),
                          std::move(client)) {}
  BraveOmniboxController(const BraveOmniboxController&) = delete;
  BraveOmniboxController& operator=(const BraveOmniboxController&) = delete;
  ~BraveOmniboxController() override = default;

  // OmniboxController overrides:
  void StartAutocomplete(const AutocompleteInput& input) const override {
    auto* client = static_cast<BraveOmniboxClient*>(client_.get());
    if (!client->IsAutocompleteEnabled()) {
      return;
    }

    OmniboxController::StartAutocomplete(input);
  }
};

#define OmniboxController BraveOmniboxController

#include "src/components/omnibox/browser/omnibox_view.cc"

#undef OmniboxController
