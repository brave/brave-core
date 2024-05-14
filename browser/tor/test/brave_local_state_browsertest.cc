/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"

using BraveLocalStateBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, BasicTest) {
  // Tor is enabled by default.
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));

  // No bridges by default.
  auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();
  EXPECT_EQ(tor::BridgesConfig::Usage::kNotUsed, bridges_config.use_bridges);
  EXPECT_TRUE(bridges_config.provided_bridges.empty());
}

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, TorEnableDisable) {
  TorProfileServiceFactory::SetTorDisabled(true);
  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));

  TorProfileServiceFactory::SetTorDisabled(false);
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
}

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, ChangeBridges) {
  tor::BridgesConfig bridges_config;
  bridges_config.use_bridges = tor::BridgesConfig::Usage::kProvide;
  bridges_config.provided_bridges.push_back("bridge1");
  bridges_config.provided_bridges.push_back("bridge2");
  bridges_config.provided_bridges.push_back("bridge3");

  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);
  EXPECT_EQ(bridges_config.ToValue(),
            TorProfileServiceFactory::GetTorBridgesConfig().ToValue());
}

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, UpdateBuiltin) {
  tor::BridgesConfig bridges_config;
  bridges_config.use_bridges = tor::BridgesConfig::Usage::kProvide;
  bridges_config.provided_bridges.push_back("bridge1");
  bridges_config.provided_bridges.push_back("bridge2");
  bridges_config.provided_bridges.push_back("bridge3");

  auto create_bridge = [](const std::string& t) {
    base::Value::List l;
    l.Append(t);
    return l;
  };
  base::Value::Dict builtin;
  builtin.Set("snowflake", create_bridge("s1"));
  builtin.Set("obfs4", create_bridge("o1"));
  builtin.Set("meek-azure", create_bridge("m1"));
  builtin.Set("unsupported", create_bridge("u1"));
  bridges_config.UpdateBuiltinBridges(builtin);

  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  using BuiltinType = tor::BridgesConfig::BuiltinType;
  EXPECT_EQ(bridges_config.ToValue(),
            TorProfileServiceFactory::GetTorBridgesConfig().ToValue());
  EXPECT_EQ(bridges_config.builtin_bridges.size(), 3u);
  EXPECT_EQ(bridges_config.builtin_bridges[BuiltinType::kSnowflake].size(), 1u);
  EXPECT_EQ(bridges_config.builtin_bridges[BuiltinType::kSnowflake][0], "s1");

  EXPECT_EQ(bridges_config.builtin_bridges[BuiltinType::kObfs4].size(), 1u);
  EXPECT_EQ(bridges_config.builtin_bridges[BuiltinType::kObfs4][0], "o1");

  EXPECT_EQ(bridges_config.builtin_bridges[BuiltinType::kMeekAzure].size(), 1u);
  EXPECT_EQ(bridges_config.builtin_bridges[BuiltinType::kMeekAzure][0], "m1");
}
