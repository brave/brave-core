/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/extensions/extension_constants.h"
#include "brave/common/url_util.h"
#include "url/gurl.h"

namespace brave {

GURL GetURLOrPDFURL(const GURL& url) {
  if (url.SchemeIs("chrome-extension") &&
      url.host() == pdfjs_extension_id) {
    static size_t pdfjs_substring_len = (std::string("chrome-extension://") +
        pdfjs_extension_id + "/").length();
    size_t http_pos = url.spec().find(std::string("chrome-extension://") +
        pdfjs_extension_id + "/http://");
    size_t https_pos = url.spec().find(std::string("chrome-extension://") +
        pdfjs_extension_id + "/https://");
    if (http_pos != std::string::npos || https_pos != std::string::npos) {
      return GURL(url.spec().substr(pdfjs_substring_len,
          url.spec().length() - pdfjs_substring_len));
    }
  }
  return url;
}

}  // namespace brave
