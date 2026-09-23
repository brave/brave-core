/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/webui_config_map.h"

#include <memory>
#include <string_view>

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/scoped_web_ui_controller_factory_registration.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

class TestConfig : public WebUIConfig {
 public:
  explicit TestConfig(std::string_view host,
                      std::string_view scheme = kChromeUIUntrustedScheme)
      : WebUIConfig(scheme, host) {}
  ~TestConfig() override = default;

  // `browser_context` is ignored, so the tests below can pass nullptr.
  bool IsWebUIEnabled(BrowserContext* browser_context) override {
    return enabled;
  }

  bool ShouldHandleURL(const GURL& url) override {
    last_handled_url = url;
    return should_handle_url;
  }

  bool ShouldHandleSubdomains() const override {
    return should_handle_subdomains;
  }

  std::unique_ptr<WebUIController> CreateWebUIController(
      WebUI* web_ui,
      const GURL& url) override {
    // Unused in these tests.
    return nullptr;
  }

  bool enabled = true;
  bool should_handle_url = true;
  // Matches the upstream WebUIConfig default.
  bool should_handle_subdomains = false;
  GURL last_handled_url;
};

// Returns a config for `host` that opts into handling its subdomains.
std::unique_ptr<TestConfig> MakeSubdomainConfig(
    std::string_view host,
    std::string_view scheme = kChromeUIUntrustedScheme) {
  auto config = std::make_unique<TestConfig>(host, scheme);
  config->should_handle_subdomains = true;
  return config;
}

}  // namespace

// A config that opts in via ShouldHandleSubdomains() resolves both its exact
// origin and its subdomains at any depth, while unrelated hosts do not match.
TEST(BraveWebUIConfigSubdomainTest, OptedInConfigHandlesSubdomains) {
  auto& map = WebUIConfigMap::GetInstance();
  ScopedWebUIConfigRegistration registration(MakeSubdomainConfig("workspaces"));

  auto* config = map.GetConfig(nullptr, GURL("chrome-untrusted://workspaces"));
  ASSERT_TRUE(config);

  // Single-label subdomains route to the same config.
  EXPECT_EQ(config, map.GetConfig(
                        nullptr, GURL("chrome-untrusted://abc123.workspaces")));
  EXPECT_EQ(config,
            map.GetConfig(nullptr, GURL("chrome-untrusted://"
                                        "00000000-1111-2222-3333-444444444444."
                                        "workspaces")));

  // So do deeper subdomains, whose intermediate hosts have no config of their
  // own: the lookup walks up to the nearest registered ancestor.
  EXPECT_EQ(config,
            map.GetConfig(nullptr, GURL("chrome-untrusted://a.b.workspaces")));

  // The opted-in host must be the suffix, not a subdomain of the request.
  EXPECT_EQ(nullptr, map.GetConfig(
                         nullptr, GURL("chrome-untrusted://workspaces.other")));
}

// How a config restricts itself to the subdomain shapes it actually serves.
TEST(BraveWebUIConfigSubdomainTest, SubdomainRejectedByShouldHandleURL) {
  auto& map = WebUIConfigMap::GetInstance();
  auto owned_config = MakeSubdomainConfig("picky");
  owned_config->should_handle_url = false;
  ScopedWebUIConfigRegistration registration(std::move(owned_config));

  EXPECT_EQ(nullptr,
            map.GetConfig(nullptr, GURL("chrome-untrusted://sub.picky")));
}

// The nearest registered ancestor wins, and its opt-in alone decides whether
// the subdomain resolves: a closer ancestor that has not opted in must not be
// bypassed in favour of one further up that has.
TEST(BraveWebUIConfigSubdomainTest, NearestAncestorConfigWins) {
  auto& map = WebUIConfigMap::GetInstance();
  ScopedWebUIConfigRegistration outer(MakeSubdomainConfig("outer"));
  // ShouldHandleSubdomains() is left at its default (false).
  ScopedWebUIConfigRegistration inner(
      std::make_unique<TestConfig>("inner.outer"));

  auto* outer_config = map.GetConfig(nullptr, GURL("chrome-untrusted://outer"));
  ASSERT_TRUE(outer_config);

  // A sibling of the non-opted-in host still reaches the opted-in ancestor.
  EXPECT_EQ(outer_config,
            map.GetConfig(nullptr, GURL("chrome-untrusted://other.outer")));
  // But a subdomain of the non-opted-in host stops there.
  EXPECT_EQ(nullptr,
            map.GetConfig(nullptr, GURL("chrome-untrusted://x.inner.outer")));
}

