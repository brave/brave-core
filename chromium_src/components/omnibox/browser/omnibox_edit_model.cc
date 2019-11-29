/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_omnibox_client.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/omnibox_controller.h"

class BraveOmniboxController : public OmniboxController {
 public:
  BraveOmniboxController(OmniboxEditModel* omnibox_edit_model,
                         OmniboxClient* client)
      : OmniboxController(omnibox_edit_model, client),
        client_(static_cast<BraveOmniboxClient*>(client)) {}
  ~BraveOmniboxController() override = default;

  // OmniboxController overrides:
  void StartAutocomplete(const AutocompleteInput& input) const override {
    if (!client_->IsAutocompleteEnabled())
      return;

    OmniboxController::StartAutocomplete(input);
  }

 private:
  BraveOmniboxClient* client_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveOmniboxController);
};

#define OmniboxController BraveOmniboxController
#include "../../../../../components/omnibox/browser/omnibox_edit_model.cc"  // NOLINT
#undef OmniboxController
