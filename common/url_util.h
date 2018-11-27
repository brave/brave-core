/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_URL_UTIL_H_
#define BRAVE_COMMON_URL_UTIL_H_

class GURL;

namespace brave {

// Returns the location of the PDF if this URL is a PDFJS extension URL.
// Otherwise simply just returns the same URL as passed in.
GURL GetURLOrPDFURL(const GURL& url);

}  // namespace brave

#endif  // BRAVE_COMMON_URL_UTIL_H_
