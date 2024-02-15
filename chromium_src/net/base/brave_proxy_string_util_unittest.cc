/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/proxy_string_util.h"

#include "net/base/proxy_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

TEST(BraveProxySpecificationUtilTest, ProxyUriWithAuthToProxyServer) {
  const struct {
    const char* const input_uri;
    const char* const expected_uri;
    ProxyServer::Scheme expected_scheme;
    const char* const expected_host;
    int expected_port;
    const char* const expected_username;
    const char* const expected_password;
    const char* const expected_pac_string;
  } tests[] = {
      {"socks5://foo:bar@foopy",  // No port.
       "socks5://foo:bar@foopy:1080", ProxyServer::SCHEME_SOCKS5, "foopy", 1080,
       "foo", "bar", "SOCKS5 foo:bar@foopy:1080"},
      {"socks5://baz:qux@foopy:10", "socks5://baz:qux@foopy:10",
       ProxyServer::SCHEME_SOCKS5, "foopy", 10, "baz", "qux",
       "SOCKS5 baz:qux@foopy:10"},
  };

  for (const auto& test : tests) {
    ProxyServer uri =
        ProxyUriToProxyServer(test.input_uri, ProxyServer::SCHEME_HTTP);
    EXPECT_TRUE(uri.is_valid());
    EXPECT_EQ(test.expected_uri, ProxyServerToProxyUri(uri));
    EXPECT_EQ(test.expected_scheme, uri.scheme());
    EXPECT_EQ(test.expected_host, uri.host_port_pair().host());
    EXPECT_EQ(test.expected_port, uri.host_port_pair().port());
    EXPECT_EQ(test.expected_username, uri.host_port_pair().username());
    EXPECT_EQ(test.expected_password, uri.host_port_pair().password());
    EXPECT_EQ(test.expected_pac_string, ProxyServerToPacResultElement(uri));
  }
}

TEST(BraveProxySpecificationUtilTest, PacResultElementWithAuthToProxyServer) {
  const struct {
    const char* const input_pac;
    const char* const expected_uri;
  } tests[] = {
      {
          "SOCKS5 foo:bar@foopy:10",
          "socks5://foo:bar@foopy:10",
      },
  };

  for (const auto& test : tests) {
    ProxyServer uri = PacResultElementToProxyServer(test.input_pac);
    EXPECT_TRUE(uri.is_valid());
    EXPECT_EQ(test.expected_uri, ProxyServerToProxyUri(uri));
  }
}

}  // namespace net
