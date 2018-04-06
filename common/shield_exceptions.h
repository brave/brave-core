/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

class GURL;

namespace brave {

bool IsEmptyDataURLRedirect(const GURL& gurl);
bool IsUAWhitelisted(const GURL& gurl);
bool IsBlockedResource(const GURL& gurl);
bool IsWhitelistedCookieExeption(const GURL& firstPartyOrigin,
                                 const GURL& subresourceUrl);
bool IsWhitelistedReferer(const GURL& firstPartyOrigin,
                          const GURL& subresourceUrl);

}  // namespace brave
