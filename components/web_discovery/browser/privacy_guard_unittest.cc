/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/privacy_guard.h"

#include "brave/components/web_discovery/browser/patterns.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

class WebDiscoveryPrivacyGuardTest : public testing::Test {
 public:
  ~WebDiscoveryPrivacyGuardTest() override = default;

  // testing::Test:
  void SetUp() override {
    search_engine_pattern_.is_search_engine = true;
    search_engine_pattern_.search_template_prefix = "find?testquery=";
  }

 protected:
  PatternsURLDetails search_engine_pattern_;
};

TEST_F(WebDiscoveryPrivacyGuardTest, ShouldDropURL) {
  // Test cases: URL string and expected result (true = should drop)
  constexpr auto kShouldDropURLTestCases = std::to_array<
      std::pair<std::string_view, bool>>({
      // Valid URLs that should NOT be dropped
      {"https://www.search1.com/search?q=test", false},
      {"https://search2.com/search?query=testing+a+nice+query", false},
      {"https://search2.com/search?query=quick+brown+fox+jumped&country=us",
       false},
      {"https://www.website.com/page/test", false},
      {"http://example.com:80/path", false},    // Port 80 is allowed for HTTP
      {"https://example.com:443/path", false},  // Port 443 is allowed for HTTPS
      {"https://example.com/", false},          // Basic valid URL
      {"http://test.org/page", false},          // HTTP with standard port

      // Invalid URLs that SHOULD be dropped
      {"", true},                         // Empty/invalid URL
      {"not-a-url", true},                // Invalid URL format
      {"ftp://example.com/file", true},   // Non-HTTP/HTTPS scheme
      {"file:///etc/passwd", true},       // File scheme
      {"javascript:alert('xss')", true},  // JavaScript scheme
      {"chrome-extension://abcdefghijklmnopqrstuvwxyzabcdef/popup.html",
       true},  // Extension scheme

      // URLs with credentials (should be dropped)
      {"https://user:pass@website.com/page/test", true},
      {"http://admin:secret@example.com/", true},
      {"https://user@example.com/path", true},   // Username without password
      {"https://:password@example.com/", true},  // Password without username

      // URLs with non-standard ports (should be dropped)
      {"https://website.com:8443/page/test", true},
      {"http://example.com:8080/", true},
      {"https://test.com:3000/api", true},
      {"http://site.org:9999/page", true},

      // IP addresses and localhost (should be dropped)
      {"https://88.88.88.88/page/test", true},
      {"http://192.168.1.1/admin", true},
      {"https://127.0.0.1/test", true},
      {"http://10.0.0.1:8080/", true},
      {"https://localhost/page", true},
      {"http://localhost:3000/api", true},
      {"https://[::1]/ipv6", true},         // IPv6 localhost
      {"http://[2001:db8::1]/test", true},  // IPv6 address

      // Hostnames that are too long (should be dropped)
      {"https://thisisanextremelylonghostnamethatexceedslimits1234567890.com/",
       true},
  });

  for (const auto& [url_str, should_drop] : kShouldDropURLTestCases) {
    GURL test_url(url_str);
    EXPECT_EQ(ShouldDropURL(test_url), should_drop)
        << "Failed for URL: " << url_str;
  }
}

