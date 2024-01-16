// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_service_keys/service_key_utils.h"

// #include "base/containers/flat_map.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_service_keys/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_service_keys {

TEST(BraveServicesUtilsUnittest, GetDigestHeader) {
  // Test vector is from
  // https://www.ietf.org/archive/id/draft-ietf-httpbis-digest-headers-04.html#section-10.4
  const auto& header = GetDigestHeader("{\"hello\": \"world\"}");
  EXPECT_EQ(header.first, "digest");
  EXPECT_EQ(header.second,
            "SHA-256=X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE=");
}

TEST(BraveServicesUtilsUnittest, CreateSignatureString) {
  base::flat_map<std::string, std::string> headers = {
      {"digest", "SHA-256=X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE="},
      {"content-type", "application/json"},
      {"host", "example.com"}};
  // Test for no headers
  auto result = CreateSignatureString(headers, {});
  EXPECT_EQ(result.first, "");
  EXPECT_EQ(result.second, "");

  // Test for single header
  result = CreateSignatureString(headers, {"digest"});
  EXPECT_EQ(result.first, "digest");
  EXPECT_EQ(result.second,
            "digest: SHA-256=X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE=");

  // Test for multiple headers in specified order
  result = CreateSignatureString(headers, {"content-type", "digest"});
  EXPECT_EQ(result.first, "content-type digest");
  EXPECT_EQ(result.second,
            "content-type: application/json\ndigest: "
            "SHA-256=X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE=");

  // Test for multiple headers in reverse order
  result = CreateSignatureString(headers, {"digest", "content-type"});
  EXPECT_EQ(result.first, "digest content-type");
  EXPECT_EQ(result.second,
            "digest: "
            "SHA-256=X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE=\ncontent-"
            "type: application/json");
}

TEST(BraveServicesUtilsUnittest, GetAuthorizationHeader) {
  const auto& digest_header = GetDigestHeader("{\"hello\": \"world\"}");
  base::flat_map<std::string, std::string> headers;
  headers[digest_header.first] = digest_header.second;
  const std::string service_key =
      "bacfb4d7e93c6df045f66fa4bf438402b43ba2c9e3ce9b4eef470d24e32378e8";
  auto result = GetAuthorizationHeader(service_key, headers,
                                       GURL("example.com"), "POST", {"digest"});
  ASSERT_TRUE(result);
  EXPECT_EQ(result->first, "authorization");
  EXPECT_EQ(
      result->second,
      base::StrCat({"Signature keyId=\"", BUILDFLAG(KEY_ID),
                    "\",algorithm=\"hs2019\",headers=\"digest\",signature=\""
                    "jumtKp4LQDzIBpuGKIEI/mxrr9AEcSzvRGD6PfYyAq8=\""}));
}

}  // namespace brave_service_keys
