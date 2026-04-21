/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/content/webcat_body_handler.h"

#include "base/base64url.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace webcat {

namespace {

std::string Base64UrlSha256(const std::string& content) {
  auto digest = crypto::SHA256(base::as_byte_span(content));
  std::string encoded;
  base::Base64UrlEncode(base::as_chars(digest),
                        base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded);
  return encoded;
}

Manifest MakeManifestWithFiles(
    std::map<std::string, std::string> files) {
  Manifest m;
  m.app = "https://app.eth";
  m.version = "1.0.0";
  m.default_csp = "default-src 'self'; object-src 'none'";
  m.default_index = "/index.html";
  m.default_fallback = "/index.html";
  m.files = std::move(files);
  return m;
}

}  // namespace

class WebcatBodyHandlerTest : public testing::Test {
 protected:
  Manifest manifest_ = MakeManifestWithFiles({
      {"/index.html", Base64UrlSha256("<html>hello</html>")},
      {"/app.js", Base64UrlSha256("console.log('app');")},
  });
};

TEST_F(WebcatBodyHandlerTest, NormalizePathRootReturnsDefaultIndex) {
  WebcatBodyHandler handler(manifest_);
  std::string result = handler.NormalizePath("/");
  EXPECT_EQ(result, "/index.html");
}

TEST_F(WebcatBodyHandlerTest, NormalizePathEmptyReturnsDefaultIndex) {
  WebcatBodyHandler handler(manifest_);
  std::string result = handler.NormalizePath("");
  EXPECT_EQ(result, "/index.html");
}

TEST_F(WebcatBodyHandlerTest, NormalizePathTrailingSlashWithDefaultIndex) {
  WebcatBodyHandler handler(manifest_);
  std::string result = handler.NormalizePath("/admin/");
  EXPECT_EQ(result, "/index.html");
}

TEST_F(WebcatBodyHandlerTest, NormalizePathExactMatch) {
  WebcatBodyHandler handler(manifest_);
  std::string result = handler.NormalizePath("/app.js");
  EXPECT_EQ(result, "/app.js");
}

TEST_F(WebcatBodyHandlerTest, NormalizePathUnknownFallsToDefaultFallback) {
  WebcatBodyHandler handler(manifest_);
  std::string result = handler.NormalizePath("/unknown.css");
  EXPECT_EQ(result, "/index.html");
}

TEST_F(WebcatBodyHandlerTest, GetExpectedHashFound) {
  WebcatBodyHandler handler(manifest_);
  std::string hash = handler.GetExpectedHash("/index.html");
  EXPECT_EQ(hash, Base64UrlSha256("<html>hello</html>"));
}

TEST_F(WebcatBodyHandlerTest, GetExpectedHashNotFound) {
  WebcatBodyHandler handler(manifest_);
  std::string hash = handler.GetExpectedHash("/nonexistent.css");
  EXPECT_TRUE(hash.empty());
}

TEST_F(WebcatBodyHandlerTest, GetEffectiveCspFromDefault) {
  WebcatBodyHandler handler(manifest_);
  std::string csp = GetEffectiveCspForPath(
      "/app.js", manifest_.default_csp, manifest_.extra_csp);
  EXPECT_EQ(csp, "default-src 'self'; object-src 'none'");
}

TEST_F(WebcatBodyHandlerTest, GetEffectiveCspWithExtraCsp) {
  Manifest m = MakeManifestWithFiles({
      {"/index.html", "hash1"},
      {"/admin/panel.html", "hash2"},
  });
  m.extra_csp["/admin/"] = "default-src 'self'; script-src 'none'";

  std::string csp = GetEffectiveCspForPath(
      "/admin/panel.html", m.default_csp, m.extra_csp);
  EXPECT_EQ(csp, "default-src 'self'; script-src 'none'");
}

TEST_F(WebcatBodyHandlerTest, IsTransformerReturnsFalse) {
  WebcatBodyHandler handler(manifest_);
  EXPECT_FALSE(handler.IsTransformer());
}

}  // namespace webcat