TEST_F(WebDiscoveryPrivacyGuardTest, IsPrivateQueryLikely) {
  // Test cases: query string and expected result (true = likely private)
  constexpr auto kIsPrivateQueryTestCases = std::to_array<
      std::pair<std::string_view, bool>>({
      // Safe queries that should NOT be considered private
      {"test", false},                      // Simple query
      {"99 cake recipes", false},           // Normal search with numbers
      {"grapefruit and pineapple", false},  // Common fruit search
      {"a quick brown fox", false},         // Short phrase
      {"how to cook pasta", false},         // Normal cooking query
      {"weather forecast today", false},    // Weather query
      {"best laptop 2025", false},          // Product search
      {"cat dog pig cow fox rat bat owl bee ant fly bug car bus van box cup",
       false},      // Many 3-letter words (17 words)
      {"", false},  // Empty query

      // Queries that are too long (> 120 chars)
      {"this is an extremely super long query that exceeds the absolute "
       "maximum allowed length of one hundred and twenty characters",
       true},

      // Queries with logograms that are under the 50-char threshold (should be
      // safe)
      {"这是一个中文查询语句测试", false},  // Chinese characters, 12 chars
      {"これは日本語クエリテスト", false},  // Japanese characters, 12 chars
      {"안녕하세요 이것은 한국어 문자가 포함된 긴 쿼리입니다",
       false},  // Korean characters, 29 chars

      // Queries with logograms that exceed the 50-char threshold (should be
      // private)
      {"这是一个包含中文字符的非常非常非常非常非常非常非常非常非常非常非常非常"
       "非常非常非常非常长的查询语句应该被认为是私有的因为它超过了五十个字符的"
       "限制",
       true},  // Chinese characters, 72 chars
      {"これは日本語文字を含む非常に非常に非常に非常に非常に非常に非常に非常に"
       "非常に非常に非常に非常に非常に非常に非常に長いクエリです私的と見なされ"
       "るべきです文字数制限",
       true},  // Japanese characters, 80 chars
      {"안녕하세요 이것은 한국어 문자가 포함된 매우 매우 매우 매우 매우 매우 "
       "매우 매우 매우 매우 매우 매우 매우 매우 매우 매우 긴 쿼리입니다 "
       "비공개로 간주되어야 합니다",
       true},  // Korean characters, 92 chars

      // Short queries with logograms (should be safe)
      {"中文", false},    // Short Chinese
      {"日本語", false},  // Short Japanese
      {"한국어", false},  // Short Korean

      // Queries with too many long words (> 16 long words)
      {"word1 word2 word3 word4 word5 word6 word7 word8 word9 word10 "
       "verylongword1 verylongword2 verylongword3 verylongword4 verylongword5 "
       "verylongword6 verylongword7 verylongword8 verylongword9 verylongword10 "
       "verylongword11 verylongword12 verylongword13 verylongword14 "
       "verylongword15 verylongword16 verylongword17",
       true},

      // Individual words that are too long (> 45 chars)
      {"Hippopotomonstrosesquippedaliophobiasymptomsss",
       true},  // Single very long word
      {"pneumonoultramicroscopicsilicovolcanoconiosisword",
       true},  // Medical term, too long

      // Long numbers (> 7 digits)
      {"search for aaa12345678901234", true},  // Long number sequence
      {"phone number 5551234567", true},       // Long phone number
      {"credit card 4111111111111111", true},  // Credit card number
      {"social security 123456789012", true},  // SSN-like number
      {"account 1234-5678-9012-3456", true},   // Long number with hyphens
      {"reference 98765_43210_11111_22222",
       true},  // Long number with underscores
      {"id 1111-2222_3333-4444_5555-6666",
       true},  // Long number with mixed hyphens and underscores

      // Valid numbers that should be allowed
      {"call 555-1234", false},   // Short phone number
      {"year 2025", false},       // Year
      {"price $99.99", false},    // Price
      {"ISSN 2049-3630", false},  // Valid ISSN format
      {"ISSN 2049-3631", true},   // Invalid ISSN checksum
      {"ISSN 2049-3630 1342",
       false},  // Valid ISSN with additional valid number
      {"ISSN 2049-3630 134223434",
       true},  // Valid ISSN with additional invalid number

      {"EAN 978–0141026626", false},  // Valid EAN-13 format
      {"EAN 978–0141026627", true},   // Invalid EAN-13 checksum

      // Email addresses
      {"contact me@testemail.com", true},            // Simple email
      {"send to user@example.org", true},            // Email in query
      {"my email is john.doe@company.co.uk", true},  // Email with explanation

      // HTTP credentials
      {"login with user:pass@site.com", true},     // Basic auth credentials
      {"access admin:secret@server", true},        // Admin credentials
      {"ftp://user:password@ftp.site.com", true},  // FTP credentials

      // Boundary cases for European long words (> 20 chars)
      {"antidisestablishmentarianism", false},  // Long English word (allowed)
      {"dichlorodifluoromethane", false},       // Chemical compound (allowed)
      {"Donaudampfschifffahrtsgesellschaftskapitän",
       false},  // Long German word (allowed)
  });

  for (const auto& [query_str, is_private] : kIsPrivateQueryTestCases) {
    EXPECT_EQ(IsPrivateQueryLikely(query_str), is_private)
        << "Failed for query: " << query_str;
  }
}

