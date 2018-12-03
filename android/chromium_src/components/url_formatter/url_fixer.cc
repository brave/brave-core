/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define IsEquivalentScheme IsEquivalentScheme_ChromiumImpl
#include "../../../../../../components/url_formatter/url_fixer.cc"
#undef IsEquivalentScheme

namespace url_formatter {

namespace {

const char kBraveUIScheme[] = "brave";

bool IsEquivalentScheme_BraveImpl(const std::string& scheme1,
                                  const std::string& scheme2) {
  return (scheme1 == kBraveUIScheme && scheme2 == kChromeUIScheme) ||
         (scheme1 == kChromeUIScheme && scheme2 == kBraveUIScheme) ||
         (scheme1 == url::kAboutScheme && scheme2 == kBraveUIScheme) ||
         (scheme1 == kBraveUIScheme && scheme2 == url::kAboutScheme);
}

}

bool IsEquivalentScheme(const std::string& scheme1,
                        const std::string& scheme2) {
  if (IsEquivalentScheme_ChromiumImpl(scheme1, scheme2)) {
    return true;
  }
  return IsEquivalentScheme_BraveImpl(scheme1, scheme2);
}

}  // namespace url_formatter
