/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/proxy_server.h"

#include "net/base/url_util.h"

namespace {

// We need to call this before url::CanonicalizeHost() to extract the username
// and password needed to create the HostPortPair later on, as well as to be
// able to pass the hostname only to url::CanonicalizeHost(), or it will fail.
void ParseAuthInfoAndHostname(base::StringPiece* hostname,
                              base::StringPiece* username,
                              base::StringPiece* password) {
  url::Component user_component, password_component;
  url::Component host_component, port_component;
  url::ParseAuthority(hostname->data(), url::Component(0, hostname->size()),
                      &user_component, &password_component, &host_component,
                      &port_component);

  // If host is not valid then extracting auth is meaningless since
  // url::CanonicalizeHost in ProxyServer::FromSchemeHostAndPort will fail
  // anyway.
  if (!host_component.is_valid())
    return;

  // Extract Auth info if it exists.
  if (user_component.is_valid())
    *username = hostname->substr(user_component.begin, user_component.len);
  if (password_component.is_valid())
    *password =
        hostname->substr(password_component.begin, password_component.len);

  // Make sure the hostname StringPiece only contains the hostname.
  *hostname = hostname->substr(host_component.begin, host_component.len);
}

}  // namespace

#define BRAVE_PROXY_SERVER_FROM_SCHEME_HOST_AND_PORT_EXTRACT_AUTH_INFO \
  base::StringPiece username, password;                                \
  ParseAuthInfoAndHostname(&host, &username, &password);

#define BRAVE_PROXY_SERVER_FROM_SCHEME_HOST_AND_PORT_RETURN_HOST_PORT_PAIR     \
  return ProxyServer(                                                          \
      scheme, HostPortPair(username, password, unbracketed_host, fixed_port)); \
  if (false)

#include "src/net/base/proxy_server.cc"

#undef BRAVE_PROXY_SERVER_FROM_SCHEME_HOST_AND_PORT_RETURN_HOST_PORT_PAIR
#undef BRAVE_PROXY_SERVER_FROM_SCHEME_HOST_AND_PORT_EXTRACT_AUTH_INFO