TEST_F(WebDiscoveryPrivacyGuardTest, GeneratePrivateSearchURL) {
  GURL original_url("https://example.com/search?q=aaa&country=us&f=1");

  EXPECT_EQ(
      GeneratePrivateSearchURL(original_url, "a simple test query",
                               search_engine_pattern_.search_template_prefix)
          .spec(),
      "https://example.com/find?testquery=a+simple+test+query");
  EXPECT_EQ(GeneratePrivateSearchURL(
                original_url, "another simple test query 123", std::nullopt)
                .spec(),
            "https://example.com/search?q=another+simple+test+query+123");
  EXPECT_EQ(
      GeneratePrivateSearchURL(original_url, "special chars @#$%^&=",
                               search_engine_pattern_.search_template_prefix)
          .spec(),
      "https://example.com/find?testquery=special+chars+%40%23%24%25%5E%26%3D");
}

TEST_F(WebDiscoveryPrivacyGuardTest, ShouldMaskURL) {
  // Test cases: URL string and expected result (true = should mask)
  constexpr auto kShouldMaskURLTestCases = std::to_array<
      std::pair<std::string_view, bool>>({
      // URLs that should NOT be masked
      {"https://www.search1.com/search?q=test", false},  // Normal search
      {"https://search2.com/search?query=testing+a+nice+query",
       false},  // Normal search
      {"https://search2.com/search?query=quick+fox&country=us&d=1",
       false},  // Normal search with params
      {"https://www.website.com/page/test", false},              // Simple path
      {"https://www.website.com/page/test?param=value", false},  // Simple query
      {"https://www.website.com/a/b/c/d/e/f/g/h",
       false},  // 8 path parts (at limit)
      {"https://www.website.com/page/test?a=1&b=2&c=3&d=4&e=5&f=6&g=7&h=8",
       false},  // 8 query params (at limit)

      // URLs that SHOULD be masked
      {"https://www.website.com/page/doc#fragment", true},  // Has fragment
      {"https://www.website.com/page/admin", true},         // Risky path part
      {"https://www.website.com/page/login", true},         // Risky path part
      {"https://www.website.com/page/edit", true},          // Risky path part
      {"https://www.website.com/page/weblogic",
       true},  // Risky path part (case insensitive)
      {"https://www.website.com/page/wp-admin", true},  // Risky path part
      {"https://www.website.com/a/b/c/d/e/f/g/h/i",
       true},  // 9 path parts (over limit)
      {"https://www.website.com/page/test?a=1&b=2&c=3&d=4&e=5&f=6&g=7&h=8&i=9",
       true},  // 9 query params (over limit)
      {"https://www.website.com/page/test?email=test@example.com",
       true},  // Email in query
      {"https://www.website.com/test@example.com/page", true},  // Email in path

      // Misc private URL regex patterns
      {"https://www.website.com/order/12345", true},     // Order path pattern
      {"https://www.website.com/shop/order/abc", true},  // Order path pattern
      {"https://www.website.com/auth/realms/master", true},  // Auth realms path
  });

  for (const auto& [url_str, should_mask] : kShouldMaskURLTestCases) {
    GURL test_url(url_str);
    EXPECT_EQ(ShouldMaskURL(test_url), should_mask)
        << "Failed for URL: " << url_str;
  }

  // Generate URL for length testing (single long path to avoid path parts
  // limit)
  std::string url_over_limit = "https://www.example.com/";
  for (int i = 0; i < 60; ++i) {
    url_over_limit += "verylongsegment";
  }

  // Create query string for query length testing (150 char limit)
  std::string query_over_limit = "https://www.example.com/search?q=";
  for (int i = 0; i < 20; ++i) {
    query_over_limit += "longparam";
  }

  // Test URL length limit (generated programmatically)
  EXPECT_TRUE(ShouldMaskURL(GURL(url_over_limit)))
      << "URL over 800 chars should be masked. Length: "
      << url_over_limit.length();

  // Test query string length limits
  EXPECT_TRUE(ShouldMaskURL(GURL(query_over_limit)))
      << "Query over 150 chars should be masked. Query length: "
      << query_over_limit.substr(query_over_limit.find('?') + 1).length();
}

