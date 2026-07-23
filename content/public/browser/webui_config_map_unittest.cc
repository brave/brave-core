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

// A valid BrowserContext is not needed for these tests: IsWebUIEnabled() below
// ignores it.
BrowserContext* const kBrowserContext = nullptr;

class TestConfig : public WebUIConfig {
 public:
  explicit TestConfig(std::string_view host,
                      std::string_view scheme = kChromeUIUntrustedScheme)
      : WebUIConfig(scheme, host) {}
  ~TestConfig() override = default;

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

// Returns a config for `host` that opts into handling its single-label
// subdomains.
std::unique_ptr<TestConfig> MakeSubdomainConfig(
    std::string_view host,
    std::string_view scheme = kChromeUIUntrustedScheme) {
  auto config = std::make_unique<TestConfig>(host, scheme);
  config->should_handle_subdomains = true;
  return config;
}

}  // namespace

// A config that opts in via ShouldHandleSubdomains() resolves both its exact
// origin and any single-label subdomain of it, while multi-label subdomains and
// unrelated hosts do not match.
TEST(BraveWebUIConfigSubdomainTest, OptedInConfigHandlesSubdomains) {
  auto& map = WebUIConfigMap::GetInstance();
  ScopedWebUIConfigRegistration registration(MakeSubdomainConfig("workspaces"));

  auto* config =
      map.GetConfig(kBrowserContext, GURL("chrome-untrusted://workspaces"));
  ASSERT_TRUE(config);

  // Single-label subdomains route to the same config.
  EXPECT_EQ(config,
            map.GetConfig(kBrowserContext,
                          GURL("chrome-untrusted://abc123.workspaces")));
  EXPECT_EQ(config, map.GetConfig(kBrowserContext,
                                  GURL("chrome-untrusted://"
                                       "00000000-1111-2222-3333-444444444444."
                                       "workspaces")));

  // Multi-label subdomains only match their direct parent, which has no
  // config, so they do not resolve.
  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://a.b.workspaces")));

  // The opted-in host must be the suffix, not a subdomain of the request.
  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext,
                          GURL("chrome-untrusted://workspaces.other")));
}

// A config that does not opt in must never be reached via a subdomain, even
// though its exact origin resolves. This guards against origin spoofing (e.g.
// chrome-untrusted://evil.settings serving the settings UI).
TEST(BraveWebUIConfigSubdomainTest, ConfigWithoutOptInRejectsSubdomains) {
  auto& map = WebUIConfigMap::GetInstance();
  // ShouldHandleSubdomains() is left at its default (false).
  ScopedWebUIConfigRegistration registration(
      std::make_unique<TestConfig>("settings"));

  EXPECT_TRUE(
      map.GetConfig(kBrowserContext, GURL("chrome-untrusted://settings")));
  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://evil.settings")));
}

// A config must be registered for the parent origin, otherwise the subdomain
// does not resolve.
TEST(BraveWebUIConfigSubdomainTest, SubdomainRequiresRegisteredConfig) {
  auto& map = WebUIConfigMap::GetInstance();

  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext, GURL("chrome-untrusted://orphan")));
  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://sub.orphan")));
}

// The full subdomain URL (not the collapsed parent) is passed on to the matched
// config, so it can distinguish requests per origin.
TEST(BraveWebUIConfigSubdomainTest, SubdomainPassesFullUrlToMatchedConfig) {
  auto& map = WebUIConfigMap::GetInstance();
  auto owned_config = MakeSubdomainConfig("widgets");
  auto* config = owned_config.get();
  ScopedWebUIConfigRegistration registration(std::move(owned_config));

  const GURL subdomain_url("chrome-untrusted://instance-42.widgets");
  EXPECT_EQ(config, map.GetConfig(kBrowserContext, subdomain_url));
  EXPECT_EQ(subdomain_url, config->last_handled_url);
}

// A disabled config is not reached via a subdomain, just as it is not reached
// via its exact origin.
TEST(BraveWebUIConfigSubdomainTest, DisabledConfigRejectsSubdomain) {
  auto& map = WebUIConfigMap::GetInstance();
  auto owned_config = MakeSubdomainConfig("disabled-host");
  owned_config->enabled = false;
  ScopedWebUIConfigRegistration registration(std::move(owned_config));

  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://disabled-host")));
  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext,
                          GURL("chrome-untrusted://sub.disabled-host")));
}

// Subdomain handling is limited to chrome-untrusted://: a chrome:// config
// opting in does not get its subdomains, as those would be new origins with
// WebUI bindings.
TEST(BraveWebUIConfigSubdomainTest, TrustedSchemeIgnoresSubdomainOptIn) {
  auto& map = WebUIConfigMap::GetInstance();
  ScopedWebUIConfigRegistration registration(
      MakeSubdomainConfig("trusted-host", kChromeUIScheme));

  // The exact chrome:// origin still resolves.
  EXPECT_TRUE(map.GetConfig(kBrowserContext, GURL("chrome://trusted-host")));
  // But its subdomains do not.
  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext, GURL("chrome://sub.trusted-host")));
}

}  // namespace content
