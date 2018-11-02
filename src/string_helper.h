/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>

namespace helper {

class String {
 public:
  static void Split(
      const std::string& str,
      const char delimiter,
      std::vector<std::string>& v);

  static void Join(
      const std::vector<std::string>& v,
      const char delimiter,
      std::string& str);
};

}  // namespace helper
