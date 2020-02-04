/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/webui_url_constants.h"
#include "brave/common/url_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/webui_url_constants.h"

BraveAutocompleteProviderClient::BraveAutocompleteProviderClient(
    Profile* profile)
    : ChromeAutocompleteProviderClient(profile->GetOriginalProfile()),
      profile_(profile) {
}

BraveAutocompleteProviderClient::~BraveAutocompleteProviderClient() {
}

std::vector<base::string16>
BraveAutocompleteProviderClient::GetBuiltinsToProvideAsUserTypes() {
  std::vector<base::string16> builtins_to_provide =
      ChromeAutocompleteProviderClient::GetBuiltinsToProvideAsUserTypes();
  builtins_to_provide.push_back(
      base::ASCIIToUTF16(kBraveUIRewardsURL));
  builtins_to_provide.push_back(
      base::ASCIIToUTF16(kBraveUITipURL));
  builtins_to_provide.push_back(
      base::ASCIIToUTF16(kBraveUIWalletURL));

  for (auto i(builtins_to_provide.begin()); i != builtins_to_provide.end(); ++i) {
    *i = brave::ReplaceChromeSchemeWithBrave(*i);
  }
  return builtins_to_provide;
}

std::string
BraveAutocompleteProviderClient::GetEmbedderRepresentationOfAboutScheme()
    const {
  return content::kBraveUIScheme;
}

TemplateURLService* BraveAutocompleteProviderClient::GetTemplateURLService() {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}

const TemplateURLService*
BraveAutocompleteProviderClient::GetTemplateURLService() const {
  return TemplateURLServiceFactory::GetForProfile(profile_);
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
  for (auto i(v.begin()); i != v.end(); ++i) {
    *i = brave::ReplaceChromeSchemeWithBrave(*i);
  }
  return v;
}
