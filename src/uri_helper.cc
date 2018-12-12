/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "uri_helper.h"

namespace helper {

std::string Uri::GetUri(const std::string& url) {
  if (url.find("http://") != 0 && url.find("https://") != 0) {
    return "https://" + url;
  }

  return url;
}

}  // namespace helper
