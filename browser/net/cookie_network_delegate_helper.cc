/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/cookie_network_delegate_helper.h"

#include "brave/common/brave_cookie_blocking.h"

namespace brave {

bool OnCanGetCookiesForBraveShields(std::shared_ptr<BraveRequestInfo> ctx) {
  return !ShouldBlockCookie(ctx->allow_brave_shields, ctx->allow_1p_cookies,
    ctx->allow_3p_cookies, ctx->tab_origin, ctx->request_url);
}

bool OnCanSetCookiesForBraveShields(std::shared_ptr<BraveRequestInfo> ctx) {
  return !ShouldBlockCookie(ctx->allow_brave_shields, ctx->allow_1p_cookies,
    ctx->allow_3p_cookies, ctx->tab_origin, ctx->request_url);
}

}  // namespace brave
