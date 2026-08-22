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

  std::unique_ptr<WebUIController> CreateWebUIController(
      WebUI* web_ui,
      const GURL& url) override {
    // Unused in these tests.
    return nullptr;
  }

  bool enabled = true;
  bool should_handle_url = true;
  GURL last_handled_url;
};

}  // namespace

// A host registered via RegisterUntrustedSubdomainHost() resolves both its
// exact origin and any single-label subdomain of it, while multi-label
// subdomains and unrelated hosts do not match.
TEST(BraveWebUIConfigSubdomainTest, RegisteredHostHandlesSubdomains) {
  auto& map = WebUIConfigMap::GetInstance();
  RegisterUntrustedSubdomainHost("workspaces");
  ScopedWebUIConfigRegistration registration(
      std::make_unique<TestConfig>("workspaces"));

  auto* config =
      map.GetConfig(kBrowserContext, GURL("chrome-untrusted://workspaces"));
  ASSERT_TRUE(config);

  // Single-label subdomains route to the same config.
  EXPECT_EQ(config, map.GetConfig(kBrowserContext,
                                  GURL("chrome-untrusted://abc123.workspaces")));
  EXPECT_EQ(config,
            map.GetConfig(kBrowserContext,
                          GURL("chrome-untrusted://"
                               "00000000-1111-2222-3333-444444444444."
                               "workspaces")));

  // Multi-label subdomains only match their direct parent, which is not
  // registered, so they do not resolve.
  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://a.b.workspaces")));

  // The registered host must be the suffix, not a subdomain of the request.
  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext,
                          GURL("chrome-untrusted://workspaces.other")));
}

// A host that was never registered must never be reached via a subdomain, even
// though its exact origin resolves. This guards against origin spoofing (e.g.
// chrome-untrusted://evil.settings serving the settings UI).
TEST(BraveWebUIConfigSubdomainTest, UnregisteredHostRejectsSubdomains) {
  auto& map = WebUIConfigMap::GetInstance();
  // "settings" is intentionally NOT registered as a subdomain host.
  ScopedWebUIConfigRegistration registration(
      std::make_unique<TestConfig>("settings"));

  EXPECT_TRUE(
      map.GetConfig(kBrowserContext, GURL("chrome-untrusted://settings")));
  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://evil.settings")));
}

// Registering a host is not enough on its own: a matching WebUIConfig must also
// be registered for the parent origin, otherwise the subdomain does not
// resolve.
TEST(BraveWebUIConfigSubdomainTest, SubdomainRequiresRegisteredConfig) {
  auto& map = WebUIConfigMap::GetInstance();
  RegisterUntrustedSubdomainHost("orphan");

  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext, GURL("chrome-untrusted://orphan")));
  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://sub.orphan")));
}

// The full subdomain URL (not the collapsed parent) is passed on to the matched
// config, so it can distinguish requests per origin.
TEST(BraveWebUIConfigSubdomainTest, SubdomainPassesFullUrlToMatchedConfig) {
  auto& map = WebUIConfigMap::GetInstance();
  RegisterUntrustedSubdomainHost("widgets");
  auto owned_config = std::make_unique<TestConfig>("widgets");
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
  RegisterUntrustedSubdomainHost("disabled-host");
  auto owned_config = std::make_unique<TestConfig>("disabled-host");
  auto* config = owned_config.get();
  config->enabled = false;
  ScopedWebUIConfigRegistration registration(std::move(owned_config));

  EXPECT_EQ(nullptr, map.GetConfig(kBrowserContext,
                                   GURL("chrome-untrusted://disabled-host")));
  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext,
                          GURL("chrome-untrusted://sub.disabled-host")));
}

// The subdomain registry is scoped to chrome-untrusted://. A registered host
// does not grant subdomain handling to a chrome:// config with the same host.
TEST(BraveWebUIConfigSubdomainTest, TrustedSchemeIgnoresSubdomainRegistration) {
  auto& map = WebUIConfigMap::GetInstance();
  RegisterUntrustedSubdomainHost("trusted-host");
  ScopedWebUIConfigRegistration registration(
      std::make_unique<TestConfig>("trusted-host", kChromeUIScheme));

  // The exact chrome:// origin still resolves.
  EXPECT_TRUE(map.GetConfig(kBrowserContext, GURL("chrome://trusted-host")));
  // But its subdomains do not, because the registry is untrusted-only.
  EXPECT_EQ(nullptr,
            map.GetConfig(kBrowserContext, GURL("chrome://sub.trusted-host")));
}

}  // namespace content
