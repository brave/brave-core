/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/common/webui_url_constants.h"

BraveAutocompleteProviderClient::BraveAutocompleteProviderClient(
    Profile* profile) : ChromeAutocompleteProviderClient(profile) {
}

BraveAutocompleteProviderClient::~BraveAutocompleteProviderClient() {
}

std::vector<base::string16> BraveAutocompleteProviderClient::GetBuiltinURLs() {
  std::vector<base::string16> v =
      ChromeAutocompleteProviderClient::GetBuiltinURLs();
  auto it = std::find(v.begin(), v.end(),
      base::ASCIIToUTF16(chrome::kChromeUISyncInternalsHost));
  DCHECK(it != v.end());
  if (it != v.end()) {
    *it = base::ASCIIToUTF16(kBraveUISyncHost);
  }
  return v;
}
