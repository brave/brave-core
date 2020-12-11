/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/restricted_cookie_manager.h"

#include "net/base/features.h"
#include "net/cookies/cookie_monster.h"

namespace {

bool ShouldUseEphemeralStorage(const GURL& url,
                               const url::Origin& top_frame_origin) {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return false;
  if (url::Origin::Create(url) == top_frame_origin)
    return false;
  return true;
}

}  // namespace

#define BRAVE_GETALLFORURL                                                  \
  if (ShouldUseEphemeralStorage(url, top_frame_origin)) {                   \
    static_cast<net::CookieMonster*>(cookie_store_)                         \
        ->GetEphemeralCookieListWithOptionsAsync(                           \
            url, top_frame_origin.GetURL(), net_options,                    \
            base::BindOnce(                                                 \
                &RestrictedCookieManager::CookieListToGetAllForUrlCallback, \
                weak_ptr_factory_.GetWeakPtr(), url, site_for_cookies,      \
                top_frame_origin, net_options, std::move(options),          \
                std::move(callback)));                                      \
  } else  // NOLINT

#define BRAVE_SETCANONICALCOOKIE                                               \
  if (ShouldUseEphemeralStorage(url, top_frame_origin)) {                      \
    static_cast<net::CookieMonster*>(cookie_store_)                            \
        ->SetEphemeralCanonicalCookieAsync(                                    \
            std::move(sanitized_cookie), origin_.GetURL(),                     \
            top_frame_origin.GetURL(), options,                                \
            base::BindOnce(&RestrictedCookieManager::SetCanonicalCookieResult, \
                           weak_ptr_factory_.GetWeakPtr(), url,                \
                           site_for_cookies, cookie_copy, options,             \
                           std::move(callback)));                              \
  } else  // NOLINT

#include "../../../../../services/network/restricted_cookie_manager.cc"
