// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_OMNIBOX_BRAVE_OMNIBOX_CLIENT_H_
#define BRAVE_BROWSER_UI_OMNIBOX_BRAVE_OMNIBOX_CLIENT_H_

#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"

class OmniboxEditController;
class Profile;

class BraveOmniboxClient : public ChromeOmniboxClient {
  public:
    BraveOmniboxClient(OmniboxEditController* controller,
                                         Profile* profile);
    ~BraveOmniboxClient() override;
    const AutocompleteSchemeClassifier& GetSchemeClassifier() const override;

  private:
    DISALLOW_COPY_AND_ASSIGN(BraveOmniboxClient);
    BraveAutocompleteSchemeClassifier scheme_classifier_;
};

#endif
