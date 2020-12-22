/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/network_service_network_delegate.h"

#include "net/url_request/url_request.h"
#include "services/network/cookie_manager.h"
#include "services/network/url_loader.h"
#include "url/gurl.h"

#if !defined(OS_IOS)
#include "services/network/websocket.h"
#endif

namespace network {

bool NetworkServiceNetworkDelegate::OnCanGetCookies(
    const net::URLRequest& request,
    bool allowed_from_caller) {
  bool allowed =
      allowed_from_caller &&
      network_context_->cookie_manager()
          ->cookie_settings()
          .IsCookieAccessOrEphemeralCookiesAccessAllowed(
              request.url(), request.site_for_cookies().RepresentativeUrl(),
              request.isolation_info().top_frame_origin());

  if (!allowed)
    return false;

  URLLoader* url_loader = URLLoader::ForRequest(request);
  if (url_loader)
    return url_loader->AllowCookies(request.url(), request.site_for_cookies());
#if !defined(OS_IOS)
  WebSocket* web_socket = WebSocket::ForRequest(request);
  if (web_socket)
    return web_socket->AllowCookies(request.url());
#endif  // !defined(OS_IOS)
  return true;
}

bool NetworkServiceNetworkDelegate::OnCanSetCookie(
    const net::URLRequest& request,
    const net::CanonicalCookie& cookie,
    net::CookieOptions* options,
    bool allowed_from_caller) {
  bool allowed =
      allowed_from_caller &&
      network_context_->cookie_manager()
          ->cookie_settings()
          .IsCookieAccessOrEphemeralCookiesAccessAllowed(
              request.url(), request.site_for_cookies().RepresentativeUrl(),
              request.isolation_info().top_frame_origin());
  if (!allowed)
    return false;
  URLLoader* url_loader = URLLoader::ForRequest(request);
  if (url_loader)
    return url_loader->AllowCookies(request.url(), request.site_for_cookies());
#if !defined(OS_IOS)
  WebSocket* web_socket = WebSocket::ForRequest(request);
  if (web_socket)
    return web_socket->AllowCookies(request.url());
#endif  // !defined(OS_IOS)
  return true;
}

}  // namespace network

#define OnCanGetCookies OnCanGetCookiesWithoutEphemeralCookies
#define OnCanSetCookie OnCanSetCookieWithoutEphemeralCookies

#include "../../../../../services/network/network_service_network_delegate.cc"

#undef OnCanSetCookie
#undef OnCanGetCookies
