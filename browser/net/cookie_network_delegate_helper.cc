/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/cookie_network_delegate_helper.h"

#include "brave/common/brave_cookie_blocking.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace brave {

bool OnCanGetCookiesForBraveShields(std::shared_ptr<BraveRequestInfo> ctx) {
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  bool allow_google_auth = prefs->GetBoolean(kGoogleLoginControlType);
  return !ShouldBlockCookie(ctx->allow_brave_shields, ctx->allow_1p_cookies,
    ctx->allow_3p_cookies, ctx->tab_origin, ctx->request_url,
    allow_google_auth);
}

bool OnCanSetCookiesForBraveShields(std::shared_ptr<BraveRequestInfo> ctx) {
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  bool allow_google_auth = prefs->GetBoolean(kGoogleLoginControlType);
  return !ShouldBlockCookie(ctx->allow_brave_shields, ctx->allow_1p_cookies,
    ctx->allow_3p_cookies, ctx->tab_origin, ctx->request_url,
    allow_google_auth);
}

}  // namespace brave
