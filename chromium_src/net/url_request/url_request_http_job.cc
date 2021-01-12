/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_http_job.h"

#include "base/bind.h"
#include "net/base/features.h"
#include "net/base/isolation_info.h"
#include "net/cookies/cookie_monster.h"
#include "net/url_request/url_request.h"

namespace {

bool ShouldUseEphemeralStorage(net::URLRequest* request) {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return false;

  const net::IsolationInfo& isolation_info = request->isolation_info();
  if (!isolation_info.top_frame_origin().has_value() ||
      !isolation_info.frame_origin().has_value())
    return false;
  if (*isolation_info.top_frame_origin() == *isolation_info.frame_origin())
    return false;

  if (url::Origin::Create(request->url()) == *isolation_info.top_frame_origin())
    return false;

  return true;
}

}  // namespace

#define BRAVE_ADDCOOKIEHEADERANDSTART                                          \
  bool use_ephemeral_storage = ShouldUseEphemeralStorage(request_);            \
  if (use_ephemeral_storage) {                                                 \
    net::CookieInclusionStatus returned_status;                                \
    auto cookie = net::CanonicalCookie::Create(                                \
        request_->url(), "a=a", base::Time::Now(), base::nullopt,              \
        &returned_status);                                                     \
                                                                               \
    auto top_frame_url =                                                       \
        request_->isolation_info().top_frame_origin()->GetURL();               \
    auto cookie_1p = net::CanonicalCookie::CreateSanitizedCookie(              \
        top_frame_url, cookie->Name(), cookie->Value(), cookie->Domain(),      \
        cookie->Path(), cookie->CreationDate(), cookie->ExpiryDate(),          \
        cookie->LastAccessDate(), cookie->IsSecure(), cookie->IsHttpOnly(),    \
        cookie->SameSite(), cookie->Priority(), cookie->IsSameParty());        \
                                                                               \
    net::CookieOptions::SameSiteCookieContext same_site_context =              \
        net::cookie_util::ComputeSameSiteContextForResponse(                   \
            top_frame_url, request_->site_for_cookies(),                       \
            request_->initiator(), false);                                     \
    net::CookieOptions options_1p = CreateCookieOptions(same_site_context);    \
                                                                               \
    if (!(CanSetCookie(*cookie_1p, &options_1p) &&                             \
          !CanSetCookie(*cookie, &options)))                                   \
      use_ephemeral_storage = false;                                           \
  }                                                                            \
  if (use_ephemeral_storage)                                                   \
    static_cast<CookieMonster*>(cookie_store)                                  \
        ->GetEphemeralCookieListWithOptionsAsync(                              \
            request_->url(),                                                   \
            request()->isolation_info().top_frame_origin()->GetURL(), options, \
            base::BindOnce(&URLRequestHttpJob::SetCookieHeaderAndStart,        \
                           weak_factory_.GetWeakPtr(), options));              \
  else

#define BRAVE_SAVECOOKIESANDNOTIFYHEADERSCOMPLETE                              \
  bool use_ephemeral_storage = ShouldUseEphemeralStorage(request_);            \
  if (use_ephemeral_storage) {                                                 \
    auto top_frame_url =                                                       \
        request_->isolation_info().top_frame_origin()->GetURL();               \
    auto cookie_1p = net::CanonicalCookie::CreateSanitizedCookie(              \
        top_frame_url, cookie->Name(), cookie->Value(), cookie->Domain(),      \
        cookie->Path(), cookie->CreationDate(), cookie->ExpiryDate(),          \
        cookie->LastAccessDate(), cookie->IsSecure(), cookie->IsHttpOnly(),    \
        cookie->SameSite(), cookie->Priority(), cookie->IsSameParty());        \
                                                                               \
    net::CookieOptions::SameSiteCookieContext same_site_context =              \
        net::cookie_util::ComputeSameSiteContextForResponse(                   \
            top_frame_url, request_->site_for_cookies(),                       \
            request_->initiator(), false);                                     \
    net::CookieOptions options_1p = CreateCookieOptions(same_site_context);    \
                                                                               \
    if (!(CanSetCookie(*cookie_1p, &options_1p) &&                             \
          !CanSetCookie(*cookie, &options)))                                   \
      use_ephemeral_storage = false;                                           \
  }                                                                            \
  if (use_ephemeral_storage)                                                   \
    static_cast<CookieMonster*>(request_->context()->cookie_store())           \
        ->SetEphemeralCanonicalCookieAsync(                                    \
            std::move(cookie), request_->url(),                                \
            request()->isolation_info().top_frame_origin()->GetURL(), options, \
            base::BindOnce(&URLRequestHttpJob::OnSetCookieResult,              \
                           weak_factory_.GetWeakPtr(), options,                \
                           cookie_to_return, cookie_string));                  \
  else

#include "../../../../../net/url_request/url_request_http_job.cc"