TEST_F(WebDiscoveryPrivacyGuardTest, MaskURL) {
  // Test cases: input URL and expected output (nullopt = should be dropped)
  constexpr auto kMaskURLTestCases = std::to_array<
      std::pair<std::string_view, std::optional<std::string_view>>>({
      // Invalid URLs (should return nullopt)
      {"", std::nullopt},                        // Empty URL
      {"not-a-url", std::nullopt},               // Invalid URL format
      {"file:///etc/passwd", std::nullopt},      // File scheme
      {"ftp://example.com/file", std::nullopt},  // Non-HTTP/HTTPS scheme

      // URLs that should be dropped (should return nullopt)
      {"https://user:pass@example.com/page", std::nullopt},  // Has credentials
      {"https://127.0.0.1/page", std::nullopt},              // IP address
      {"https://localhost/page", std::nullopt},              // Localhost
      {"https://example.com:8080/page", std::nullopt},  // Non-standard port

      // URLs that should NOT be masked (input same as output)
      {"https://www.search1.com/search?q=test",
       "https://www.search1.com/search?q=test"},
      {"https://www.website.com/page/test",
       "https://www.website.com/page/test"},
      {"https://example.com/simple/path", "https://example.com/simple/path"},
      {"https://site.com/page?param=value",
       "https://site.com/page?param=value"},

      // URLs that SHOULD be masked (should return masked version)
      {"https://www.website.com/page/admin",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/page/login",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/page/edit",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/page#fragment",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/order/12345",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/auth/realms/master",
       "https://www.website.com/ (PROTECTED)"},
      {"https://example.com/test@email.com",
       "https://example.com/ (PROTECTED)"},
  });

  // Test cases for relaxed mode (path preservation)
  constexpr auto kMaskURLRelaxedTestCases = std::to_array<
      std::pair<std::string_view, std::optional<std::string_view>>>({
      // URLs with query/fragment that should preserve path in relaxed mode
      {"https://www.website.com/safe/path",
       "https://www.website.com/safe/path"},
      {"https://www.website.com/safe/path?query=value#fragment",
       "https://www.website.com/safe/path (PROTECTED)"},
      {"https://www.website.com/article/123?share=1234567890",
       "https://www.website.com/article/123 (PROTECTED)"},

      // URLs that still get fully masked even in relaxed mode (risky paths)
      {"https://www.website.com/admin?query=value",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/login#fragment",
       "https://www.website.com/ (PROTECTED)"},
      {"https://www.website.com/order/123?param=value",
       "https://www.website.com/ (PROTECTED)"},
  });

  // Test non-relaxed mode
  for (const auto& [input_url, expected_output] : kMaskURLTestCases) {
    GURL test_url(input_url);
    auto result = MaskURL(test_url, false);
    EXPECT_EQ(result, expected_output)
        << "Failed for URL (non-relaxed): " << input_url;
  }

  // Test relaxed mode
  for (const auto& [input_url, expected_output] : kMaskURLRelaxedTestCases) {
    GURL test_url(input_url);
    auto result = MaskURL(test_url, true);
    EXPECT_EQ(result, expected_output)
        << "Failed for URL (relaxed): " << input_url;
  }
}

}  // namespace web_discovery
