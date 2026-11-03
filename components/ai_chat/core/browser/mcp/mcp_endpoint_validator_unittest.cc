/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_endpoint_validator.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

using Result = MCPEndpointValidationResult;

struct EndpointCase {
  const char* url;
  Result private_ips_disabled;
  Result private_ips_enabled;
};

constexpr EndpointCase kCases[] = {
    {"https://mcp.example.com/mcp", Result::kValid, Result::kValid},
    {"https://mcp.example.com:8443/mcp?key=v", Result::kValid, Result::kValid},
    {"https://8.8.8.8/mcp", Result::kValid, Result::kValid},
    {"https://localhost/mcp", Result::kValid, Result::kValid},
    {"https://sub.localhost/mcp", Result::kValid, Result::kValid},
    // The plain http exceptions.
    {"http://localhost:3000/mcp", Result::kValid, Result::kValid},
    {"http://127.0.0.1:3000/mcp", Result::kValid, Result::kValid},
    {"http://[::1]:3000/mcp", Result::kValid, Result::kValid},
    // Other private and loopback addresses, over either scheme.
    {"http://127.0.0.2/mcp", Result::kPrivateAddressNotAllowed, Result::kValid},
    {"http://[::ffff:127.0.0.1]/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"http://192.168.1.10/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"https://192.168.1.10/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"http://10.0.0.1/mcp", Result::kPrivateAddressNotAllowed, Result::kValid},
    {"https://172.16.0.1/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"http://169.254.169.254/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"http://[fe80::1]/mcp", Result::kPrivateAddressNotAllowed, Result::kValid},
    {"http://[fd00::1]/mcp", Result::kPrivateAddressNotAllowed, Result::kValid},
    {"https://[::ffff:10.0.0.1]/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    // Plain http to anything else.
    {"http://mcp.example.com/mcp", Result::kInsecureScheme,
     Result::kInsecureScheme},
    {"http://8.8.8.8/mcp", Result::kInsecureScheme, Result::kInsecureScheme},
    {"http://sub.localhost/mcp", Result::kInsecureScheme,
     Result::kInsecureScheme},
    // Hosts that only look local.
    {"http://localhost.evil.com/mcp", Result::kInsecureScheme,
     Result::kInsecureScheme},
    {"http://127.0.0.1.nip.io/mcp", Result::kInsecureScheme,
     Result::kInsecureScheme},
    {"http://localhost./mcp", Result::kInsecureScheme, Result::kInsecureScheme},
    {"https://localhost.evil.com/mcp", Result::kValid, Result::kValid},
    // Other spellings of IPv4 addresses are canonicalized first.
    {"http://2130706433/mcp", Result::kValid, Result::kValid},
    {"http://0177.0.0.1/mcp", Result::kValid, Result::kValid},
    {"http://0x7f.0.0.1/mcp", Result::kValid, Result::kValid},
    {"http://127.0.0.1./mcp", Result::kValid, Result::kValid},
    {"http://167772161/mcp", Result::kPrivateAddressNotAllowed, Result::kValid},
    {"https://012.0.0.1/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"https://0xc0.0xa8.0.1/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"https://10.0.0.1./mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"http://[::ffff:7f00:1]/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    {"https://100.64.0.1/mcp", Result::kPrivateAddressNotAllowed,
     Result::kValid},
    // Unspecified addresses.
    {"http://0.0.0.0/mcp", Result::kInvalidUrl, Result::kInvalidUrl},
    {"https://0.0.0.0/mcp", Result::kInvalidUrl, Result::kInvalidUrl},
    {"http://0/mcp", Result::kInvalidUrl, Result::kInvalidUrl},
    {"http://[::]/mcp", Result::kInvalidUrl, Result::kInvalidUrl},
    {"http://[::ffff:0.0.0.0]/mcp", Result::kInvalidUrl, Result::kInvalidUrl},
    {"https://[::ffff:0:0]/mcp", Result::kInvalidUrl, Result::kInvalidUrl},
    // Schemes are case-insensitive.
    {"HTTPS://MCP.EXAMPLE.COM/mcp", Result::kValid, Result::kValid},
    {"HTTP://LOCALHOST:3000/mcp", Result::kValid, Result::kValid},
    {"HTTP://MCP.EXAMPLE.COM/mcp", Result::kInsecureScheme,
     Result::kInsecureScheme},
    // Embedded credentials.
    {"https://user:pass@mcp.example.com/mcp", Result::kEmbeddedCredentials,
     Result::kEmbeddedCredentials},
    {"https://user@mcp.example.com/mcp", Result::kEmbeddedCredentials,
     Result::kEmbeddedCredentials},
    {"http://user:pass@localhost/mcp", Result::kEmbeddedCredentials,
     Result::kEmbeddedCredentials},
    // Other schemes.
    {"ws://localhost/mcp", Result::kUnsupportedScheme,
     Result::kUnsupportedScheme},
    {"wss://mcp.example.com/mcp", Result::kUnsupportedScheme,
     Result::kUnsupportedScheme},
    {"ftp://mcp.example.com/", Result::kUnsupportedScheme,
     Result::kUnsupportedScheme},
    {"file:///tmp/mcp", Result::kUnsupportedScheme, Result::kUnsupportedScheme},
    {"data:text/plain,mcp", Result::kUnsupportedScheme,
     Result::kUnsupportedScheme},
    {"", Result::kInvalidUrl, Result::kInvalidUrl},
    {"not a url", Result::kInvalidUrl, Result::kInvalidUrl},
};

class MCPEndpointValidatorTest : public testing::TestWithParam<bool> {
 public:
  MCPEndpointValidatorTest() {
    feature_list_.InitWithFeatureState(features::kAllowPrivateIPs, GetParam());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

}  // namespace

TEST_P(MCPEndpointValidatorTest, ValidateMCPEndpoint) {
  for (const EndpointCase& test_case : kCases) {
    SCOPED_TRACE(test_case.url);
    EXPECT_EQ(ValidateMCPEndpoint(GURL(test_case.url)),
              GetParam() ? test_case.private_ips_enabled
                         : test_case.private_ips_disabled);
  }
}

INSTANTIATE_TEST_SUITE_P(All,
                         MCPEndpointValidatorTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "PrivateIPsAllowed"
                                             : "PrivateIPsNotAllowed";
                         });

}  // namespace ai_chat