// A config that does not opt in must never be reached via a subdomain, even
// though its exact origin resolves. This guards against origin spoofing (e.g.
// chrome-untrusted://evil.settings serving the settings UI).
TEST(BraveWebUIConfigSubdomainTest, ConfigWithoutOptInRejectsSubdomains) {
  auto& map = WebUIConfigMap::GetInstance();
  // ShouldHandleSubdomains() is left at its default (false).
  ScopedWebUIConfigRegistration registration(
      std::make_unique<TestConfig>("settings"));

  EXPECT_TRUE(map.GetConfig(nullptr, GURL("chrome-untrusted://settings")));
  EXPECT_EQ(nullptr,
            map.GetConfig(nullptr, GURL("chrome-untrusted://evil.settings")));
}

// A config must be registered for the parent origin, otherwise the subdomain
// does not resolve.
TEST(BraveWebUIConfigSubdomainTest, SubdomainRequiresRegisteredConfig) {
  auto& map = WebUIConfigMap::GetInstance();

  EXPECT_EQ(nullptr, map.GetConfig(nullptr, GURL("chrome-untrusted://orphan")));
  EXPECT_EQ(nullptr,
            map.GetConfig(nullptr, GURL("chrome-untrusted://sub.orphan")));
}

// The full subdomain URL (not the collapsed parent) is passed on to the matched
// config, so it can distinguish requests per origin.
TEST(BraveWebUIConfigSubdomainTest, SubdomainPassesFullUrlToMatchedConfig) {
  auto& map = WebUIConfigMap::GetInstance();
  auto owned_config = MakeSubdomainConfig("widgets");
  auto* config = owned_config.get();
  ScopedWebUIConfigRegistration registration(std::move(owned_config));

  const GURL subdomain_url("chrome-untrusted://instance-42.widgets");
  EXPECT_EQ(config, map.GetConfig(nullptr, subdomain_url));
  EXPECT_EQ(subdomain_url, config->last_handled_url);
}

// A disabled config is not reached via a subdomain, just as it is not reached
// via its exact origin.
TEST(BraveWebUIConfigSubdomainTest, DisabledConfigRejectsSubdomain) {
  auto& map = WebUIConfigMap::GetInstance();
  auto owned_config = MakeSubdomainConfig("disabled-host");
  owned_config->enabled = false;
  ScopedWebUIConfigRegistration registration(std::move(owned_config));

  EXPECT_EQ(nullptr,
            map.GetConfig(nullptr, GURL("chrome-untrusted://disabled-host")));
  EXPECT_EQ(
      nullptr,
      map.GetConfig(nullptr, GURL("chrome-untrusted://sub.disabled-host")));
}

// Subdomain handling is limited to chrome-untrusted://: a chrome:// config
// opting in does not get its subdomains, as those would be new origins with
// WebUI bindings.
TEST(BraveWebUIConfigSubdomainTest, TrustedSchemeIgnoresSubdomainOptIn) {
  auto& map = WebUIConfigMap::GetInstance();
  ScopedWebUIConfigRegistration registration(
      MakeSubdomainConfig("trusted-host", kChromeUIScheme));

  // The exact chrome:// origin still resolves.
  EXPECT_TRUE(map.GetConfig(nullptr, GURL("chrome://trusted-host")));
  // But its subdomains do not.
  EXPECT_EQ(nullptr, map.GetConfig(nullptr, GURL("chrome://sub.trusted-host")));
}

}  // namespace content
