/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/bundle_parser.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace webcat {

namespace {

const char kValidBundle[] = R"({
  "manifest": {
    "app": "https://app.eth",
    "version": "1.0.0",
    "default_csp": "default-src 'self'; object-src 'none'",
    "default_index": "/index.html",
    "default_fallback": "/index.html",
    "files": {
      "/index.html": "abcdef123456",
      "/app.js": "ghijkl789012"
    },
    "wasm": []
  },
  "signatures": {
    "pubkey1": "sig1"
  }
})";

const char kValidBundleWithExtraCsp[] = R"({
  "manifest": {
    "app": "https://app.eth",
    "version": "1.0.0",
    "default_csp": "default-src 'self'; object-src 'none'",
    "extra_csp": {
      "/admin/": "default-src 'self'; script-src 'none'"
    },
    "default_index": "/index.html",
    "default_fallback": "/index.html",
    "files": {
      "/index.html": "abcdef123456",
      "/app.js": "ghijkl789012"
    },
    "wasm": ["wasmhash1"]
  },
  "signatures": {
    "pubkey1": "sig1"
  }
})";

}  // namespace

TEST(BundleParserTest, ParseValidBundle) {
  auto result = ParseBundle(kValidBundle);
  EXPECT_TRUE(result.error == WebcatError::kNone);
  ASSERT_TRUE(result.bundle.has_value());

  const auto& bundle = *result.bundle;
  EXPECT_EQ(bundle.manifest.app, "https://app.eth");
  EXPECT_EQ(bundle.manifest.version, "1.0.0");
  EXPECT_EQ(bundle.manifest.default_csp,
            "default-src 'self'; object-src 'none'");
  EXPECT_EQ(bundle.manifest.default_index, "/index.html");
  EXPECT_EQ(bundle.manifest.default_fallback, "/index.html");
  EXPECT_EQ(bundle.manifest.files.size(), 2u);
  EXPECT_TRUE(bundle.manifest.files.contains("/index.html"));
  EXPECT_TRUE(bundle.manifest.files.contains("/app.js"));
  EXPECT_EQ(bundle.signatures.size(), 1u);
  EXPECT_TRUE(bundle.signatures.contains("pubkey1"));
}

TEST(BundleParserTest, ParseValidBundleWithExtraCsp) {
  auto result = ParseBundle(kValidBundleWithExtraCsp);
  EXPECT_TRUE(result.error == WebcatError::kNone);
  ASSERT_TRUE(result.bundle.has_value());

  const auto& bundle = *result.bundle;
  EXPECT_EQ(bundle.manifest.extra_csp.size(), 1u);
  EXPECT_TRUE(bundle.manifest.extra_csp.contains("/admin/"));
  EXPECT_EQ(bundle.manifest.wasm.size(), 1u);
  EXPECT_EQ(bundle.manifest.wasm[0], "wasmhash1");
}

TEST(BundleParserTest, ParseInvalidJson) {
  auto result = ParseBundle("not json");
  EXPECT_EQ(result.error, WebcatError::kBundleParseError);
  EXPECT_FALSE(result.bundle.has_value());
}

TEST(BundleParserTest, ParseMissingManifest) {
  std::string json = R"({"signatures": {"k": "v"}})";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kBundleParseError);
}

TEST(BundleParserTest, ParseMissingSignatures) {
  std::string json = R"({"manifest": {"app": "a", "version": "1", "default_csp": "c", "default_index": "/i", "default_fallback": "/i", "files": {"/i": "h"}}})";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kBundleParseError);
}

TEST(BundleParserTest, ParseEmptyFiles) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_index": "/index.html",
      "default_fallback": "/index.html",
      "files": {}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseMissingDefaultCsp) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_index": "/index.html",
      "default_fallback": "/index.html",
      "files": {"/index.html": "hash1"}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseMissingDefaultIndex) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_fallback": "/index.html",
      "files": {"/index.html": "hash1"}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseMissingDefaultFallback) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_index": "/index.html",
      "files": {"/index.html": "hash1"}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseDefaultIndexNotInFiles) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_index": "/missing.html",
      "default_fallback": "/index.html",
      "files": {"/index.html": "hash1"}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseDefaultFallbackNotInFiles) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_index": "/index.html",
      "default_fallback": "/missing.html",
      "files": {"/index.html": "hash1"}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseFilePathWithoutLeadingSlash) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_index": "/index.html",
      "default_fallback": "/index.html",
      "files": {"no-slash.html": "hash1"}
    },
    "signatures": {"k": "v"}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kManifestStructureInvalid);
}

TEST(BundleParserTest, ParseSignatureNotString) {
  std::string json = R"({
    "manifest": {
      "app": "https://app.eth",
      "version": "1.0.0",
      "default_csp": "default-src 'self'",
      "default_index": "/index.html",
      "default_fallback": "/index.html",
      "files": {"/index.html": "hash1"}
    },
    "signatures": {"pubkey1": 123}
  })";
  auto result = ParseBundle(json);
  EXPECT_EQ(result.error, WebcatError::kBundleParseError);
}

}  // namespace webcat