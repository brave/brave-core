/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

namespace helper {

class Locale {
 public:
  static const std::string GetLanguageCode(const std::string& locale);
  static const std::string GetCountryCode(const std::string& locale);
};

}  // namespace helper
