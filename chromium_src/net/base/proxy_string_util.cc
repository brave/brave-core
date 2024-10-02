/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/proxy_string_util.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "url/third_party/mozilla/url_parse.h"

namespace net {
namespace {

// Based on FromSchemeHostAndPort() from proxy_server.cc, but to consider auth
// information when creating a ProxyServer, instead of bailing out.
ProxyServer CreateProxyServerWithAuthInfo(const ProxyServer::Scheme& scheme,
                                          std::string_view host_and_port) {
  url::Component username_component;
  url::Component password_component;
  url::Component hostname_component;
  url::Component port_component;
  url::ParseAuthority(host_and_port.data(),
                      url::Component(0, host_and_port.size()),
                      &username_component, &password_component,
                      &hostname_component, &port_component);

  std::string_view hostname =
      host_and_port.substr(hostname_component.begin, hostname_component.len);
  if (port_component.is_valid() && !port_component.is_nonempty())
    return ProxyServer();
  std::string_view port =
      port_component.is_nonempty()
          ? host_and_port.substr(port_component.begin, port_component.len)
          : "";

  // Prepend AUTH info to hostname if needed before creating the ProxyServer.
  std::string auth_hostname(hostname);
  std::string auth_str;
  if (username_component.is_valid()) {
    auth_str = std::string(
        host_and_port.substr(username_component.begin, username_component.len));
    if (password_component.is_valid()) {
      auth_str = base::StrCat({auth_str, ":",
                               host_and_port.substr(password_component.begin,
                                                    password_component.len)});
    }
    auth_hostname = base::StrCat({auth_str, "@", auth_hostname});
  }

  return ProxyServer::FromSchemeHostAndPort(scheme, auth_hostname, port);
}

std::string GetProxyServerAuthString(const ProxyServer& proxy_server) {
  std::string auth_string;
  const HostPortPair& host_port_pair = proxy_server.host_port_pair();
  if (!host_port_pair.username().empty()) {
    auth_string = host_port_pair.username();
    if (!host_port_pair.password().empty()) {
      base::StrAppend(&auth_string, {":", host_port_pair.password()});
    }
    base::StrAppend(&auth_string, {"@"});
  }
  return auth_string;
}

}  // namespace

// Declaring this function prototype is necessary, as the function is referenced
// in the translation unit before its declaration, which breaks the substitution
// below without this definition.
std::string ProxyServerToPacResultElement_ChromiumImpl(
    const ProxyServer& proxy_server);

}  // namespace net

#define ProxyServerToProxyUri ProxyServerToProxyUri_ChromiumImpl
#define ProxyServerToPacResultElement ProxyServerToPacResultElement_ChromiumImpl

#define ParseAuthority(HOST_AND_PORT, AUTH, USER, PASS, HOST, PORT)            \
  ParseAuthority(host_and_port.data(),                                         \
                 url::Component(0, host_and_port.size()), &username_component, \
                 &password_component, &hostname_component, &port_component);   \
  url::ParseAuthority(HOST_AND_PORT, AUTH, USER, PASS, HOST, PORT);            \
  if (!hostname_component.is_nonempty())                                       \
    return ProxyServer();                                                      \
  else                                                                         \
    return CreateProxyServerWithAuthInfo(scheme, host_and_port);

#include "src/net/base/proxy_string_util.cc"

#undef ParseAuthority
#undef ProxyServerToPacResultElement
#undef ProxyServerToProxyUri

namespace net {

std::string ProxyServerToProxyUri(const ProxyServer& proxy_server) {
  std::string proxy_uri = ProxyServerToProxyUri_ChromiumImpl(proxy_server);

  // We only inject AUTH information for SOCKS5 proxies (Tor-only).
  if (proxy_server.scheme() != ProxyServer::SCHEME_SOCKS5)
    return proxy_uri;

  std::string scheme_prefix;
  size_t colon = proxy_uri.find(':');
  if (colon != std::string::npos && proxy_uri.size() - colon >= 3 &&
      proxy_uri[colon + 1] == '/' && proxy_uri[colon + 2] == '/') {
    scheme_prefix = proxy_uri.substr(0, colon + 3);
    proxy_uri = proxy_uri.substr(colon + 3);  // Skip past the "://"
  }

  std::string result = base::StrCat(
      {scheme_prefix, GetProxyServerAuthString(proxy_server), proxy_uri});
  return result;
}

std::string ProxyServerToPacResultElement(const ProxyServer& proxy_server) {
  std::string proxy_pac =
      ProxyServerToPacResultElement_ChromiumImpl(proxy_server);

  // We only inject AUTH information for SOCKS5 proxies (Tor-only).
  if (proxy_server.scheme() != ProxyServer::SCHEME_SOCKS5)
    return proxy_pac;

  std::string scheme_prefix;
  size_t space = proxy_pac.find(' ');
  if (space != std::string::npos && proxy_pac.size() - space >= 1) {
    scheme_prefix = proxy_pac.substr(0, space + 1);
    proxy_pac = proxy_pac.substr(space + 1);  // Skip past the space
  }

  std::string result = base::StrCat(
      {scheme_prefix, GetProxyServerAuthString(proxy_server), proxy_pac});
  return result;
}

}  // namespace net
