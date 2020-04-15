/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_PROVIDER_CLIENT_H_
#define BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_PROVIDER_CLIENT_H_

#include <string>
#include <vector>

#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"

class BraveAutocompleteProviderClient
    : public ChromeAutocompleteProviderClient {
 public:
  explicit BraveAutocompleteProviderClient(Profile* profile);
  ~BraveAutocompleteProviderClient() override;

  std::vector<base::string16> GetBuiltinURLs() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveAutocompleteProviderClient);
};

#endif  // BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_PROVIDER_CLIENT_H_
