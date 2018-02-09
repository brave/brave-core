/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef COMPONENTS_OMNIBOX_BROWSER_BRAVE_AUTOCOMPLETE_CONTROLLER_H_
#define COMPONENTS_OMNIBOX_BROWSER_BRAVE_AUTOCOMPLETE_CONTROLLER_H_

#include "components/omnibox/browser/autocomplete_controller.h"

class BraveAutocompleteController : public AutocompleteController {
public:
  BraveAutocompleteController(
      std::unique_ptr<AutocompleteProviderClient> provider_client,
      AutocompleteControllerDelegate* delegate,
      int provider_types);
  ~BraveAutocompleteController() override;
};


#endif  // COMPONENTS_OMNIBOX_BROWSER_BRAVE_AUTOCOMPLETE_CONTROLLER_H_
