/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sstream>

#include "string_helper.h"

namespace helper {

void String::Split(
    const std::string& str,
    const char delimiter,
    std::vector<std::string>& v) {
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, delimiter)) {
    v.push_back(token);
  }
}

void String::Join(
    const std::vector<std::string>& v,
    const char delimiter,
    std::string& str) {
  str.clear();

  for (auto p = v.begin(); p != v.end(); ++p) {
    str += *p;
    if (p != v.end() - 1) {
      str += delimiter;
    }
  }
}

}  // namespace helper
