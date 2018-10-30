#include "brave/common/brave_cookie_blocking.h"

#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

bool ShouldBlockCookie(bool allow_brave_shields, bool allow_1p_cookies,
    bool allow_3p_cookies, const GURL& primary_url, const GURL& url) {

  if (primary_url.SchemeIs("chrome-extension")) {
    return false;
  }

  if (!allow_brave_shields) {
    return false;
  }

  // If 1p cookies are not allowed, then we just want to block everything.
  if (!allow_1p_cookies) {
    return true;
  }

  // If 3p is allowed, we have nothing extra to block
  if (allow_3p_cookies) {
    return false;
  }

  // Same TLD+1 whouldn't set the referrer
  return !SameDomainOrHost(url, primary_url,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}
