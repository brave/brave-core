/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_ORIGIN_HELPER_H
#define BRAVE_COMMON_ORIGIN_HELPER_H

class GURL;

namespace brave {

// Warning! Returns true if the result could be determined
// The actual result is in `result`
bool IsSameTLDPlus1(const GURL& url1, const GURL& url2,
                    bool *result);

}  // namespace brave

#endif  // BRAVE_COMMON_ORIGIN_HELPER_H

